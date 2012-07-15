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
// XserverDesktop.cxx
//

#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <network/TcpSocket.h>
#include <rfb/Exception.h>
#include <rfb/VNCServerST.h>
#include <rfb/HTTPServer.h>
#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>
#include "XserverDesktop.h"
#include "vncExtInit.h"

extern "C" {
#define public c_public
#define class c_class

  // windowTable is in globals.h in XFree 4, but not in XFree 3 unfortunately
extern WindowPtr *WindowTable;
extern char *display;

#include "inputstr.h"
#include "servermd.h"
#include "colormapst.h"
#include "resource.h"
#include "cursorstr.h"
#include "windowstr.h"
#define XK_CYRILLIC
#include "keysym.h"
#undef public
#undef class
}

using namespace rfb;
using namespace network;

static LogWriter vlog("XserverDesktop");

rfb::IntParameter deferUpdateTime("DeferUpdate",
                                  "Time in milliseconds to defer updates",40);

rfb::BoolParameter alwaysSetDeferUpdateTimer("AlwaysSetDeferUpdateTimer",
                  "Always reset the defer update timer on every change",false);

IntParameter queryConnectTimeout("QueryConnectTimeout",
                                 "Number of seconds to show the Accept Connection dialog before "
                                 "rejecting the connection",
                                 10);

static KeyCode KeysymToKeycode(KeySymsPtr keymap, KeySym ks, int* col);

static rdr::U8 reverseBits[] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0,
  0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4,
  0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc,
  0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca,
  0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6,
  0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1,
  0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9,
  0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd,
  0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3,
  0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7,
  0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf,
  0x3f, 0xbf, 0x7f, 0xff
};


class FileHTTPServer : public rfb::HTTPServer {
public:
  FileHTTPServer(XserverDesktop* d) : desktop(d) {}
  virtual ~FileHTTPServer() {}

  virtual rdr::InStream* getFile(const char* name, const char** contentType,
                                 int* contentLength, time_t* lastModified)
  {
    if (name[0] != '/' || strstr(name, "..") != 0) {
      vlog.info("http request was for invalid file name");
      return 0;
    }

    if (strcmp(name, "/") == 0) name = "/index.vnc";

    CharArray httpDirStr(httpDir.getData());
    CharArray fname(strlen(httpDirStr.buf)+strlen(name)+1);
    sprintf(fname.buf, "%s%s", httpDirStr.buf, name);
    int fd = open(fname.buf, O_RDONLY);
    if (fd < 0) return 0;
    rdr::InStream* is = new rdr::FdInStream(fd, -1, 0, true);
    *contentType = guessContentType(name, *contentType);
    if (strlen(name) > 4 && strcasecmp(&name[strlen(name)-4], ".vnc") == 0) {
      is = new rdr::SubstitutingInStream(is, desktop, 20);
      *contentType = "text/html";
    } else {
      struct stat st;
      if (fstat(fd, &st) == 0) {
        *contentLength = st.st_size;
        *lastModified = st.st_mtime;
      }
    }
    return is;
  }

  XserverDesktop* desktop;
};


XserverDesktop::XserverDesktop(ScreenPtr pScreen_,
                               network::TcpListener* listener_,
                               network::TcpListener* httpListener_,
                               const char* name, void* fbptr)
  : pScreen(pScreen_), deferredUpdateTimer(0), dummyTimer(0),
    server(0), httpServer(0),
    listener(listener_), httpListener(httpListener_),
    cmap(0), deferredUpdateTimerSet(false),
    grabbing(false), ignoreHooks_(false), directFbptr(fbptr != 0),
    oldButtonMask(0),
    queryConnectId(0)
{
  int i;
  format.depth = pScreen->rootDepth;
  for (i = 0; i < screenInfo.numPixmapFormats; i++) {
    if (screenInfo.formats[i].depth == format.depth) {
      format.bpp = screenInfo.formats[i].bitsPerPixel;
      break;
    }
  }
  if (i == screenInfo.numPixmapFormats) {
    fprintf(stderr,"no pixmap format for root depth???\n");
    abort();
  }
  format.bigEndian = (screenInfo.imageByteOrder == MSBFirst);

  VisualPtr vis;
  for (i = 0; i < pScreen->numVisuals; i++) {
    if (pScreen->visuals[i].vid == pScreen->rootVisual) {
      vis = &pScreen->visuals[i];
      break;
    }
  }
  if (i == pScreen->numVisuals) {
    fprintf(stderr,"no visual rec for root visual???\n");
    abort();
  }
  format.trueColour = (vis->c_class == TrueColor);
  if (!format.trueColour && format.bpp != 8)
    throw rfb::Exception("X server uses unsupported visual");
  format.redShift   = ffs(vis->redMask) - 1;
  format.greenShift = ffs(vis->greenMask) - 1;
  format.blueShift  = ffs(vis->blueMask) - 1;
  format.redMax     = vis->redMask   >> format.redShift;
  format.greenMax   = vis->greenMask >> format.greenShift;
  format.blueMax    = vis->blueMask  >> format.blueShift;

  width_ = pScreen->width;
  height_ = pScreen->height;
  if (fbptr)
    data = (rdr::U8*)fbptr;
  else
    data = new rdr::U8[pScreen->width * pScreen->height * (format.bpp/8)];
  colourmap = this;

  serverReset(pScreen);

  server = new VNCServerST(name, this);
  server->setPixelBuffer(this);
  server->setQueryConnectionHandler(this);

  if (httpListener)
    httpServer = new FileHTTPServer(this);
}

