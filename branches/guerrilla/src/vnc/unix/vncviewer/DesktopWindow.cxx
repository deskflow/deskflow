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
// DesktopWindow.cxx
//

#include "DesktopWindow.h"
#include "CConn.h"
#include <rfb/CMsgWriter.h>
#include <rfb/LogWriter.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include "parameters.h"

#ifndef XK_ISO_Left_Tab
#define	XK_ISO_Left_Tab					0xFE20
#endif

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

using namespace rfb;

static rfb::LogWriter vlog("DesktopWindow");

DesktopWindow::DesktopWindow(Display* dpy, int w, int h,
                             const rfb::PixelFormat& serverPF,
                             CConn* cc_, TXWindow* parent)
  : TXWindow(dpy, w, h, parent), cc(cc_), im(0),
    cursorVisible(false), cursorAvailable(false), currentSelectionTime(0),
    newSelection(0), gettingInitialSelectionTime(true),
    newServerCutText(false), serverCutText_(0),
    setColourMapEntriesTimer(this), viewport(0),
    pointerEventTimer(this),
    lastButtonMask(0)
{
  setEventHandler(this);
  gc = XCreateGC(dpy, win(), 0, 0);
  addEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
               PointerMotionMask | KeyPressMask | KeyReleaseMask |
               EnterWindowMask | LeaveWindowMask);
  createXCursors();
  XDefineCursor(dpy, win(), dotCursor);
  im = new TXImage(dpy, width(), height());
  if (!serverPF.trueColour)
    im->setPF(serverPF);
  XConvertSelection(dpy, sendPrimary ? XA_PRIMARY : xaCLIPBOARD, xaTIMESTAMP,
                    xaSELECTION_TIME, win(), CurrentTime);
  memset(downKeysym, 0, 256*4);
}

DesktopWindow::~DesktopWindow()
{
  XFreeGC(dpy, gc);
  XFreeCursor(dpy, dotCursor);
  XFreeCursor(dpy, noCursor);
  if (localXCursor)
    XFreeCursor(dpy, localXCursor);
  delete im;
}

void DesktopWindow::setViewport(TXViewport* viewport_)
{
  viewport = viewport_;
  viewport->setChild(this);
}

// Cursor stuff

void DesktopWindow::createXCursors()
{
  static char dotSource[] = { 0x00, 0x0e, 0x0e, 0x0e, 0x00 };
  static char dotMask[]   = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
  Pixmap source = XCreateBitmapFromData(dpy, win(), dotSource, 5, 5);
  Pixmap mask = XCreateBitmapFromData(dpy, win(), dotMask, 5, 5);
  XColor fg, bg;
  fg.red = fg.green = fg.blue = 0;
  bg.red = bg.green = bg.blue = 0xffff;
  dotCursor = XCreatePixmapCursor(dpy, source, mask, &fg, &bg, 2, 2);
  XFreePixmap(dpy, source);
  XFreePixmap(dpy, mask);
  char zero = 0;
  Pixmap empty = XCreateBitmapFromData(dpy, win(), &zero, 1, 1);
  noCursor = XCreatePixmapCursor(dpy, empty, empty, &fg, &bg, 0, 0);
  XFreePixmap(dpy, empty);
  localXCursor = 0;
}

