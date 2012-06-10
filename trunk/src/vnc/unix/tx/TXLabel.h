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
// TXLabel.h
//
// An TXLabel allows you to put up multiline text in a window with various
// alignments.  The label must be big enough to contain the text - if not then
// it will be resized appropriately.
//

#ifndef __TXLABEL_H__
#define __TXLABEL_H__

#include <stdlib.h>
#include "TXWindow.h"
#include <rfb/util.h>

class TXLabel : public TXWindow, public TXEventHandler {
public:
  enum HAlign { left, centre, right };
  enum VAlign { top, middle, bottom };

  TXLabel(Display* dpy_, const char* text_, TXWindow* parent_=0,
          int w=1, int h=1, HAlign ha=centre, VAlign va=middle)
    : TXWindow(dpy_, w, h, parent_), lineSpacing(2), lines(0),
      halign(ha), valign(va)
  {
    setEventHandler(this);
    setText(text_);
    addEventMask(ExposureMask);
  }

  // setText() changes the text in the label.
  void setText(const char* text_) {
    text.buf = rfb::strDup(text_);
    lines = 0;
    int lineStart = 0;
    int textWidth = 0;
    int i = -1;
    do {
      i++;
      if (text.buf[i] == '\n' || text.buf[i] == 0) {
        int tw = XTextWidth(defaultFS, &text.buf[lineStart], i-lineStart);
        if (tw > textWidth) textWidth = tw;
        lineStart = i+1;
        lines++;
      }
    } while (text.buf[i] != 0);
    int textHeight = ((defaultFS->ascent + defaultFS->descent + lineSpacing)
                      * lines);
    int newWidth = __rfbmax(width(), textWidth + xPad*2);
    int newHeight = __rfbmax(height(), textHeight + yPad*2);
    if (width() < newWidth || height() < newHeight) {
      resize(newWidth, newHeight);
    }
    invalidate();
  }

private:
  int xOffset(int textWidth) {
    switch (halign) {
    case left:  return xPad;
    case right: return width() - xPad - textWidth;
    default:    return (width() - textWidth) / 2;
    }
  }

  int yOffset(int lineNum) {
    int textHeight = ((defaultFS->ascent + defaultFS->descent + lineSpacing)
                      * lines);
    int lineOffset = ((defaultFS->ascent + defaultFS->descent + lineSpacing)
                      * lineNum + defaultFS->ascent);
    switch (valign) {
    case top:    return yPad + lineOffset;
    case bottom: return height() - yPad - textHeight + lineOffset;
    default:     return (height() - textHeight) / 2 + lineOffset;
    }
  }

  void paint() {
    int lineNum = 0;
    int lineStart = 0;
    int i = -1;
    do {
      i++;
      if (text.buf[i] == '\n' || text.buf[i] == 0) {
        int tw = XTextWidth(defaultFS, &text.buf[lineStart], i-lineStart);
        XDrawString(dpy, win(), defaultGC, xOffset(tw), yOffset(lineNum),
                    &text.buf[lineStart], i-lineStart);
        lineStart = i+1;
        lineNum++;
      }
    } while (text.buf[i] != 0);
  }

  virtual void handleEvent(TXWindow* w, XEvent* ev) {
    switch (ev->type) {
    case Expose:
      paint();
      break;
    }
  }

  int lineSpacing;
  rfb::CharArray text;
  int lines;
  HAlign halign;
  VAlign valign;
};

#endif
