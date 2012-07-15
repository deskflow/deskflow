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
// TXEntry.h
//
// A TXEntry allows you to enter a single line of text in a window.  The entry
// must be tall enough to contain a line of text - if not then it will be
// resized appropriately.  If the passwd argument to the constructor is true,
// then the text in the entry will be replaced by asterisks on the screen.
//

#ifndef __TXENTRY_H__
#define __TXENTRY_H__

#include "TXWindow.h"
#include <X11/keysym.h>

#ifndef XK_ISO_Left_Tab
#define	XK_ISO_Left_Tab					0xFE20
#endif

// TXEntryCallback's entryCallback() method is called when one of three special
// key presses have happened: Enter/Return, forward tab, or backward tab.
class TXEntry;
class TXEntryCallback {
public:
  enum Detail { ENTER, NEXT_FOCUS, PREV_FOCUS };
  virtual void entryCallback(TXEntry* entry, Detail detail, Time time)=0;
};


class TXEntry : public TXWindow, public TXEventHandler {
public:

  TXEntry(Display* dpy_, TXEntryCallback* cb_=0,
          TXWindow* parent_=0, bool passwd_=false, int w=1, int h=1)
    : TXWindow(dpy_, w, h, parent_), cb(cb_),
      passwd(passwd_), disabled_(false), gotFocus(false)
  {
    setEventHandler(this);
    gc = XCreateGC(dpy, win(), 0, 0);
    addEventMask(ExposureMask | KeyPressMask | FocusChangeMask
                 | ButtonPressMask);
    text[0] = 0;
    int textHeight = (defaultFS->ascent + defaultFS->descent);
    int newHeight = __rfbmax(height(), textHeight + yPad*2 + bevel*2);
    if (height() < newHeight) {
      resize(width(), newHeight);
    }
  }

  virtual ~TXEntry() {
    XFreeGC(dpy, gc);
    // overwrite memory used to store password - not critical, but can avoid
    // accidental exposure of a password in uninitialised memory.
    if (passwd)
      memset(text, 0, maxLen);
  }

  // getText() gets the text in the entry.
  const char* getText() { return text; }

  // setText() sets the text in the entry.
  void setText(const char* text_) {
    strncpy(text, text_, maxLen-1);
    text[maxLen-1] = 0;
    paint();
  }

  // disabled() sets or queries the disabled state of the entry.  A disabled
  // entry cannot have text entered into it.
  void disabled(bool b) { disabled_ = b; paint(); }
  bool disabled() { return disabled_; }

private:
  void paint() {
    if (disabled_)
      drawBevel(gc, 0, 0, width(), height(), bevel, disabledBg,darkBg,lightBg);
    else
      drawBevel(gc, 0, 0, width(), height(), bevel, enabledBg, darkBg,lightBg);
    char* str = text;
    char stars[maxLen];
    if (passwd) {
      int i;
      for (i = 0; i < (int)strlen(text); i++) stars[i] = '*';
      stars[i] = 0;
      str = stars;
    }
    int tw = XTextWidth(defaultFS, str, strlen(str));
    int startx = bevel + xPad;
    if (startx + tw > width() - 2*bevel) {
      startx = width() - 2*bevel - tw;
    }
    XDrawString(dpy, win(), defaultGC, startx,
                (height() + defaultFS->ascent - defaultFS->descent) / 2,
                str, strlen(str));
    if (!disabled_ && gotFocus)
      XDrawLine(dpy, win(), defaultGC, startx+tw,
                (height() - defaultFS->ascent - defaultFS->descent) / 2,
                startx+tw,
                (height() + defaultFS->ascent + defaultFS->descent) / 2);
  }

  virtual void handleEvent(TXWindow* w, XEvent* ev) {
    switch (ev->type) {
    case Expose:
      paint();
      break;

    case FocusIn:
      gotFocus = true;
      paint();
      break;

    case FocusOut:
      gotFocus = false;
      paint();
      break;

    case ButtonPress:
      if (!disabled_)
        XSetInputFocus(dpy, win(), RevertToParent, ev->xbutton.time);
      break;

    case KeyPress:
      {
        if (disabled_ || !gotFocus) break;
        KeySym keysym;
        XComposeStatus compose;
        char buf[10];
        int count = XLookupString(&ev->xkey, buf, 10, &keysym, &compose);
        if (count >= 1 && buf[0] >= ' ' && buf[0] <= '~') {
          if (strlen(text) + count >= maxLen) {
            XBell(dpy, 0);
          } else {
            strncat(text, buf, count);
            paint();
          }
        } else if (keysym == XK_BackSpace || keysym == XK_Delete ||
                   keysym == XK_KP_Delete) {
          if (strlen(text) > 0) {
            text[strlen(text)-1] = 0;
            paint();
          }
        } else if (keysym == XK_Return || keysym == XK_KP_Enter ||
                   keysym == XK_Linefeed) {
          if (cb) cb->entryCallback(this, TXEntryCallback::ENTER,
                                    ev->xkey.time);
        } else if ((keysym == XK_Tab || keysym == XK_KP_Tab)
                   && !(ev->xkey.state & ShiftMask))
        {
          if (cb) cb->entryCallback(this, TXEntryCallback::NEXT_FOCUS,
                                    ev->xkey.time);
        } else if (((keysym == XK_Tab || keysym == XK_KP_Tab)
                    && (ev->xkey.state & ShiftMask))
                   || keysym == XK_ISO_Left_Tab)
        {
          if (cb) cb->entryCallback(this, TXEntryCallback::PREV_FOCUS,
                                    ev->xkey.time);
        }
      }
    }
  }
  GC gc;
  enum { maxLen = 256 };
  char text[maxLen];
  TXEntryCallback* cb;
  bool passwd;
  bool disabled_;
  bool gotFocus;
};

#endif
