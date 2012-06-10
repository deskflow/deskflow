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
//
// CConn.cxx
//

#include <unistd.h>
#include "CConn.h"
#include <rfb/CMsgWriter.h>
#include <rfb/encodings.h>
#include <rfb/secTypes.h>
#include <rfb/CSecurityNone.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/Hostname.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb/Password.h>
#include <network/TcpSocket.h>

#include "TXViewport.h"
#include "DesktopWindow.h"
#include "ServerDialog.h"
#include "PasswdDialog.h"
#include "parameters.h"

using namespace rfb;

static rfb::LogWriter vlog("CConn");

IntParameter debugDelay("DebugDelay","Milliseconds to display inverted "
                        "pixel data - a debugging feature", 0);

StringParameter menuKey("MenuKey", "The key which brings up the popup menu",
                        "F8");
StringParameter windowName("name", "The X window name", "");

CConn::CConn(Display* dpy_, int argc_, char** argv_, network::Socket* sock_,
             char* vncServerName, bool reverse)
  : dpy(dpy_), argc(argc_),
    argv(argv_), serverHost(0), serverPort(0), sock(sock_), viewport(0),
    desktop(0), desktopEventHandler(0),
    currentEncoding(encodingZRLE), lastServerEncoding((unsigned int)-1),
    fullColour(::fullColour),
    autoSelect(::autoSelect), shared(::shared), formatChange(false),
    encodingChange(false), sameMachine(false), fullScreen(::fullScreen),
    ctrlDown(false), altDown(false),
    menuKeysym(0), menu(dpy, this), options(dpy, this), about(dpy), info(dpy),
    reverseConnection(reverse)
{
  CharArray menuKeyStr(menuKey.getData());
  menuKeysym = XStringToKeysym(menuKeyStr.buf);

  setShared(shared);
  addSecType(secTypeNone);
  addSecType(secTypeVncAuth);
  CharArray encStr(preferredEncoding.getData());
  int encNum = encodingNum(encStr.buf);
  if (encNum != -1) {
    currentEncoding = encNum;
    autoSelect = false;
  }
  cp.supportsDesktopResize = true;
  cp.supportsLocalCursor = useLocalCursor;
  initMenu();

  if (sock) {
    char* name = sock->getPeerEndpoint();
    vlog.info("Accepted connection from %s", name);
    if (name) free(name);
  } else {
    if (vncServerName) {
      getHostAndPort(vncServerName, &serverHost, &serverPort);
    } else {
      ServerDialog dlg(dpy, &options, &about);
      if (!dlg.show() || dlg.entry.getText()[0] == 0) {
        exit(1);
      }
      getHostAndPort(dlg.entry.getText(), &serverHost, &serverPort);
    }

    sock = new network::TcpSocket(serverHost, serverPort);
    vlog.info("connected to host %s port %d", serverHost, serverPort);
  }

  sameMachine = sock->sameMachine();
  sock->inStream().setBlockCallback(this);
  setServerName(sock->getPeerEndpoint());
  setStreams(&sock->inStream(), &sock->outStream());
  initialiseProtocol();
}

CConn::~CConn() {
  free(serverHost);
  delete desktop;
  delete viewport;
  delete sock;
}

// deleteWindow() is called when the user closes the desktop or menu windows.

void CConn::deleteWindow(TXWindow* w) {
  if (w == &menu) {
    menu.unmap();
  } else if (w == viewport) {
    exit(1);
  }
}

// handleEvent() filters all events on the desktop and menu.  Most are passed
// straight through.  The exception is the F8 key.  When pressed on the
// desktop, it is used to bring up the menu.  An F8 press or release on the
// menu is passed through as if it were on the desktop.