XserverDesktop::~XserverDesktop()
{
  if (!directFbptr)
    delete [] data;
  TimerFree(deferredUpdateTimer);
  TimerFree(dummyTimer);
  delete httpServer;
  delete server;
}

void XserverDesktop::serverReset(ScreenPtr pScreen_)
{
  pScreen = pScreen_;
  XID* ids = new XID[pScreen->maxInstalledCmaps];
  int nmaps = (*pScreen->ListInstalledColormaps)(pScreen, ids);
  cmap = (ColormapPtr)LookupIDByType(ids[0], RT_COLORMAP);
  delete [] ids;
}

char* XserverDesktop::substitute(const char* varName)
{
  if (strcmp(varName, "$$") == 0) {
    return rfb::strDup("$");
  }
  if (strcmp(varName, "$PORT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", listener ? listener->getMyPort() : 0);
    return str;
  }
  if (strcmp(varName, "$WIDTH") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", width());
    return str;
  }
  if (strcmp(varName, "$HEIGHT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", height());
    return str;
  }
  if (strcmp(varName, "$APPLETWIDTH") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", width());
    return str;
  }
  if (strcmp(varName, "$APPLETHEIGHT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", height() + 32);
    return str;
  }
  if (strcmp(varName, "$DESKTOP") == 0) {
    return rfb::strDup(server->getName());
  }
  if (strcmp(varName, "$DISPLAY") == 0) {
    struct utsname uts;
    uname(&uts);
    char* str = new char[256];
    strncat(str, uts.nodename, 240);
    strcat(str, ":");
    strncat(str, display, 10);
    return str;
  }
  if (strcmp(varName, "$USER") == 0) {
    struct passwd* user = getpwuid(getuid());
    return rfb::strDup(user ? user->pw_name : "?");
  }
  return 0;
}

rfb::VNCServerST::queryResult
XserverDesktop::queryConnection(network::Socket* sock,
                                const char* userName,
                                char** reason) {
  if (queryConnectId) {
    *reason = strDup("Another connection is currently being queried.");
    return rfb::VNCServerST::REJECT;
  }
  queryConnectAddress.replaceBuf(sock->getPeerAddress());
  if (!userName)
    userName = "(anonymous)";
  queryConnectUsername.replaceBuf(strDup(userName));
  queryConnectId = sock;
  vncQueryConnect(this, sock);
  return rfb::VNCServerST::PENDING;
}


void XserverDesktop::setColormap(ColormapPtr cmap_)
{
  if (cmap != cmap_) {
    cmap = cmap_;
    setColourMapEntries(0, 0);
  }
}

void XserverDesktop::setColourMapEntries(ColormapPtr pColormap, int ndef,
                                         xColorItem* pdef)
{
  if (cmap != pColormap || ndef <= 0) return;

  int first = pdef[0].pixel;
  int n = 1;

  for (int i = 1; i < ndef; i++) {
    if (first + n == pdef[i].pixel) {
      n++;
    } else {
      setColourMapEntries(first, n);
      first = pdef[i].pixel;
      n = 1;
    }
  }
  setColourMapEntries(first, n);
}

void XserverDesktop::setColourMapEntries(int firstColour, int nColours)
{
  try {
    server->setColourMapEntries(firstColour, nColours);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::setColourMapEntries: %s",e.str());
  }
}

void XserverDesktop::bell()
{
  server->bell();
}

void XserverDesktop::serverCutText(const char* str, int len)
{
  try {
    server->serverCutText(str, len);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::serverCutText: %s",e.str());
  }
}