void DesktopWindow::setCursor(int width, int height, const Point& hotspot,
                              void* data, void* mask)
{
  if (!useLocalCursor) return;

  hideLocalCursor();

  int mask_len = ((width+7)/8) * height;

  int i;
  for (i = 0; i < mask_len; i++)
    if (((rdr::U8*)mask)[i]) break;

  if (i == mask_len) {
    if (dotWhenNoCursor) {
      vlog.debug("cursor is empty - using dot");
      XDefineCursor(dpy, win(), dotCursor);
    } else {
      XDefineCursor(dpy, win(), noCursor);
    }
    cursorAvailable = false;
    return;
  }

  cursor.hotspot = hotspot;

  cursor.setSize(width, height);
  cursor.setPF(getPF());
  cursor.imageRect(cursor.getRect(), data);

  cursorBacking.setSize(width, height);
  cursorBacking.setPF(getPF());

  delete [] cursor.mask.buf;
  cursor.mask.buf = new rdr::U8[mask_len];
  memcpy(cursor.mask.buf, mask, mask_len);

  Pixel pix0, pix1;
  rdr::U8Array bitmap(cursor.getBitmap(&pix0, &pix1));
  if (bitmap.buf && cursor.getRect().contains(cursor.hotspot)) {
    int bytesPerRow = (cursor.width() + 7) / 8;
    for (int j = 0; j < cursor.height(); j++) {
      for (int i = 0; i < bytesPerRow; i++) {
        bitmap.buf[j * bytesPerRow + i]
          = reverseBits[bitmap.buf[j * bytesPerRow + i]];
        cursor.mask.buf[j * bytesPerRow + i]
          = reverseBits[cursor.mask.buf[j * bytesPerRow + i]];
      }
    }
    Pixmap source = XCreateBitmapFromData(dpy, win(), (char*)bitmap.buf,
                                          cursor.width(), cursor.height());
    Pixmap mask = XCreateBitmapFromData(dpy, win(), (char*)cursor.mask.buf,
                                        cursor.width(), cursor.height());
    Colour rgb;
    XColor fg, bg;
    getPF().rgbFromPixel(pix1, im->getColourMap(), &rgb);
    fg.red = rgb.r; fg.green = rgb.g; fg.blue = rgb.b;
    getPF().rgbFromPixel(pix0, im->getColourMap(), &rgb);
    bg.red = rgb.r; bg.green = rgb.g; bg.blue = rgb.b;
    if (localXCursor)
      XFreeCursor(dpy, localXCursor);
    localXCursor = XCreatePixmapCursor(dpy, source, mask, &fg, &bg,
                                       cursor.hotspot.x, cursor.hotspot.y);
    XDefineCursor(dpy, win(), localXCursor);
    XFreePixmap(dpy, source);
    XFreePixmap(dpy, mask);
    cursorAvailable = false;
    return;
  }

  if (!cursorAvailable) {
    XDefineCursor(dpy, win(), noCursor);
    cursorAvailable = true;
  }

  showLocalCursor();
}

void DesktopWindow::resetLocalCursor()
{
  hideLocalCursor();
  XDefineCursor(dpy, win(), dotCursor);
  cursorAvailable = false;
}

void DesktopWindow::hideLocalCursor()
{
  // - Blit the cursor backing store over the cursor
  if (cursorVisible) {
    cursorVisible = false;
    im->imageRect(cursorBackingRect, cursorBacking.data);
    im->put(win(), gc, cursorBackingRect);
  }
}

void DesktopWindow::showLocalCursor()
{
  if (cursorAvailable && !cursorVisible) {
    if (!getPF().equal(cursor.getPF()) ||
        cursor.getRect().is_empty()) {
      vlog.error("attempting to render invalid local cursor");
      XDefineCursor(dpy, win(), dotCursor);
      cursorAvailable = false;
      return;
    }
    cursorVisible = true;

    rfb::Rect cursorRect = (cursor.getRect().translate(cursorPos).
                            translate(cursor.hotspot.negate()));
    cursorBackingRect = cursorRect.intersect(im->getRect());
    im->getImage(cursorBacking.data, cursorBackingRect);

    im->maskRect(cursorRect, cursor.data, cursor.mask.buf);
    im->put(win(), gc, cursorBackingRect);
  }
}