void CConn::handleEvent(TXWindow* w, XEvent* ev)
{
  KeySym ks;
  char str[256];

  switch (ev->type) {
  case KeyPress:
  case KeyRelease:
    XLookupString(&ev->xkey, str, 256, &ks, NULL);
    if (ks == menuKeysym && (ev->xkey.state & (ShiftMask|ControlMask)) == 0) {
      if (w == desktop && ev->type == KeyPress) {
        showMenu(ev->xkey.x_root, ev->xkey.y_root);
        break;
      } else if (w == &menu) {
        if (ev->type == KeyPress) menu.unmap();
        desktopEventHandler->handleEvent(w, ev);
        break;
      }
    }
    // drop through

  default:
    if (w == desktop) desktopEventHandler->handleEvent(w, ev);
    else if (w == &menu) menuEventHandler->handleEvent(w, ev);
  }
}

// blockCallback() is called when reading from the socket would block.  We
// process X events until the socket is ready for reading again.

void CConn::blockCallback() {
  fd_set rfds;
  do {
    struct timeval tv;
    struct timeval* tvp = 0;

    // Process any incoming X events
    TXWindow::handleXEvents(dpy);
    
    // Process expired timers and get the time until the next one
    int timeoutMs = Timer::checkTimeouts();
    if (timeoutMs) {
      tv.tv_sec = timeoutMs / 1000;
      tv.tv_usec = (timeoutMs % 1000) * 1000;
      tvp = &tv;
    }
    
    // If there are X requests pending then poll, don't wait!
    if (XPending(dpy)) {
      tv.tv_usec = tv.tv_sec = 0;
      tvp = &tv;
    }

    // Wait for X events, VNC traffic, or the next timer expiry
    FD_ZERO(&rfds);
    FD_SET(ConnectionNumber(dpy), &rfds);
    FD_SET(sock->getFd(), &rfds);
    int n = select(FD_SETSIZE, &rfds, 0, 0, tvp);
    if (n < 0) throw rdr::SystemException("select",errno);
  } while (!(FD_ISSET(sock->getFd(), &rfds)));
}


// getPasswd() is called by the CSecurity object when it needs us to read a
// password from the user.

void CConn::getUserPasswd(char** user, char** password)
{
  CharArray passwordFileStr(passwordFile.getData());
  if (!user && passwordFileStr.buf[0]) {
    FILE* fp = fopen(passwordFileStr.buf, "r");
    if (!fp) throw rfb::Exception("Opening password file failed");
    ObfuscatedPasswd obfPwd(256);
    obfPwd.length = fread(obfPwd.buf, 1, obfPwd.length, fp);
    fclose(fp);
    PlainPasswd passwd(obfPwd);
    *password = passwd.takeBuf();
    return;
  }

  const char* secType = secTypeName(getCurrentCSecurity()->getType());
  const char* titlePrefix = "VNC Authentication";
  CharArray title(strlen(titlePrefix) + strlen(secType) + 4);
  sprintf(title.buf, "%s [%s]", titlePrefix, secType);
  PasswdDialog dlg(dpy, title.buf, !user);
  if (!dlg.show()) throw rfb::Exception("Authentication cancelled");
  if (user)
    *user = strDup(dlg.userEntry.getText());
  *password = strDup(dlg.passwdEntry.getText());
}


// CConnection callback methods

// getCSecurity() gets the appropriate CSecurity object for the security
// types which we support.
CSecurity* CConn::getCSecurity(int secType) {
  switch (secType) {
  case secTypeNone:
    return new CSecurityNone();
  case secTypeVncAuth:
    return new CSecurityVncAuth(this);
  default:
    throw rfb::Exception("Unsupported secType?");
  }
}

// serverInit() is called when the serverInit message has been received.  At
// this point we create the desktop window and display it.  We also tell the
// server the pixel format and encodings to use and request the first update.
void CConn::serverInit() {
  CConnection::serverInit();
  serverPF = cp.pf();
  desktop = new DesktopWindow(dpy, cp.width, cp.height, serverPF, this);
  desktopEventHandler = desktop->setEventHandler(this);
  desktop->addEventMask(KeyPressMask | KeyReleaseMask);
  fullColourPF = desktop->getPF();
  if (!serverPF.trueColour)
    fullColour = true;
  recreateViewport();
  formatChange = encodingChange = true;
  requestNewUpdate();
}