void XserverDesktop::setCursor(CursorPtr cursor)
{
  try {
    int w = cursor->bits->width;
    int h = cursor->bits->height;
    rdr::U8* cursorData = new rdr::U8[w * h * (getPF().bpp / 8)];

    xColorItem fg, bg;
    fg.red   = cursor->foreRed;
    fg.green = cursor->foreGreen;
    fg.blue  = cursor->foreBlue;
    FakeAllocColor(cmap, &fg);
    bg.red   = cursor->backRed;
    bg.green = cursor->backGreen;
    bg.blue  = cursor->backBlue;
    FakeAllocColor(cmap, &bg);
    FakeFreeColor(cmap, fg.pixel);
    FakeFreeColor(cmap, bg.pixel);

    int xMaskBytesPerRow = BitmapBytePad(w);

    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int byte = y * xMaskBytesPerRow + x / 8;
#if (BITMAP_BIT_ORDER == MSBFirst)
        int bit = 7 - x % 8;
#else
        int bit = x % 8;
#endif
        switch (getPF().bpp) {
        case 8:
          ((rdr::U8*)cursorData)[y * w + x]
            = (cursor->bits->source[byte] & (1 << bit)) ? fg.pixel : bg.pixel;
          break;
        case 16:
          ((rdr::U16*)cursorData)[y * w + x]
            = (cursor->bits->source[byte] & (1 << bit)) ? fg.pixel : bg.pixel;
          break;
        case 32:
          ((rdr::U32*)cursorData)[y * w + x]
            = (cursor->bits->source[byte] & (1 << bit)) ? fg.pixel : bg.pixel;
          break;
        }
      }
    }

    int rfbMaskBytesPerRow = (w + 7) / 8;

    rdr::U8* cursorMask = new rdr::U8[rfbMaskBytesPerRow * h];

    for (int j = 0; j < h; j++) {
      for (int i = 0; i < rfbMaskBytesPerRow; i++)
#if (BITMAP_BIT_ORDER == MSBFirst)
        cursorMask[j * rfbMaskBytesPerRow + i]
          = cursor->bits->mask[j * xMaskBytesPerRow + i];
#else
        cursorMask[j * rfbMaskBytesPerRow + i]
          = reverseBits[cursor->bits->mask[j * xMaskBytesPerRow + i]];
#endif
    }

    server->setCursor(cursor->bits->width, cursor->bits->height,
                      Point(cursor->bits->xhot, cursor->bits->yhot),
                      cursorData, cursorMask);
    server->tryUpdate();
    delete [] cursorData;
    delete [] cursorMask;
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::setCursor: %s",e.str());
  }
}

static void printRegion(RegionPtr reg)
{
  int nrects = REGION_NUM_RECTS(reg);

  fprintf(stderr,"Region num rects %2d extents %3d,%3d %3dx%3d\n",nrects,
          (REGION_EXTENTS(pScreen,reg))->x1,
          (REGION_EXTENTS(pScreen,reg))->y1,
          (REGION_EXTENTS(pScreen,reg))->x2-(REGION_EXTENTS(pScreen,reg))->x1,
          (REGION_EXTENTS(pScreen,reg))->y2-(REGION_EXTENTS(pScreen,reg))->y1);

  for (int i = 0; i < nrects; i++) {
    fprintf(stderr,"    rect %3d,%3d %3dx%3d\n",
            REGION_RECTS(reg)[i].x1,
            REGION_RECTS(reg)[i].y1,
            REGION_RECTS(reg)[i].x2-REGION_RECTS(reg)[i].x1,
            REGION_RECTS(reg)[i].y2-REGION_RECTS(reg)[i].y1);
  }
}

CARD32 XserverDesktop::deferredUpdateTimerCallback(OsTimerPtr timer,
                                                   CARD32 now, pointer arg)
{
  XserverDesktop* desktop = (XserverDesktop*)arg;
  desktop->deferredUpdateTimerSet = false;
  try {
    desktop->server->tryUpdate();
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::deferredUpdateTimerCallback: %s",e.str());
  }
  return 0;
}

void XserverDesktop::deferUpdate()
{
  if (deferUpdateTime != 0) {
    if (!deferredUpdateTimerSet || alwaysSetDeferUpdateTimer) {
      deferredUpdateTimerSet = true;
      deferredUpdateTimer = TimerSet(deferredUpdateTimer, 0,
                                     deferUpdateTime,
                                     deferredUpdateTimerCallback, this);
    }
  } else {
    server->tryUpdate();
  }
}

void XserverDesktop::add_changed(RegionPtr reg)
{
  if (ignoreHooks_) return;
  if (grabbing) return;
  try {
    rfb::Region rfbReg;
    rfbReg.setExtentsAndOrderedRects((ShortRect*)REGION_EXTENTS(pScreen, reg),
                                     REGION_NUM_RECTS(reg),
                                     (ShortRect*)REGION_RECTS(reg));
    server->add_changed(rfbReg);
    deferUpdate();
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::add_changed: %s",e.str());
  }
}

void XserverDesktop::add_copied(RegionPtr dst, int dx, int dy)
{
  if (ignoreHooks_) return;
  if (grabbing) return;
  try {
    rfb::Region rfbReg;
    rfbReg.setExtentsAndOrderedRects((ShortRect*)REGION_EXTENTS(pScreen, dst),
                                     REGION_NUM_RECTS(dst),
                                     (ShortRect*)REGION_RECTS(dst));
    server->add_copied(rfbReg, rfb::Point(dx, dy));
    deferUpdate();
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::add_copied: %s",e.str());
  }
}

void XserverDesktop::positionCursor()
{
  if (!cursorPos.equals(oldCursorPos)) {
    oldCursorPos = cursorPos;
    (*pScreen->SetCursorPosition) (pScreen, cursorPos.x, cursorPos.y, FALSE);
    server->setCursorPos(cursorPos);
    server->tryUpdate();
  }
}