// setColourMapEntries() changes some of the entries in the colourmap.
// Unfortunately these messages are often sent one at a time, so we delay the
// settings taking effect by 100ms.  This is because recalculating the internal
// translation table can be expensive.
void DesktopWindow::setColourMapEntries(int firstColour, int nColours,
                                        rdr::U16* rgbs)
{
  im->setColourMapEntries(firstColour, nColours, rgbs);
  if (!setColourMapEntriesTimer.isStarted())
    setColourMapEntriesTimer.start(100);
}

void DesktopWindow::serverCutText(const char* str, int len)
{
  if (acceptClipboard) {
    newServerCutText = true;
    delete [] serverCutText_;
    serverCutText_ = new char[len+1];
    memcpy(serverCutText_, str, len);
    serverCutText_[len] = 0;
  }
}


// Call XSync() at the end of an update.  We do this because we'd like to
// ensure that the current update has actually been drawn by the X server
// before the next update arrives - this is necessary for copyRect to
// behave correctly.  In particular, if part of the source of a copyRect is
// not actually displayed in the window, then XCopyArea results in
// GraphicsExpose events, which require us to draw from the off-screen
// image.  By the time XSync returns, the GraphicsExpose events will be in
// Xlib's queue, so hopefully will be processed before the next update.
// Possibly we should process the GraphicsExpose events here explicitly?

void DesktopWindow::framebufferUpdateEnd()
{
  XSync(dpy, False);
}


// invertRect() flips all the bits in every pixel in the given rectangle

void DesktopWindow::invertRect(const Rect& r)
{
  int stride;
  rdr::U8* p = im->getPixelsRW(r, &stride);
  for (int y = 0; y < r.height(); y++) {
    for (int x = 0; x < r.width(); x++) {
      switch (getPF().bpp) {
      case 8:  ((rdr::U8* )p)[x+y*stride] ^= 0xff;       break;
      case 16: ((rdr::U16*)p)[x+y*stride] ^= 0xffff;     break;
      case 32: ((rdr::U32*)p)[x+y*stride] ^= 0xffffffff; break;
      }
    }
  }
  im->put(win(), gc, r);
}


// resize() - resize the window and the image, taking care to remove the local
// cursor first.
void DesktopWindow::resize(int w, int h)
{
  hideLocalCursor();
  TXWindow::resize(w, h);
  im->resize(w, h);
}


bool DesktopWindow::handleTimeout(rfb::Timer* timer)
{
  if (timer == &setColourMapEntriesTimer) {
    im->updateColourMap();
    im->put(win(), gc, im->getRect());
  } else if (timer == &pointerEventTimer) {
    if (!viewOnly) {
      cc->writer()->pointerEvent(lastPointerPos, lastButtonMask);
    }
  }
  return false;
}


void DesktopWindow::handlePointerEvent(const Point& pos, int buttonMask)
{
  if (!viewOnly) {
    if (pointerEventInterval == 0 || buttonMask != lastButtonMask) {
      cc->writer()->pointerEvent(pos, buttonMask);
    } else {
      if (!pointerEventTimer.isStarted())
        pointerEventTimer.start(pointerEventInterval);
    }
    lastPointerPos = pos;
    lastButtonMask = buttonMask;
  }
  // - If local cursor rendering is enabled then use it
  if (cursorAvailable) {
    // - Render the cursor!
    if (!pos.equals(cursorPos)) {
      hideLocalCursor();
      if (im->getRect().contains(pos)) {
        cursorPos = pos;
        showLocalCursor();
      }
    }
  }
}