// setDesktopSize() is called when the desktop size changes (including when
// it is set initially).
void CConn::setDesktopSize(int w, int h) {
  CConnection::setDesktopSize(w,h);
  if (desktop) {
    desktop->resize(w, h);
    recreateViewport();
  }
}

// framebufferUpdateEnd() is called at the end of an update.
// For each rectangle, the FdInStream will have timed the speed
// of the connection, allowing us to select format and encoding
// appropriately, and then request another incremental update.
void CConn::framebufferUpdateEnd() {
  if (debugDelay != 0) {
    XSync(dpy, False);
    struct timeval tv;
    tv.tv_sec = debugDelay / 1000;
    tv.tv_usec = (debugDelay % 1000) * 1000;
    select(0, 0, 0, 0, &tv);
    std::list<rfb::Rect>::iterator i;
    for (i = debugRects.begin(); i != debugRects.end(); i++) {
      desktop->invertRect(*i);
    }
    debugRects.clear();
  }
  desktop->framebufferUpdateEnd();
  if (autoSelect)
    autoSelectFormatAndEncoding();
  requestNewUpdate();
}

// The rest of the callbacks are fairly self-explanatory...

void CConn::setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs)
{
  desktop->setColourMapEntries(firstColour, nColours, rgbs);
}

void CConn::bell() { XBell(dpy, 0); }

void CConn::serverCutText(const char* str, int len) {
  desktop->serverCutText(str,len);
}

// We start timing on beginRect and stop timing on endRect, to
// avoid skewing the bandwidth estimation as a result of the server
// being slow or the network having high latency
void CConn::beginRect(const Rect& r, unsigned int encoding)
{
  sock->inStream().startTiming();
  if (encoding != encodingCopyRect) {
    lastServerEncoding = encoding;
  }
}

void CConn::endRect(const Rect& r, unsigned int encoding)
{
  sock->inStream().stopTiming();
  if (debugDelay != 0) {
    desktop->invertRect(r);
    debugRects.push_back(r);
  }
}

void CConn::fillRect(const rfb::Rect& r, rfb::Pixel p) {
  desktop->fillRect(r,p);
}
void CConn::imageRect(const rfb::Rect& r, void* p) {
  desktop->imageRect(r,p);
}
void CConn::copyRect(const rfb::Rect& r, int sx, int sy) {
  desktop->copyRect(r,sx,sy);
}
void CConn::setCursor(int width, int height, const Point& hotspot,
                      void* data, void* mask) {
  desktop->setCursor(width, height, hotspot, data, mask);
}


// Menu stuff - menuSelect() is called when the user selects a menu option.

enum { ID_OPTIONS, ID_INFO, ID_FULLSCREEN, ID_REFRESH, ID_F8, ID_CTRLALTDEL,
       ID_ABOUT, ID_DISMISS, ID_EXIT, ID_NEWCONN, ID_CTRL, ID_ALT };

void CConn::initMenu() {
  menuEventHandler = menu.setEventHandler(this);
  menu.addEventMask(KeyPressMask | KeyReleaseMask);
  menu.addEntry("Exit viewer", ID_EXIT);
  menu.addEntry(0, 0);
  menu.addEntry("Full screen", ID_FULLSCREEN);
  menu.check(ID_FULLSCREEN, fullScreen);
  menu.addEntry(0, 0);
  menu.addEntry("Ctrl", ID_CTRL);
  menu.addEntry("Alt", ID_ALT);
  CharArray menuKeyStr(menuKey.getData());
  CharArray sendMenuKey(6+strlen(menuKeyStr.buf));
  sprintf(sendMenuKey.buf, "Send %s", menuKeyStr.buf);
  menu.addEntry(sendMenuKey.buf, ID_F8);
  menu.addEntry("Send Ctrl-Alt-Del", ID_CTRLALTDEL);
  menu.addEntry(0, 0);
  menu.addEntry("Refresh screen", ID_REFRESH);
  menu.addEntry(0, 0);
  menu.addEntry("New connection...", ID_NEWCONN);
  menu.addEntry("Options...", ID_OPTIONS);
  menu.addEntry("Connection info...", ID_INFO);
  menu.addEntry("About VNCviewer...", ID_ABOUT);
  menu.addEntry(0, 0);
  menu.addEntry("Dismiss menu", ID_DISMISS);
  menu.toplevel("VNC Menu", this);
  menu.setBorderWidth(1);
}