void XserverDesktop::blockHandler(fd_set* fds)
{
  try {
    ScreenPtr screenWithCursor = GetCurrentRootWindow()->drawable.pScreen;
    if (screenWithCursor == pScreen) {
      int x, y;
      GetSpritePosition(&x, &y);
      if (x != cursorPos.x || y != cursorPos.y) {
        cursorPos = oldCursorPos = Point(x, y);
        server->setCursorPos(cursorPos);
        server->tryUpdate();
      }
    }

    if (listener)
      FD_SET(listener->getFd(), fds);
    if (httpListener)
      FD_SET(httpListener->getFd(), fds);

    std::list<Socket*> sockets;
    server->getSockets(&sockets);
    std::list<Socket*>::iterator i;
    for (i = sockets.begin(); i != sockets.end(); i++) {
      int fd = (*i)->getFd();
      if ((*i)->isShutdown()) {
        vlog.debug("client gone, sock %d",fd);
        server->removeSocket(*i);
        vncClientGone(fd);
        delete (*i);
      } else {
        FD_SET(fd, fds);
      }
    }
    if (httpServer) {
      httpServer->getSockets(&sockets);
      for (i = sockets.begin(); i != sockets.end(); i++) {
        int fd = (*i)->getFd();
        if ((*i)->isShutdown()) {
          vlog.debug("http client gone, sock %d",fd);
          httpServer->removeSocket(*i);
          delete (*i);
        } else {
          FD_SET(fd, fds);
        }
      }
    }
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::blockHandler: %s",e.str());
  }
}

static CARD32 dummyTimerCallback(OsTimerPtr timer, CARD32 now, pointer arg) {
  return 0;
}

void XserverDesktop::wakeupHandler(fd_set* fds, int nfds)
{
  try {
    if (nfds >= 1) {

      if (listener) {
        if (FD_ISSET(listener->getFd(), fds)) {
          FD_CLR(listener->getFd(), fds);
          Socket* sock = listener->accept();
          server->addSocket(sock);
          vlog.debug("new client, sock %d",sock->getFd());
        }
      }

      if (httpListener) {
        if (FD_ISSET(httpListener->getFd(), fds)) {
          FD_CLR(httpListener->getFd(), fds);
          Socket* sock = httpListener->accept();
          httpServer->addSocket(sock);
          vlog.debug("new http client, sock %d",sock->getFd());
        }
      }

      std::list<Socket*> sockets;
      server->getSockets(&sockets);
      std::list<Socket*>::iterator i;
      for (i = sockets.begin(); i != sockets.end(); i++) {
        int fd = (*i)->getFd();
        if (FD_ISSET(fd, fds)) {
          FD_CLR(fd, fds);
          server->processSocketEvent(*i);
        }
      }

      if (httpServer) {
        httpServer->getSockets(&sockets);
        for (i = sockets.begin(); i != sockets.end(); i++) {
          int fd = (*i)->getFd();
          if (FD_ISSET(fd, fds)) {
            FD_CLR(fd, fds);
            httpServer->processSocketEvent(*i);
          }
        }
      }

      positionCursor();
    }

    int timeout = server->checkTimeouts();
    if (timeout > 0) {
      // set a dummy timer just so we are guaranteed be called again next time.
      dummyTimer = TimerSet(dummyTimer, 0, timeout,
                            dummyTimerCallback, 0);
    }

  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::wakeupHandler: %s",e.str());
  }
}

void XserverDesktop::addClient(Socket* sock, bool reverse)
{
  vlog.debug("new client, sock %d reverse %d",sock->getFd(),reverse);
  server->addSocket(sock, reverse);
}

void XserverDesktop::disconnectClients()
{
  vlog.debug("disconnecting all clients");
  return server->closeClients("Disconnection from server end");
}


int XserverDesktop::getQueryTimeout(void* opaqueId,
                                    const char** address,
                                    const char** username)
{
  if (opaqueId && queryConnectId == opaqueId) {
    vlog.info("address=%s, username=%s, timeout=%d",
              queryConnectAddress.buf, queryConnectUsername.buf,
              (int)queryConnectTimeout);
    if (address) *address = queryConnectAddress.buf;
    if (username) *username = queryConnectUsername.buf;
    return queryConnectTimeout;
  }
  return 0;
}

void XserverDesktop::approveConnection(void* opaqueId, bool accept,
                                       const char* rejectMsg)
{
  if (queryConnectId == opaqueId) {
    server->approveConnection((network::Socket*)opaqueId, accept, rejectMsg);
    queryConnectId = 0;
  }
}

///////////////////////////////////////////////////////////////////////////
//
// SDesktop callbacks


