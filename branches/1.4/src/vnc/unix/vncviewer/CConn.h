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
// CConn represents a client connection to a VNC server.
//

#ifndef __CCONN_H__
#define __CCONN_H__

#include <rfb/CConnection.h>
#include <rfb/Exception.h>
#include <rfb/UserPasswdGetter.h>
#include <rdr/FdInStream.h>
#include <list>

#include "TXWindow.h"
#include "AboutDialog.h"
#include "InfoDialog.h"
#include "TXMenu.h"
#include "OptionsDialog.h"

class TXWindow;
class TXViewport;
class DesktopWindow;
namespace network { class Socket; }

class CConn : public rfb::CConnection, public rfb::UserPasswdGetter,
              public TXDeleteWindowCallback,
              public rdr::FdInStreamBlockCallback,
              public TXMenuCallback , public OptionsDialogCallback,
              public TXEventHandler
{
public:

  CConn(Display* dpy_, int argc_, char** argv_, network::Socket* sock_,
        char* vncServerName, bool reverse=false);
  ~CConn();

  // TXDeleteWindowCallback methods
  void deleteWindow(TXWindow* w);

  // FdInStreamBlockCallback methods
  void blockCallback();

  // UserPasswdGetter methods
  virtual void getUserPasswd(char** user, char** password);

  // TXMenuCallback methods
  void menuSelect(long id, TXMenu* m);

  // OptionsDialogCallback methods
  virtual void setOptions();
  virtual void getOptions();

  // TXEventHandler callback method
  virtual void handleEvent(TXWindow* w, XEvent* ev);
  
  // CConnection callback methods
  rfb::CSecurity* getCSecurity(int secType);
  void serverInit();
  void setDesktopSize(int w, int h);
  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);
  void bell();
  void serverCutText(const char* str, int len);
  void framebufferUpdateEnd();
  void beginRect(const rfb::Rect& r, unsigned int encoding);
  void endRect(const rfb::Rect& r, unsigned int encoding);
  void fillRect(const rfb::Rect& r, rfb::Pixel p);
  void imageRect(const rfb::Rect& r, void* p);
  void copyRect(const rfb::Rect& r, int sx, int sy);
  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

private:

  void recreateViewport();
  void reconfigureViewport();
  void initMenu();
  void showMenu(int x, int y);
  void autoSelectFormatAndEncoding();
  void checkEncodings();
  void requestNewUpdate();

  Display* dpy;
  int argc;
  char** argv;
  char* serverHost;
  int serverPort;
  network::Socket* sock;
  rfb::PixelFormat serverPF;
  TXViewport* viewport;
  DesktopWindow* desktop;
  TXEventHandler* desktopEventHandler;
  rfb::PixelFormat fullColourPF;
  std::list<rfb::Rect> debugRects;
  unsigned int currentEncoding, lastServerEncoding;
  bool fullColour;
  bool autoSelect;
  bool shared;
  bool formatChange;
  bool encodingChange;
  bool sameMachine;
  bool fullScreen;
  bool ctrlDown;
  bool altDown;
  KeySym menuKeysym;
  TXMenu menu;
  TXEventHandler* menuEventHandler;
  OptionsDialog options;
  AboutDialog about;
  InfoDialog info;
  bool reverseConnection;
};

#endif
