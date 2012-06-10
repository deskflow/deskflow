/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <rfb/VNCServerST.h>
#include <rfb/Configuration.h>
#include <rfb/SSecurityFactoryStandard.h>
#include <rfb/Timer.h>
#include <network/TcpSocket.h>
#include <tx/TXWindow.h>

#include "QueryConnectDialog.h"
#include "Image.h"
#include <signal.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>


//#include <rfb/Encoder.h>

using namespace rfb;
using namespace rdr;
using namespace network;

LogWriter vlog("main");

StringParameter displayname("display", "The X display", "");
IntParameter rfbport("rfbport", "TCP port to listen for RFB protocol",5900);
IntParameter queryConnectTimeout("QueryConnectTimeout",
                                 "Number of seconds to show the Accept Connection dialog before "
                                 "rejecting the connection",
                                 10);


static void CleanupSignalHandler(int sig)
{
  // CleanupSignalHandler allows C++ object cleanup to happen because it calls
  // exit() rather than the default which is to abort.
  fprintf(stderr,"CleanupSignalHandler called\n");
  exit(1);
}


class QueryConnHandler : public VNCServerST::QueryConnectionHandler,
                         public QueryResultCallback {
public:
  QueryConnHandler(Display* dpy, VNCServerST* vs)
    : display(dpy), server(vs), queryConnectDialog(0), queryConnectSock(0) {}
  ~QueryConnHandler() { delete queryConnectDialog; }

  // -=- VNCServerST::QueryConnectionHandler interface
  virtual VNCServerST::queryResult queryConnection(network::Socket* sock,
                                                   const char* userName,
                                                   char** reason) {
    if (queryConnectSock) {
      *reason = strDup("Another connection is currently being queried.");
      return VNCServerST::REJECT;
    }
    if (!userName) userName = "(anonymous)";
    queryConnectSock = sock;
    CharArray address(sock->getPeerAddress());
    delete queryConnectDialog;
    queryConnectDialog = new QueryConnectDialog(display, address.buf,
                                                userName, queryConnectTimeout,
                                                this);
    queryConnectDialog->map();
    return VNCServerST::PENDING;
  }

  // -=- QueryResultCallback interface
  virtual void queryApproved() {
    server->approveConnection(queryConnectSock, true, 0);
    queryConnectSock = 0;
  }
  virtual void queryRejected() {
    server->approveConnection(queryConnectSock, false,
                              "Connection rejected by local user");
    queryConnectSock = 0;
  }
private:
  Display* display;
  VNCServerST* server;
  QueryConnectDialog* queryConnectDialog;
  network::Socket* queryConnectSock;
};


class XDesktop : public SDesktop, public ColourMap,
                 public Timer::Callback
{
public:
  XDesktop(Display* dpy_)
    : dpy(dpy_), pb(0), server(0), image(0), oldButtonMask(0),
      haveXtest(false), pollTimer(this)
  {
    int xtestEventBase;
    int xtestErrorBase;
    int major, minor;

    if (XTestQueryExtension(dpy, &xtestEventBase,
                            &xtestErrorBase, &major, &minor)) {
      XTestGrabControl(dpy, True);
      vlog.info("XTest extension present - version %d.%d",major,minor);
      haveXtest = true;
    } else {
      vlog.info("XTest extension not present");
      vlog.info("unable to inject events or display while server is grabbed");
    }

  }
  virtual ~XDesktop() {
    stop();
  }

  // -=- SDesktop interface

  virtual void start(VNCServer* vs) {
    int dpyWidth = DisplayWidth(dpy, DefaultScreen(dpy));
    int dpyHeight = DisplayHeight(dpy, DefaultScreen(dpy));
    Visual* vis = DefaultVisual(dpy, DefaultScreen(dpy));

    image = new Image(dpy, dpyWidth, dpyHeight);
    image->get(DefaultRootWindow(dpy));

    pf.bpp = image->xim->bits_per_pixel;
    pf.depth = image->xim->depth;
    pf.bigEndian = (image->xim->byte_order == MSBFirst);
    pf.trueColour = (vis->c_class == TrueColor);
    pf.redShift   = ffs(vis->red_mask) - 1;
    pf.greenShift = ffs(vis->green_mask) - 1;
    pf.blueShift  = ffs(vis->blue_mask) - 1;
    pf.redMax     = vis->red_mask   >> pf.redShift;
    pf.greenMax   = vis->green_mask >> pf.greenShift;
    pf.blueMax    = vis->blue_mask  >> pf.blueShift;

    pb = new FullFramePixelBuffer(pf, dpyWidth, dpyHeight,
                                  (rdr::U8*)image->xim->data, this);
    server = vs;
    server->setPixelBuffer(pb);
    pollTimer.start(50);
  }

  virtual void stop() {
    pollTimer.stop();
    delete pb;
    delete image;
  }

  virtual void pointerEvent(const Point& pos, int buttonMask) {
    if (!haveXtest) return;
    XTestFakeMotionEvent(dpy, DefaultScreen(dpy), pos.x, pos.y, CurrentTime);
    if (buttonMask != oldButtonMask) {
      for (int i = 0; i < 5; i++) {
	if ((buttonMask ^ oldButtonMask) & (1<<i)) {
          if (buttonMask & (1<<i)) {
            XTestFakeButtonEvent(dpy, i+1, True, CurrentTime);
          } else {
            XTestFakeButtonEvent(dpy, i+1, False, CurrentTime);
          }
        }
      }
    }
    oldButtonMask = buttonMask;
  }

  virtual void keyEvent(rdr::U32 key, bool down) {
    if (!haveXtest) return;
    int keycode = XKeysymToKeycode(dpy, key);
    if (keycode)
      XTestFakeKeyEvent(dpy, keycode, down, CurrentTime);
  }

  virtual void clientCutText(const char* str, int len) {
  }

  virtual Point getFbSize() {
    return Point(pb->width(), pb->height());
  }

  // -=- ColourMap callbacks
  virtual void lookup(int index, int* r, int* g, int* b) {
    XColor xc;
    xc.pixel = index;
    if (index < DisplayCells(dpy,DefaultScreen(dpy))) {
      XQueryColor(dpy, DefaultColormap(dpy,DefaultScreen(dpy)), &xc);
    } else {
      xc.red = xc.green = xc.blue = 0;
    }
    *r = xc.red;
    *g = xc.green;
    *b = xc.blue;
  }

  // -=- Timer::Callback interface
  virtual bool handleTimeout(Timer* t) {
    if (server->clientsReadyForUpdate()) {
      image->get(DefaultRootWindow(dpy));
      server->add_changed(pb->getRect());
      server->tryUpdate();
    }
    return true;
  }

protected:
  Display* dpy;
  PixelFormat pf;
  PixelBuffer* pb;
  VNCServer* server;
  Image* image;
  int oldButtonMask;
  bool haveXtest;
  Timer pollTimer;
};