void CConn::showMenu(int x, int y) {
  menu.check(ID_FULLSCREEN, fullScreen);
  menu.move(x, y);
  menu.raise();
  menu.map();
}

void CConn::menuSelect(long id, TXMenu* m) {
  switch (id) {
  case ID_NEWCONN:
    {
      menu.unmap();
      if (fullScreen) {
        fullScreen = false;
        if (viewport) recreateViewport();
      }
      int pid = fork();
      if (pid < 0) { perror("fork"); exit(1); }
      if (pid == 0) {
        delete sock;
        close(ConnectionNumber(dpy));
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200*1000;
        select(0, 0, 0, 0, &tv);
        execlp(programName, programName, 0);
        perror("execlp"); exit(1);
      }
      break;
    }
  case ID_OPTIONS:
    menu.unmap();
    options.show();
    break;
  case ID_INFO:
    {
      menu.unmap();
      char pfStr[100];
      char spfStr[100];
      cp.pf().print(pfStr, 100);
      serverPF.print(spfStr, 100);
      int secType = getCurrentCSecurity()->getType();
      char infoText[1024];
      sprintf(infoText,
              "Desktop name: %.80s\n"
              "Host: %.80s port: %d\n"
              "Size: %d x %d\n"
              "Pixel format: %s\n"
              "(server default %s)\n"
              "Requested encoding: %s\n"
              "Last used encoding: %s\n"
              "Line speed estimate: %d kbit/s\n"
              "Protocol version: %d.%d\n"
              "Security method: %s\n",
              cp.name(), serverHost, serverPort, cp.width, cp.height,
              pfStr, spfStr, encodingName(currentEncoding),
              encodingName(lastServerEncoding),
              sock->inStream().kbitsPerSecond(),
              cp.majorVersion, cp.minorVersion,
              secTypeName(secType));
      info.setText(infoText);
      info.show();
      break;
    }
  case ID_FULLSCREEN:
    menu.unmap();
    fullScreen = !fullScreen;
    if (viewport) recreateViewport();
    break;
  case ID_REFRESH:
    menu.unmap();
    writer()->writeFramebufferUpdateRequest(Rect(0, 0, cp.width, cp.height),
                                            false);
    break;
  case ID_F8:
    menu.unmap();
    if (!viewOnly) {
      writer()->keyEvent(menuKeysym, true);
      writer()->keyEvent(menuKeysym, false);
    }
    break;
  case ID_CTRLALTDEL:
    menu.unmap();
    if (!viewOnly) {
      writer()->keyEvent(XK_Control_L, true);
      writer()->keyEvent(XK_Alt_L, true);
      writer()->keyEvent(XK_Delete, true);
      writer()->keyEvent(XK_Delete, false);
      writer()->keyEvent(XK_Alt_L, false);
      writer()->keyEvent(XK_Control_L, false);
    }
    break;
  case ID_CTRL:
    menu.unmap();
    if (!viewOnly) {
      ctrlDown = !ctrlDown;
      writer()->keyEvent(XK_Control_L, ctrlDown);
      menu.check(ID_CTRL, ctrlDown);
    }
    break;
  case ID_ALT:
    menu.unmap();
    if (!viewOnly) {
      altDown = !altDown;
      writer()->keyEvent(XK_Alt_L, altDown);
      menu.check(ID_ALT, altDown);
    }
    break;
  case ID_ABOUT:
    menu.unmap();
    about.show();
    break;
  case ID_DISMISS:
    menu.unmap();
    break;
  case ID_EXIT:
    exit(1);
    break;
  }
}