void XserverDesktop::pointerEvent(const Point& pos, int buttonMask)
{
  xEvent ev;
  DevicePtr dev = LookupPointerDevice();

  // SetCursorPosition seems to be very expensive (at least on XFree86 3.3.6
  // for S3), so we delay calling it until positionCursor() is called at the
  // end of processing a load of RFB.
  //(*pScreen->SetCursorPosition) (pScreen, pos.x, pos.y, FALSE);

  NewCurrentScreen(pScreen, pos.x, pos.y);

  ev.u.u.type = MotionNotify;
  ev.u.u.detail = 0;
  ev.u.keyButtonPointer.rootX = pos.x;
  ev.u.keyButtonPointer.rootY = pos.y;
  ev.u.keyButtonPointer.time = GetTimeInMillis();

  if (!pos.equals(cursorPos))
    (*dev->processInputProc)(&ev, (DeviceIntPtr)dev, 1);

  for (int i = 0; i < 5; i++) {
    if ((buttonMask ^ oldButtonMask) & (1<<i)) {
#ifdef XINPUT
      // God knows why but some idiot decided to conditionally move the pointer
      // mapping out of DIX, so we guess here that if XINPUT is defined we have
      // to do it ourselves...
      ev.u.u.detail = ((DeviceIntPtr)dev)->button->map[i + 1];
#else
      ev.u.u.detail = i + 1;
#endif
      ev.u.u.type = (buttonMask & (1<<i)) ? ButtonPress : ButtonRelease;
      (*dev->processInputProc)(&ev, (DeviceIntPtr)dev, 1);
    }
  }

  cursorPos = pos;
  oldButtonMask = buttonMask;
}

void XserverDesktop::clientCutText(const char* str, int len)
{
  vncClientCutText(str, len);
}

void XserverDesktop::grabRegion(const rfb::Region& region)
{
  if (directFbptr) return;
  if (!pScreen->GetImage) {
    vlog.error("VNC error: pScreen->GetImage == 0");
    return;
  }

  grabbing = true;

  int bytesPerPixel = format.bpp/8;
  int bytesPerRow = pScreen->width * bytesPerPixel;

  std::vector<rfb::Rect> rects;
  std::vector<rfb::Rect>::iterator i;
  region.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    for (int y = i->tl.y; y < i->br.y; y++) {
      (*pScreen->GetImage) ((DrawablePtr)WindowTable[pScreen->myNum],
                            i->tl.x, y, i->width(), 1,
                            ZPixmap, (unsigned long)~0L,
                            ((char*)data
                             + y * bytesPerRow + i->tl.x * bytesPerPixel));
    }
  }
  grabbing = false;
}

void XserverDesktop::lookup(int index, int* r, int* g, int* b)
{
  if ((cmap->c_class | DynamicClass) == DirectColor) {
    VisualPtr v = cmap->pVisual;
    *r = cmap->red  [(index & v->redMask  ) >> v->offsetRed  ].co.local.red;
    *g = cmap->green[(index & v->greenMask) >> v->offsetGreen].co.local.green;
    *b = cmap->blue [(index & v->blueMask ) >> v->offsetBlue ].co.local.blue;
  } else {
    EntryPtr pent;
    pent = (EntryPtr)&cmap->red[index];
    if (pent->fShared) {
      *r = pent->co.shco.red->color;
      *g = pent->co.shco.green->color;
      *b = pent->co.shco.blue->color;
    } else {
      *r = pent->co.local.red;
      *g = pent->co.local.green;
      *b = pent->co.local.blue;
    }
  }
}

//
// Keyboard handling
//

#define IS_PRESSED(keyc, keycode) \
  ((keyc)->down[(keycode) >> 3] & (1 << ((keycode) & 7)))

// ModifierState is a class which helps simplify generating a "fake" press
// or release of shift, ctrl, alt, etc.  An instance of the class is created
// for every modifier which may need to be pressed or released.  Then either
// press() or release() may be called to make sure that the corresponding keys
// are in the right state.  The destructor of the class automatically reverts
// to the previous state.  Each modifier may have multiple keys associated with
// it, so in the case of a fake release, this may involve releasing more than
// one key.

class ModifierState {
public:
  ModifierState(DeviceIntPtr dev_, int modIndex_)
    : dev(dev_), modIndex(modIndex_), nKeys(0), keys(0), pressed(false)
  {
  }
  ~ModifierState() {
    for (int i = 0; i < nKeys; i++)
      generateXKeyEvent(keys[i], !pressed);
    delete [] keys;
  }
  void press() {
    KeyClassPtr keyc = dev->key;
    if (!(keyc->state & (1<<modIndex))) {
      tempKeyEvent(keyc->modifierKeyMap[modIndex * keyc->maxKeysPerModifier],
                   true);
      pressed = true;
    }
  }
  void release() {
    KeyClassPtr keyc = dev->key;
    if (keyc->state & (1<<modIndex)) {
      for (int k = 0; k < keyc->maxKeysPerModifier; k++) {
        int keycode
          = keyc->modifierKeyMap[modIndex * keyc->maxKeysPerModifier + k];
        if (keycode && IS_PRESSED(keyc, keycode))
          tempKeyEvent(keycode, false);
      }
    }
  }
private:
  void tempKeyEvent(int keycode, bool down) {
    if (keycode) {
      if (!keys) keys = new int[dev->key->maxKeysPerModifier];
      keys[nKeys++] = keycode;
      generateXKeyEvent(keycode, down);
    }
  }
  void generateXKeyEvent(int keycode, bool down) {
    xEvent ev;
    ev.u.u.type = down ? KeyPress : KeyRelease;
    ev.u.u.detail = keycode;
    ev.u.keyButtonPointer.time = GetTimeInMillis();
    (*dev->c_public.processInputProc)(&ev, dev, 1);
    vlog.debug("fake keycode %d %s", keycode, down ? "down" : "up");
  }
  DeviceIntPtr dev;
  int modIndex;
  int nKeys;
  int* keys;
  bool pressed;
};