// handleXEvent() handles the various X events on the window
void DesktopWindow::handleEvent(TXWindow* w, XEvent* ev)
{
  switch (ev->type) {
  case GraphicsExpose:
  case Expose:
    im->put(win(), gc, Rect(ev->xexpose.x, ev->xexpose.y,
                            ev->xexpose.x + ev->xexpose.width,
                            ev->xexpose.y + ev->xexpose.height));
    break;

  case MotionNotify:
    while (XCheckTypedWindowEvent(dpy, win(), MotionNotify, ev));
    if (viewport && viewport->bumpScrollEvent(&ev->xmotion)) break;
    handlePointerEvent(Point(ev->xmotion.x, ev->xmotion.y),
                       (ev->xmotion.state & 0x1f00) >> 8);
    break;

  case ButtonPress:
    handlePointerEvent(Point(ev->xbutton.x, ev->xbutton.y),
                       (((ev->xbutton.state & 0x1f00) >> 8) |
                        (1 << (ev->xbutton.button-1))));
    break;

  case ButtonRelease:
    handlePointerEvent(Point(ev->xbutton.x, ev->xbutton.y),
                       (((ev->xbutton.state & 0x1f00) >> 8) &
                        ~(1 << (ev->xbutton.button-1))));
    break;

  case KeyPress:
    if (!viewOnly) {
      KeySym ks;
      char keyname[256];
      XLookupString(&ev->xkey, keyname, 256, &ks, NULL);
      bool fakeShiftPress = false;

      // Turn ISO_Left_Tab into shifted Tab
      if (ks == XK_ISO_Left_Tab) {
        fakeShiftPress = !(ev->xkey.state & ShiftMask);
        ks = XK_Tab;
      }

      if (fakeShiftPress)
        cc->writer()->keyEvent(XK_Shift_L, true);

      downKeysym[ev->xkey.keycode] = ks;
      cc->writer()->keyEvent(ks, true);

      if (fakeShiftPress)
        cc->writer()->keyEvent(XK_Shift_L, false);
      break;
    }

  case KeyRelease:
    if (!viewOnly) {
      if (downKeysym[ev->xkey.keycode]) {
        cc->writer()->keyEvent(downKeysym[ev->xkey.keycode], false);
        downKeysym[ev->xkey.keycode] = 0;
      }
    }
    break;

  case EnterNotify:
    newSelection = 0;
    if (sendPrimary && !selectionOwner(XA_PRIMARY)) {
      XConvertSelection(dpy, XA_PRIMARY, xaTIMESTAMP, xaSELECTION_TIME,
                        win(), ev->xcrossing.time);
    } else if (!selectionOwner(xaCLIPBOARD)) {
      XConvertSelection(dpy, xaCLIPBOARD, xaTIMESTAMP, xaSELECTION_TIME,
                        win(), ev->xcrossing.time);
    }
    break;

  case LeaveNotify:
    if (serverCutText_ && newServerCutText) {
      newServerCutText = false;
      vlog.debug("acquiring primary and clipboard selections");
      XStoreBytes(dpy, serverCutText_, strlen(serverCutText_));
      ownSelection(XA_PRIMARY, ev->xcrossing.time);
      ownSelection(xaCLIPBOARD, ev->xcrossing.time);
      currentSelectionTime = ev->xcrossing.time;
    }
    // Release all keys - this should probably done on a FocusOut event, but
    // LeaveNotify is near enough...
    for (int i = 8; i < 256; i++) {
      if (downKeysym[i]) {
        cc->writer()->keyEvent(downKeysym[i], false);
        downKeysym[i] = 0;
      }
    }
    break;
  }
}

// selectionRequest() is called when we are the selection owner and another X
// client has requested the selection.  We simply put the server's cut text
// into the requested property.  TXWindow will handle the rest.
bool DesktopWindow::selectionRequest(Window requestor,
                                     Atom selection, Atom property)
{
  XChangeProperty(dpy, requestor, property, XA_STRING, 8,
                  PropModeReplace, (unsigned char*)serverCutText_,
                  strlen(serverCutText_));
  return true;
}