// OptionsDialogCallback.  setOptions() sets the options dialog's checkboxes
// etc to reflect our flags.  getOptions() sets our flags according to the
// options dialog's checkboxes.

void CConn::setOptions() {
  options.autoSelect.checked(autoSelect);
  options.fullColour.checked(fullColour);
  options.veryLowColour.checked(!fullColour && lowColourLevel == 0);
  options.lowColour.checked(!fullColour && lowColourLevel == 1);
  options.mediumColour.checked(!fullColour && lowColourLevel == 2);
  options.zrle.checked(currentEncoding == encodingZRLE);
  options.hextile.checked(currentEncoding == encodingHextile);
  options.raw.checked(currentEncoding == encodingRaw);
  options.viewOnly.checked(viewOnly);
  options.acceptClipboard.checked(acceptClipboard);
  options.sendClipboard.checked(sendClipboard);
  options.sendPrimary.checked(sendPrimary);
  if (state() == RFBSTATE_NORMAL)
    options.shared.disabled(true);
  else
    options.shared.checked(shared);
  options.fullScreen.checked(fullScreen);
  options.useLocalCursor.checked(useLocalCursor);
  options.dotWhenNoCursor.checked(dotWhenNoCursor);
}

void CConn::getOptions() {
  autoSelect = options.autoSelect.checked();
  if (fullColour != options.fullColour.checked())
    formatChange = true;
  fullColour = options.fullColour.checked();
  if (!fullColour) {
    int newLowColourLevel = (options.veryLowColour.checked() ? 0 :
                             options.lowColour.checked() ? 1 : 2);
    if (newLowColourLevel != lowColourLevel) {
      lowColourLevel.setParam(newLowColourLevel);
      formatChange = true;
    }
  }
  unsigned int newEncoding = (options.zrle.checked() ? encodingZRLE :
                              options.hextile.checked() ? encodingHextile :
                              encodingRaw);
  if (newEncoding != currentEncoding) {
    currentEncoding = newEncoding;
    encodingChange = true;
  }
  viewOnly.setParam(options.viewOnly.checked());
  acceptClipboard.setParam(options.acceptClipboard.checked());
  sendClipboard.setParam(options.sendClipboard.checked());
  sendPrimary.setParam(options.sendPrimary.checked());
  shared = options.shared.checked();
  setShared(shared);
  if (fullScreen != options.fullScreen.checked()) {
    fullScreen = options.fullScreen.checked();
    if (viewport) recreateViewport();
  }
  useLocalCursor.setParam(options.useLocalCursor.checked());
  if (cp.supportsLocalCursor != useLocalCursor) {
    cp.supportsLocalCursor = useLocalCursor;
    encodingChange = true;
    if (desktop)
      desktop->resetLocalCursor();
  }
  dotWhenNoCursor.setParam(options.dotWhenNoCursor.checked());
  checkEncodings();
}

void CConn::recreateViewport()
{
  TXViewport* oldViewport = viewport;
  viewport = new TXViewport(dpy, cp.width, cp.height);
  desktop->setViewport(viewport);
  CharArray windowNameStr(windowName.getData());
  if (!windowNameStr.buf[0]) {
    windowNameStr.replaceBuf(new char[256]);
    sprintf(windowNameStr.buf,"VNC: %.240s",cp.name());
  }
  viewport->toplevel(windowNameStr.buf, this, argc, argv);
  viewport->setBumpScroll(fullScreen);
  XSetWindowAttributes attr;
  attr.override_redirect = fullScreen;
  XChangeWindowAttributes(dpy, viewport->win(), CWOverrideRedirect, &attr);
  XChangeWindowAttributes(dpy, menu.win(), CWOverrideRedirect, &attr);
  XChangeWindowAttributes(dpy, options.win(), CWOverrideRedirect, &attr);
  XChangeWindowAttributes(dpy, about.win(), CWOverrideRedirect, &attr);
  XChangeWindowAttributes(dpy, info.win(), CWOverrideRedirect, &attr);
  reconfigureViewport();
  menu.setTransientFor(viewport->win());
  viewport->map();
  if (fullScreen) {
    XGrabKeyboard(dpy, desktop->win(), True, GrabModeAsync, GrabModeAsync,
                  CurrentTime);
  } else {
    XUngrabKeyboard(dpy, CurrentTime);
  }
  if (oldViewport) delete oldViewport;
}

