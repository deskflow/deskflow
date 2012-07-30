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
// TXCheckbox.h
//
// A TXCheckbox has a box which may be "checked" with some text next to it.
// The checkbox window must be big enough to contain the text - if not then it
// will be resized appropriately.
//
// There are two styles of checkbox: the normal style which uses a tick in a
// square box, and the radio style which uses a dot inside a circle.  The
// default behaviour when clicking on the checkbox is to toggle it on or off,
// but this behaviour can be changed by the callback object.  In particular to
// get radiobutton behaviour, the callback must ensure that only one of a set
// of radiobuttons is selected.
//

#ifndef __TXCHECKBOX_H__
#define __TXCHECKBOX_H__

#include "TXWindow.h"
#include <rfb/util.h>

// TXCheckboxCallback's checkboxSelect() method is called when the state of a
// checkbox changes.
class TXCheckbox;
class TXCheckboxCallback {
public:
  virtual void checkboxSelect(TXCheckbox* checkbox)=0;
};


class TXCheckbox : public TXWindow, public TXEventHandler {
public:
  TXCheckbox(Display* dpy_, const char* text_, TXCheckboxCallback* cb_,
             bool radio_=false, TXWindow* parent_=0, int w=1, int h=1)
    : TXWindow(dpy_, w, h, parent_), cb(cb_), text(0),
      boxSize(radio_ ? 12 : 13), boxPad(4),
      checked_(false), disabled_(false), radio(radio_)
  {
    setEventHandler(this);
    setText(text_);
    gc = XCreateGC(dpy, win(), 0, 0);
    XSetFont(dpy, gc, defaultFont);
    addEventMask(ExposureMask| ButtonPressMask | ButtonReleaseMask);
  }

  virtual ~TXCheckbox() {
    XFreeGC(dpy, gc);
    if (text) free(text);
  }

  // setText() changes the text in the checkbox.
  void setText(const char* text_) {
    if (text) free(text);
    text = strdup(text_);
    int textWidth = XTextWidth(defaultFS, text, strlen(text));
    int textHeight = (defaultFS->ascent + defaultFS->descent);
    int newWidth = __rfbmax(width(), textWidth + xPad*2 + boxPad*2 + boxSize);
    int newHeight = __rfbmax(height(), textHeight + yPad*2);
    if (width() < newWidth || height() < newHeight) {
      resize(newWidth, newHeight);
    }
  }

  // checked() sets or queries the state of the checkbox
  void checked(bool b) { checked_ = b; paint(); }
  bool checked() { return checked_; }

  // disabled() sets or queries the disabled state of the checkbox.  A disabled
  // checkbox cannot be changed via the user interface.
  void disabled(bool b) { disabled_ = b; paint(); }
  bool disabled() { return disabled_; }

private:
  void paint() {
    if (disabled_)
      drawBevel(gc, xPad + boxPad, (height() - boxSize) / 2, boxSize, boxSize,
                bevel, disabledBg, darkBg, lightBg, radio);
    else
      drawBevel(gc, xPad + boxPad, (height() - boxSize) / 2, boxSize, boxSize,
                bevel, enabledBg, darkBg, lightBg, radio);
    XSetBackground(dpy, gc, disabled_ ? disabledBg : enabledBg);
    XSetForeground(dpy, gc, disabled_ ? disabledFg : defaultFg);
    if (checked_) {
      Pixmap icon = radio ? dot : tick;
      int iconSize = radio ? dotSize : tickSize;
      XCopyPlane(dpy, icon, win(), gc, 0, 0, iconSize, iconSize,
                 xPad + boxPad + (boxSize - iconSize) / 2,
                 (height() - iconSize) / 2, 1);
    }
    XDrawString(dpy, win(), gc, xPad + boxSize + boxPad*2,
                (height() + defaultFS->ascent - defaultFS->descent) / 2,
                text, strlen(text));
  }

  virtual void handleEvent(TXWindow* w, XEvent* ev) {
    switch (ev->type) {
    case Expose:
      paint();
      break;
    case ButtonPress:
      break;
    case ButtonRelease:
      if (ev->xbutton.x >= 0 && ev->xbutton.x < width() &&
          ev->xbutton.y >= 0 && ev->xbutton.y < height()) {
        if (!disabled_) {
          checked_ = !checked_;
          if (cb) cb->checkboxSelect(this);
          paint();
        }
      }
      break;
    }
  }

  TXCheckboxCallback* cb;
  GC gc;
  char* text;
  int boxSize;
  int boxPad;
  bool checked_;
  bool disabled_;
  bool radio;
};

#endif