// selectionNotify() is called when we have requested any information about a
// selection from the selection owner.  Note that there are two selections,
// PRIMARY and CLIPBOARD, plus the cut buffer, and we try to use whichever is
// the most recent of the three.
//
// There are two different "targets" for which selectionNotify() is called, the
// timestamp and the actual string value of the selection.  We always use the
// timestamp to decide which selection to retrieve.
//
// The first time selectionNotify() is called is when we are trying to find the
// timestamp of the selections at initialisation.  This should be called first
// for PRIMARY, then we call XConvertSelection() for CLIPBOARD.  The second
// time should be the result for CLIPBOARD.  At this stage we've got the
// "currentSelectionTime" so we return.
//
// Subsequently selectionNotify() is called whenever the mouse enters the
// viewer window.  Again, the first time it is called should be the timestamp
// for PRIMARY, and we then request the timestamp for CLIPBOARD.  When
// selectionNotify() is called again with the timestamp for CLIPBOARD, we now
// know if either selection is "new" i.e. later than the previous value of
// currentSelectionTime.  The last thing to check is the timestamp on the cut
// buffer.  If the cut buffer is newest we send that to the server, otherwise
// if one of the selections was newer, we request the string value of that
// selection.
//
// Finally, if we get selectionNotify() called for the string value of a
// selection, we sent that to the server.
//
// As a final minor complication, when one of the selections is actually owned
// by us, we don't request the details for it.

// TIME_LATER treats 0 as meaning a long time ago, so a==0 means a cannot be
// later than b.  This is different to the usual meaning of CurrentTime.
#define TIME_LATER(a, b) ((a) != 0 && ((b) == 0 || (long)((a) - (b)) > 0))


void DesktopWindow::selectionNotify(XSelectionEvent* ev, Atom type, int format,
                                    int nitems, void* data)
{
  if (ev->requestor != win())
    return;

  if (ev->target == xaTIMESTAMP) {
    if (ev->property == xaSELECTION_TIME) {
      if (data && format == 32 && nitems == 1) {
        Time t = *(rdr::U32 *)data;
        vlog.debug("selection (%d) time is %d, later %d",
                   ev->selection, t, TIME_LATER(t, currentSelectionTime));
        if (TIME_LATER(t, currentSelectionTime)) {
          currentSelectionTime = t;
          newSelection = ev->selection;
        }
      }
    } else {
      vlog.debug("no selection (%d)",ev->selection);
    }

    if (ev->selection == XA_PRIMARY) {
      if (!selectionOwner(xaCLIPBOARD)) {
        XConvertSelection(dpy, xaCLIPBOARD, xaTIMESTAMP, xaSELECTION_TIME,
                          win(), ev->time);
        return;
      }
    } else if (ev->selection != xaCLIPBOARD) {
      vlog.error("unknown selection %d",ev->selection);
      return;
    }

    if (gettingInitialSelectionTime) {
      gettingInitialSelectionTime = false;
      return;
    }

    if (!sendClipboard) return;
    if (sendPrimary) {
      vlog.debug("cut buffer time is %d, later %d", cutBufferTime,
                 TIME_LATER(cutBufferTime, currentSelectionTime));
      if (TIME_LATER(cutBufferTime, currentSelectionTime)) {
        currentSelectionTime = cutBufferTime;
        int len;
        char* str = XFetchBytes(dpy, &len);
        if (str) {
          if (!viewOnly) {
            vlog.debug("sending cut buffer to server");
            cc->writer()->clientCutText(str, len);
          }
          XFree(str);
          return;
        }
      }
    }
    if (newSelection) {
      XConvertSelection(dpy, newSelection, XA_STRING, xaSELECTION_STRING,
                        win(), CurrentTime);
    }

  } else if (ev->target == XA_STRING) {
    if (!sendClipboard) return;
    if (ev->property == xaSELECTION_STRING) {
      if (data && format == 8) {
        if (!viewOnly) {
          vlog.debug("sending %s selection to server",
                     ev->selection == XA_PRIMARY ? "primary" :
                     ev->selection == xaCLIPBOARD ? "clipboard" : "unknown" );
          cc->writer()->clientCutText((char*)data, nitems);
        }
      }
    }
  }
}