// altKeysym is a table of alternative keysyms which have the same meaning.

struct altKeysym_t {
  KeySym a, b;
};

altKeysym_t altKeysym[] = {
  { XK_Shift_L,        XK_Shift_R },
  { XK_Control_L,      XK_Control_R },
  { XK_Meta_L,         XK_Meta_R },
  { XK_Alt_L,          XK_Alt_R },
  { XK_Super_L,        XK_Super_R },
  { XK_Hyper_L,        XK_Hyper_R },
  { XK_KP_Space,       XK_space },
  { XK_KP_Tab,         XK_Tab },
  { XK_KP_Enter,       XK_Return },
  { XK_KP_F1,          XK_F1 },
  { XK_KP_F2,          XK_F2 },
  { XK_KP_F3,          XK_F3 },
  { XK_KP_F4,          XK_F4 },
  { XK_KP_Home,        XK_Home },
  { XK_KP_Left,        XK_Left },
  { XK_KP_Up,          XK_Up },
  { XK_KP_Right,       XK_Right },
  { XK_KP_Down,        XK_Down },
  { XK_KP_Page_Up,     XK_Page_Up },
  { XK_KP_Page_Down,   XK_Page_Down },
  { XK_KP_End,         XK_End },
  { XK_KP_Begin,       XK_Begin },
  { XK_KP_Insert,      XK_Insert },
  { XK_KP_Delete,      XK_Delete },
  { XK_KP_Equal,       XK_equal },
  { XK_KP_Multiply,    XK_asterisk },
  { XK_KP_Add,         XK_plus },
  { XK_KP_Separator,   XK_comma },
  { XK_KP_Subtract,    XK_minus },
  { XK_KP_Decimal,     XK_period },
  { XK_KP_Divide,      XK_slash },
  { XK_KP_0,           XK_0 },
  { XK_KP_1,           XK_1 },
  { XK_KP_2,           XK_2 },
  { XK_KP_3,           XK_3 },
  { XK_KP_4,           XK_4 },
  { XK_KP_5,           XK_5 },
  { XK_KP_6,           XK_6 },
  { XK_KP_7,           XK_7 },
  { XK_KP_8,           XK_8 },
  { XK_KP_9,           XK_9 },
};

// keyEvent() - work out the best keycode corresponding to the keysym sent by
// the viewer.  This is non-trivial because we can't assume much about the
// local keyboard layout.  We must also find out which column of the keyboard
// mapping the keysym is in, and alter the shift state appropriately.  Column 0
// means both shift and "mode_switch" (AltGr) must be released, column 1 means
// shift must be pressed and mode_switch released, column 2 means shift must be
// released and mode_switch pressed, and column 3 means both shift and
// mode_switch must be pressed.