void CConn::reconfigureViewport()
{
  viewport->setMaxSize(cp.width, cp.height);
  if (fullScreen) {
    viewport->resize(DisplayWidth(dpy,DefaultScreen(dpy)),
                     DisplayHeight(dpy,DefaultScreen(dpy)));
  } else {
    int w = cp.width;
    int h = cp.height;
    if (w + wmDecorationWidth >= DisplayWidth(dpy,DefaultScreen(dpy)))
      w = DisplayWidth(dpy,DefaultScreen(dpy)) - wmDecorationWidth;
    if (h + wmDecorationHeight >= DisplayHeight(dpy,DefaultScreen(dpy)))
      h = DisplayHeight(dpy,DefaultScreen(dpy)) - wmDecorationHeight;

    int x = (DisplayWidth(dpy,DefaultScreen(dpy)) - w - wmDecorationWidth) / 2;
    int y = (DisplayHeight(dpy,DefaultScreen(dpy)) - h - wmDecorationHeight)/2;

    CharArray geometryStr(geometry.getData());
    viewport->setGeometry(geometryStr.buf, x, y, w, h);
  }
}

// autoSelectFormatAndEncoding() chooses the format and encoding appropriate
// to the connection speed:
//   Above 16Mbps (timing for at least a second), same machine, switch to raw
//   Above 3Mbps, switch to hextile
//   Below 1.5Mbps, switch to ZRLE
//   Above 1Mbps, switch to full colour mode
void CConn::autoSelectFormatAndEncoding()
{
  int kbitsPerSecond = sock->inStream().kbitsPerSecond();
  unsigned int newEncoding = currentEncoding;

  if (kbitsPerSecond > 16000 && sameMachine &&
      sock->inStream().timeWaited() >= 10000) {
    newEncoding = encodingRaw;
  } else if (kbitsPerSecond > 3000) {
    newEncoding = encodingHextile;
  } else if (kbitsPerSecond < 1500) {
    newEncoding = encodingZRLE;
  }

  if (newEncoding != currentEncoding) {
    vlog.info("Throughput %d kbit/s - changing to %s encoding",
              kbitsPerSecond, encodingName(newEncoding));
    currentEncoding = newEncoding;
    encodingChange = true;
  }

  if (kbitsPerSecond > 1000) {
    if (!fullColour) {
      vlog.info("Throughput %d kbit/s - changing to full colour",
                kbitsPerSecond);
      fullColour = true;
      formatChange = true;
    }
  }
}

// checkEncodings() sends a setEncodings message if one is needed.
void CConn::checkEncodings()
{
  if (encodingChange && writer()) {
    vlog.info("Using %s encoding",encodingName(currentEncoding));
    writer()->writeSetEncodings(currentEncoding, true);
    encodingChange = false;
  }
}

// requestNewUpdate() requests an update from the server, having set the
// format and encoding appropriately.
void CConn::requestNewUpdate()
{
  if (formatChange) {
    if (fullColour) {
      desktop->setPF(fullColourPF);
    } else {
      if (lowColourLevel == 0)
        desktop->setPF(PixelFormat(8,3,0,1,1,1,1,2,1,0));
      else if (lowColourLevel == 1)
        desktop->setPF(PixelFormat(8,6,0,1,3,3,3,4,2,0));
      else
        desktop->setPF(PixelFormat(8,8,0,0));
    }
    char str[256];
    desktop->getPF().print(str, 256);
    vlog.info("Using pixel format %s",str);
    cp.setPF(desktop->getPF());
    writer()->writeSetPixelFormat(cp.pf());
  }
  checkEncodings();
  writer()->writeFramebufferUpdateRequest(Rect(0, 0, cp.width, cp.height),
                                          !formatChange);
  formatChange = false;
}