char* programName;

static void usage()
{
  fprintf(stderr, "\nusage: %s [<parameters>]\n", programName);
  fprintf(stderr,"\n"
          "Parameters can be turned on with -<param> or off with -<param>=0\n"
          "Parameters which take a value can be specified as "
          "-<param> <value>\n"
          "Other valid forms are <param>=<value> -<param>=<value> "
          "--<param>=<value>\n"
          "Parameter names are case-insensitive.  The parameters are:\n\n");
  Configuration::listParams(79, 14);
  exit(1);
}

int main(int argc, char** argv)
{
  initStdIOLoggers();
  LogWriter::setLogParams("*:stderr:30");

  programName = argv[0];
  Display* dpy;

  for (int i = 1; i < argc; i++) {
    if (Configuration::setParam(argv[i]))
      continue;

    if (argv[i][0] == '-') {
      if (i+1 < argc) {
        if (Configuration::setParam(&argv[i][1], argv[i+1])) {
          i++;
          continue;
        }
      }
      usage();
    }

    usage();
  }

  CharArray dpyStr(displayname.getData());
  if (!(dpy = XOpenDisplay(dpyStr.buf[0] ? dpyStr.buf : 0))) {
    fprintf(stderr,"%s: unable to open display \"%s\"\r\n",
            programName, XDisplayName(displayname.getData()));
    exit(1);
  }

  signal(SIGHUP, CleanupSignalHandler);
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  try {
    TXWindow::init(dpy,"x0vncserver");
    XDesktop desktop(dpy);
    VNCServerST server("x0vncserver", &desktop);
    QueryConnHandler qcHandler(dpy, &server);
    server.setQueryConnectionHandler(&qcHandler);

    TcpListener listener((int)rfbport);
    vlog.info("Listening on port %d", (int)rfbport);

    while (true) {
      struct timeval tv;
      struct timeval* tvp = 0;
      fd_set rfds;
      std::list<Socket*> sockets;
      std::list<Socket*>::iterator i;

      // Process any incoming X events
      TXWindow::handleXEvents(dpy);
      
      // Process expired timers and get the time until the next one
      int timeoutMs = Timer::checkTimeouts();
      soonestTimeout(&timeoutMs, server.checkTimeouts());
      if (timeoutMs) {
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        tvp = &tv;
      }
    
      // Wait for X events, VNC traffic, or the next timer expiry
      // NB: This code assumes that:
      //   - removeSocket cannot cause the next timer to become sooner
      //   - removeSocket cannot cause other sockets to shutdown()
      FD_ZERO(&rfds);
      FD_SET(listener.getFd(), &rfds);
      FD_SET(ConnectionNumber(dpy), &rfds);
      server.getSockets(&sockets);
      for (i = sockets.begin(); i != sockets.end(); i++) {
        if ((*i)->isShutdown()) {
          server.removeSocket(*i);
          delete (*i);
        } else {
          FD_SET((*i)->getFd(), &rfds);
        }
      }
      
      // If there are X requests pending then poll, don't wait!
      if (XPending(dpy)) {
        tv.tv_usec = tv.tv_sec = 0;
        tvp = &tv;
      }
      
      // Do the wait...
      int n = select(FD_SETSIZE, &rfds, 0, 0, &tv);
      if (n < 0) throw rdr::SystemException("select",errno);

      // Accept new VNC connections
      if (FD_ISSET(listener.getFd(), &rfds)) {
        Socket* sock = listener.accept();
        if (sock) server.addSocket(sock);
      }

      // Process events on existing VNC connections
      for (i = sockets.begin(); i != sockets.end(); i++) {
        if (FD_ISSET((*i)->getFd(), &rfds))
          server.processSocketEvent(*i);
      }
    }

  } catch (rdr::Exception &e) {
    vlog.error(e.str());
  };

  return 0;
}
