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
// DesktopWindow is a TXWindow representing a VNC desktop.
//

#ifndef __DESKTOPWINDOW_H__
#define __DESKTOPWINDOW_H__

#include <rfb/Cursor.h>
#include <rfb/Rect.h>
#include <rfb/Timer.h>
#include "TXWindow.h"
#include "TXViewport.h"
#include "TXImage.h"

class CConn;

class DesktopWindow : public TXWindow, public TXEventHandler,
                      public rfb::Timer::Callback {
public:

  DesktopWindow(Display* dpy, int w, int h,
                const rfb::PixelFormat& serverPF, CConn* cc_,
                TXWindow* parent=0);
  ~DesktopWindow();

  void setViewport(TXViewport* viewport);

  // getPF() and setPF() get and set the TXImage's pixel format
  const rfb::PixelFormat& getPF() { return im->getPF(); }
  void setPF(const rfb::PixelFormat& pf) { im->setPF(pf); }

  // setCursor() sets the shape of the local cursor
  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

  // resetLocalCursor() stops the rendering of the local cursor
  void resetLocalCursor();

  // Methods forwarded from CConn
  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);
  void serverCutText(const char* str, int len);
  void framebufferUpdateEnd();

  void fillRect(const rfb::Rect& r, rfb::Pixel pix) {
    if (r.overlaps(cursorBackingRect)) hideLocalCursor();
    im->fillRect(r, pix);
    im->put(win(), gc, r);
    showLocalCursor();
  }
  void imageRect(const rfb::Rect& r, void* pixels) {
    if (r.overlaps(cursorBackingRect)) hideLocalCursor();
    im->imageRect(r, pixels);
    im->put(win(), gc, r);
    showLocalCursor();
  }
  void copyRect(const rfb::Rect& r, int srcX, int srcY) {
    if (r.overlaps(cursorBackingRect) ||
        cursorBackingRect.overlaps(rfb::Rect(srcX, srcY,
                                             srcX+r.width(), srcY+r.height())))
      hideLocalCursor();
    if (im->usingShm())
      XSync(dpy, False);
    im->copyRect(r, rfb::Point(r.tl.x-srcX, r.tl.y-srcY));
    XCopyArea(dpy, win(), win(), gc, srcX, srcY,
              r.width(), r.height(), r.tl.x, r.tl.y);
    showLocalCursor();
  }
  void invertRect(const rfb::Rect& r);

  // TXWindow methods
  virtual void resize(int w, int h);
  virtual bool selectionRequest(Window requestor,
                                Atom selection, Atom property);
  virtual void selectionNotify(XSelectionEvent* ev, Atom type, int format,
                               int nitems, void* data);
  virtual void handleEvent(TXWindow* w, XEvent* ev);

private:

  void createXCursors();
  void hideLocalCursor();
  void showLocalCursor();
  bool handleTimeout(rfb::Timer* timer);
  void handlePointerEvent(const rfb::Point& pos, int buttonMask);

  CConn* cc;
  TXImage* im;
  GC gc;
  ::Cursor dotCursor, noCursor, localXCursor;

  rfb::Cursor cursor;
  bool cursorVisible;     // Is cursor currently rendered?
  bool cursorAvailable;   // Is cursor available for rendering?
  rfb::Point cursorPos;
  rfb::ManagedPixelBuffer cursorBacking;
  rfb::Rect cursorBackingRect;

  Time currentSelectionTime;
  Atom newSelection;
  bool gettingInitialSelectionTime;
  bool newServerCutText;
  char* serverCutText_;

  rfb::Timer setColourMapEntriesTimer;
  TXViewport* viewport;
  rfb::Timer pointerEventTimer;
  rfb::Point lastPointerPos;
  int lastButtonMask;
  rdr::U32 downKeysym[256];
};

#endif
