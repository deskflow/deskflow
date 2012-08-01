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
// TXButton.h
//
// A TXButton is a clickable button with some text in it.  The button must be
// big enough to contain the text - if not then it will be resized
// appropriately.
//

#ifndef __TXBUTTON_H__
#define __TXBUTTON_H__

#include "TXWindow.h"
#include <rfb/util.h>

// TXButtonCallback's buttonActivate() method is called when a button is
// activated.
class TXButton;
class TXButtonCallback {
public:
  virtual void buttonActivate(TXButton* button)=0;
};


class TXButton : public TXWindow, public TXEventHandler {
public:

  TXButton(Display* dpy_, const char* text_, TXButtonCallback* cb_=0,
           TXWindow* parent_=0, int w=1, int h=1)
    : TXWindow(dpy_, w, h, parent_), cb(cb_), down(false),
      disabled_(false)
  {
    setEventHandler(this);
    setText(text_);
    gc = XCreateGC(dpy, win(), 0, 0);
    XSetFont(dpy, gc, defaultFont);
    addEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask);
  }

  virtual ~TXButton() {
    XFreeGC(dpy, gc);
  }

  // setText() changes the text in the button.
  void setText(const char* text_) {
    text.buf = rfb::strDup(text_);
    int textWidth = XTextWidth(defaultFS, text.buf, strlen(text.buf));
    int textHeight = (defaultFS->ascent + defaultFS->descent);
    int newWidth = __rfbmax(width(), textWidth + xPad*2 + bevel*2);
    int newHeight = __rfbmax(height(), textHeight + yPad*2 + bevel*2);
    if (width() < newWidth || height() < newHeight) {
      resize(newWidth, newHeight);
    }
  }

  // disabled() sets or queries the disabled state of the checkbox.  A disabled
  // checkbox cannot be changed via the user interface.
  void disabled(bool b) { disabled_ = b; paint(); }
  bool disabled() { return disabled_; }

private:

  void paint() {
    int tw = XTextWidth(defaultFS, text.buf, strlen(text.buf));
    int startx = (width() - tw) / 2;
    int starty = (height() + defaultFS->ascent - defaultFS->descent) / 2;
    if (down || disabled_) {
      drawBevel(gc, 0, 0, width(), height(), bevel, defaultBg, darkBg,lightBg);
      startx++; starty++;
    } else {
      drawBevel(gc, 0, 0, width(), height(), bevel, defaultBg, lightBg,darkBg);
    }

    XSetForeground(dpy, gc, disabled_ ? disabledFg : defaultFg);
    XDrawString(dpy, win(), gc, startx, starty, text.buf, strlen(text.buf));
  }

  virtual void handleEvent(TXWindow* w, XEvent* ev) {
    switch (ev->type) {
    case Expose:
      paint();
      break;
    case ButtonPress:
      if (!disabled_) {
        down = true;
        paint();
      }
      break;
    case ButtonRelease:
      if (!down) break;
      down = false;
      paint();
      if (ev->xbutton.x >= 0 && ev->xbutton.x < width() &&
          ev->xbutton.y >= 0 && ev->xbutton.y < height()) {
        if (cb) cb->buttonActivate(this);
      }
      break;
    }
  }

  GC gc;
  rfb::CharArray text;
  TXButtonCallback* cb;
  bool down;
  bool disabled_;
};

#endif
