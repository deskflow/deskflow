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
// XserverDesktop.h
//

#ifndef __XSERVERDESKTOP_H__
#define __XSERVERDESKTOP_H__

#include <rfb/SDesktop.h>
#include <rfb/HTTPServer.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Configuration.h>
#include <rfb/VNCServerST.h>
#include <rdr/SubstitutingInStream.h>

extern "C" {
#define class c_class;
#include <scrnintstr.h>
#include <os.h>
#undef class
}

namespace rfb {
  class VNCServerST;
}

namespace network { class TcpListener; class Socket; }

class XserverDesktop : public rfb::SDesktop, public rfb::FullFramePixelBuffer,
                       public rfb::ColourMap, public rdr::Substitutor,
                       public rfb::VNCServerST::QueryConnectionHandler {
public:

  XserverDesktop(ScreenPtr pScreen, network::TcpListener* listener,
                 network::TcpListener* httpListener_,
                 const char* name, void* fbptr);
  virtual ~XserverDesktop();

  // methods called from X server code
  void serverReset(ScreenPtr pScreen);
  void setColormap(ColormapPtr cmap);
  void setColourMapEntries(ColormapPtr pColormap, int ndef, xColorItem* pdef);
  void bell();
  void serverCutText(const char* str, int len);
  void setCursor(CursorPtr cursor);
  void add_changed(RegionPtr reg);
  void add_copied(RegionPtr dst, int dx, int dy);
  void positionCursor();
  void ignoreHooks(bool b) { ignoreHooks_ = b; }
  void blockHandler(fd_set* fds);
  void wakeupHandler(fd_set* fds, int nfds);
  void addClient(network::Socket* sock, bool reverse);
  void disconnectClients();

  // QueryConnect methods called from X server code
  // getQueryTimeout()
  //   Returns the timeout associated with a particular
  //   connection, identified by an opaque Id passed to the
  //   X code earlier.  Also optionally gets the address and
  //   name associated with that connection.
  //   Returns zero if the Id is not recognised.
  int getQueryTimeout(void* opaqueId,
                      const char** address=0,
                      const char** username=0);

  // approveConnection()
  //   Used by X server code to supply the result of a query.
  void approveConnection(void* opaqueId, bool accept,
                         const char* rejectMsg=0);

  // rfb::SDesktop callbacks
  virtual void pointerEvent(const rfb::Point& pos, int buttonMask);
  virtual void keyEvent(rdr::U32 key, bool down);
  virtual void clientCutText(const char* str, int len);
  virtual rfb::Point getFbSize() { return rfb::Point(width(), height()); }

  // rfb::PixelBuffer callbacks
  virtual void grabRegion(const rfb::Region& r);

  // rfb::ColourMap callbacks
  virtual void lookup(int index, int* r, int* g, int* b);

  // rdr::Substitutor callback
  virtual char* substitute(const char* varName);

  // rfb::VNCServerST::QueryConnectionHandler callback
  virtual rfb::VNCServerST::queryResult queryConnection(network::Socket* sock,
                                                        const char* userName,
                                                        char** reason);

private:
  void setColourMapEntries(int firstColour, int nColours);
  static CARD32 deferredUpdateTimerCallback(OsTimerPtr timer, CARD32 now,
                                            pointer arg);
  void deferUpdate();
  ScreenPtr pScreen;
  OsTimerPtr deferredUpdateTimer, dummyTimer;
  rfb::VNCServerST* server;
  rfb::HTTPServer* httpServer;
  network::TcpListener* listener;
  network::TcpListener* httpListener;
  ColormapPtr cmap;
  bool deferredUpdateTimerSet;
  bool grabbing;
  bool ignoreHooks_;
  bool directFbptr;
  int oldButtonMask;
  rfb::Point cursorPos, oldCursorPos;

  void* queryConnectId;
  rfb::CharArray queryConnectAddress;
  rfb::CharArray queryConnectUsername;
};
#endif