void XserverDesktop::keyEvent(rdr::U32 keysym, bool down)
{
  if (keysym == XK_Caps_Lock) {
    vlog.debug("Ignoring caps lock");
    return;
  }
  DeviceIntPtr dev = (DeviceIntPtr)LookupKeyboardDevice();
  KeyClassPtr keyc = dev->key;
  KeySymsPtr keymap = &keyc->curKeySyms;

  // find which modifier Mode_switch is on.
  int modeSwitchMapIndex = 0;
  for (int i = 3; i < 8; i++) {
    for (int k = 0; k < keyc->maxKeysPerModifier; k++) {
      int keycode = keyc->modifierKeyMap[i * keyc->maxKeysPerModifier + k];
      for (int j = 0; j < keymap->mapWidth; j++) {
        if (keycode != 0 &&
            keymap->map[(keycode - keymap->minKeyCode)
                        * keymap->mapWidth + j] == XK_Mode_switch)
        {
          modeSwitchMapIndex = i;
          break;
        }
      }
    }
  }

  int col = 0;
  if (keyc->state & (1<<ShiftMapIndex)) col |= 1;
  if (modeSwitchMapIndex && (keyc->state & (1<<modeSwitchMapIndex))) col |= 2;

  int kc = KeysymToKeycode(keymap, keysym, &col);

  // Sort out the "shifted Tab" mess.  If we are sent a shifted Tab, generate a
  // local shifted Tab regardless of what the "shifted Tab" keysym is on the
  // local keyboard (it might be Tab, ISO_Left_Tab or HP's private BackTab
  // keysym, and quite possibly some others too).  We never get ISO_Left_Tab
  // here because it's already been translated in VNCSConnectionST.
  if (keysym == XK_Tab && (keyc->state & (1<<ShiftMapIndex)))
    col |= 1;

  if (kc == 0) {
    // Not a direct match in the local keyboard mapping.  Check for alternative
    // keysyms with the same meaning.
    for (int i = 0; i < sizeof(altKeysym) / sizeof(altKeysym_t); i++) {
      if (keysym == altKeysym[i].a)
        kc = KeysymToKeycode(keymap, altKeysym[i].b, &col);
      else if (keysym == altKeysym[i].b)
        kc = KeysymToKeycode(keymap, altKeysym[i].a, &col);
      if (kc) break;
    }
  }

  if (kc == 0) {
    // Last resort - dynamically add a new key to the keyboard mapping.
    for (kc = keymap->maxKeyCode; kc >= keymap->minKeyCode; kc--) {
      if (!keymap->map[(kc - keymap->minKeyCode) * keymap->mapWidth]) {
        keymap->map[(kc - keymap->minKeyCode) * keymap->mapWidth] = keysym;
        col = 0;
        SendMappingNotify(MappingKeyboard, kc, 1, serverClient);
        vlog.info("Added unknown keysym 0x%x to keycode %d",keysym,kc);
        break;
      }
    }
    if (kc < keymap->minKeyCode) {
      vlog.info("Keyboard mapping full - ignoring unknown keysym 0x%x",keysym);
      return;
    }
  }

  // See if it's a modifier key.  If so, then don't do any auto-repeat, because
  // the X server will translate each press into a release followed by a press.
  for (int i = 0; i < 8; i++) {
    for (int k = 0; k < keyc->maxKeysPerModifier; k++) {
      if (kc == keyc->modifierKeyMap[i * keyc->maxKeysPerModifier + k] &&
          IS_PRESSED(keyc,kc) && down)
        return;
    }
  }

  ModifierState shift(dev, ShiftMapIndex);
  ModifierState modeSwitch(dev, modeSwitchMapIndex);
  if (down) {
    if (col & 1)
      shift.press();
    else
      shift.release();
    if (modeSwitchMapIndex) {
      if (col & 2)
        modeSwitch.press();
      else
        modeSwitch.release();
    }
  }
  vlog.debug("keycode %d %s", kc, down ? "down" : "up");
  xEvent ev;
  ev.u.u.type = down ? KeyPress : KeyRelease;
  ev.u.u.detail = kc;
  ev.u.keyButtonPointer.time = GetTimeInMillis();
  (*dev->c_public.processInputProc)(&ev, dev, 1);
}


void XConvertCase(KeySym sym, KeySym *lower, KeySym *upper)
{
    *lower = sym;
    *upper = sym;
    switch(sym >> 8) {
    case 0: /* Latin 1 */
	if ((sym >= XK_A) && (sym <= XK_Z))
	    *lower += (XK_a - XK_A);
	else if ((sym >= XK_a) && (sym <= XK_z))
	    *upper -= (XK_a - XK_A);
	else if ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis))
	    *lower += (XK_agrave - XK_Agrave);
	else if ((sym >= XK_agrave) && (sym <= XK_odiaeresis))
	    *upper -= (XK_agrave - XK_Agrave);
	else if ((sym >= XK_Ooblique) && (sym <= XK_Thorn))
	    *lower += (XK_oslash - XK_Ooblique);
	else if ((sym >= XK_oslash) && (sym <= XK_thorn))
	    *upper -= (XK_oslash - XK_Ooblique);
	break;
    case 1: /* Latin 2 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym == XK_Aogonek)
	    *lower = XK_aogonek;
	else if (sym >= XK_Lstroke && sym <= XK_Sacute)
	    *lower += (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_Scaron && sym <= XK_Zacute)
	    *lower += (XK_scaron - XK_Scaron);
	else if (sym >= XK_Zcaron && sym <= XK_Zabovedot)
	    *lower += (XK_zcaron - XK_Zcaron);
	else if (sym == XK_aogonek)
	    *upper = XK_Aogonek;
	else if (sym >= XK_lstroke && sym <= XK_sacute)
	    *upper -= (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_scaron && sym <= XK_zacute)
	    *upper -= (XK_scaron - XK_Scaron);
	else if (sym >= XK_zcaron && sym <= XK_zabovedot)
	    *upper -= (XK_zcaron - XK_Zcaron);
	else if (sym >= XK_Racute && sym <= XK_Tcedilla)
	    *lower += (XK_racute - XK_Racute);
	else if (sym >= XK_racute && sym <= XK_tcedilla)
	    *upper -= (XK_racute - XK_Racute);
	break;
    case 2: /* Latin 3 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Hstroke && sym <= XK_Hcircumflex)
	    *lower += (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_Gbreve && sym <= XK_Jcircumflex)
	    *lower += (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_hstroke && sym <= XK_hcircumflex)
	    *upper -= (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_gbreve && sym <= XK_jcircumflex)
	    *upper -= (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_Cabovedot && sym <= XK_Scircumflex)
	    *lower += (XK_cabovedot - XK_Cabovedot);
	else if (sym >= XK_cabovedot && sym <= XK_scircumflex)
	    *upper -= (XK_cabovedot - XK_Cabovedot);
	break;
    case 3: /* Latin 4 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Rcedilla && sym <= XK_Tslash)
	    *lower += (XK_rcedilla - XK_Rcedilla);
	else if (sym >= XK_rcedilla && sym <= XK_tslash)
	    *upper -= (XK_rcedilla - XK_Rcedilla);
	else if (sym == XK_ENG)
	    *lower = XK_eng;
	else if (sym == XK_eng)
	    *upper = XK_ENG;
	else if (sym >= XK_Amacron && sym <= XK_Umacron)
	    *lower += (XK_amacron - XK_Amacron);
	else if (sym >= XK_amacron && sym <= XK_umacron)
	    *upper -= (XK_amacron - XK_Amacron);
	break;
    case 6: /* Cyrillic */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Serbian_DJE && sym <= XK_Serbian_DZE)
	    *lower -= (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Serbian_dje && sym <= XK_Serbian_dze)
	    *upper += (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Cyrillic_YU && sym <= XK_Cyrillic_HARDSIGN)
	    *lower -= (XK_Cyrillic_YU - XK_Cyrillic_yu);
	else if (sym >= XK_Cyrillic_yu && sym <= XK_Cyrillic_hardsign)
	    *upper += (XK_Cyrillic_YU - XK_Cyrillic_yu);
        break;
    case 7: /* Greek */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Greek_ALPHAaccent && sym <= XK_Greek_OMEGAaccent)
	    *lower += (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_alphaaccent && sym <= XK_Greek_omegaaccent &&
		 sym != XK_Greek_iotaaccentdieresis &&
		 sym != XK_Greek_upsilonaccentdieresis)
	    *upper -= (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_ALPHA && sym <= XK_Greek_OMEGA)
	    *lower += (XK_Greek_alpha - XK_Greek_ALPHA);
	else if (sym >= XK_Greek_alpha && sym <= XK_Greek_omega &&
		 sym != XK_Greek_finalsmallsigma)
	    *upper -= (XK_Greek_alpha - XK_Greek_ALPHA);
        break;
    }
}

static KeySym KeyCodetoKeySym(KeySymsPtr keymap, int keycode, int col)
{
  register int per = keymap->mapWidth;
  register KeySym *syms;
  KeySym lsym, usym;

  if ((col < 0) || ((col >= per) && (col > 3)) ||
      (keycode < keymap->minKeyCode) || (keycode > keymap->maxKeyCode))
    return NoSymbol;

  syms = &keymap->map[(keycode - keymap->minKeyCode) * per];
  if (col < 4) {
    if (col > 1) {
      while ((per > 2) && (syms[per - 1] == NoSymbol))
        per--;
      if (per < 3)
        col -= 2;
    }
    if ((per <= (col|1)) || (syms[col|1] == NoSymbol)) {
      XConvertCase(syms[col&~1], &lsym, &usym);
      if (!(col & 1))
        return lsym;
      // I'm commenting out this logic because it's incorrect even though it
      // was copied from the Xlib sources.  The X protocol book quite clearly
      // states that where a group consists of element 1 being a non-alphabetic
      // keysym and element 2 being NoSymbol that you treat the second element
      // as being the same as the first.  This also tallies with the behaviour
      // produced by the installed Xlib on my linux box (I believe this is
      // because it uses some XKB code rather than the original Xlib code -
      // compare XKBBind.c with KeyBind.c in lib/X11).
      // else if (usym == lsym)
      //   return NoSymbol;
      else
        return usym;
    }
  }
  return syms[col];
}

// KeysymToKeycode() - find the keycode and column corresponding to the given
// keysym.  The value of col passed in should be the column determined from the
// current shift state.  If the keysym can be found in that column we prefer
// that to finding it in a different column (which would require fake events to
// alter the shift state).

static KeyCode KeysymToKeycode(KeySymsPtr keymap, KeySym ks, int* col)
{
  register int i, j;

  j = *col;
  for (i = keymap->minKeyCode; i <= keymap->maxKeyCode; i++) {
    if (KeyCodetoKeySym(keymap, i, j) == ks)
      return i;
  }

  for (j = 0; j < keymap->mapWidth; j++) {
    for (i = keymap->minKeyCode; i <= keymap->maxKeyCode; i++) {
      if (KeyCodetoKeySym(keymap, i, j) == ks) {
        *col = j;
        return i;
      }
    }
  }
  return 0;
}
