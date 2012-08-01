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
// TXScrollbar.h
//
// A TXScrollbar represents a range of values starting at start, of length len,
// between zero and limit.  The vertical argument to the constructor says
// whether the scrollbar is horizontal or vertical.
//
// In fact it can represent a range in each dimension but usually one of the
// dimensions is fixed, according to the vertical flag (for a vertical
// scrollbar, the horizontal dimension is fixed, and vice-versa).
//
// The TXScrollbarCallback argument is an object which will be notified when
// the user has attempted to move the scrollbar.  The x and y arguments to the
// scrollbarPos() method give the start values in the respective dimensions.
// They are guaranteed to be between 0 and limit-len.
//

#ifndef __TXSCROLLBAR_H__
#define __TXSCROLLBAR_H__

#include "TXWindow.h"

class TXScrollbarCallback;

class TXScrollbar : public TXWindow, public TXEventHandler {
public:
  TXScrollbar(Display* dpy_, int width=1, int height=1, bool vertical=false,
              TXScrollbarCallback* cb=0, TXWindow* parent_=0);
  virtual ~TXScrollbar();

  // set() sets the limit, start and length of the range represented by the
  // scrollbar.  The values of limit and len passed in must be valid
  // (i.e. limit > 0 and 0 <= len <= limit).  Values of start are clipped to
  // the range 0 to limit-len.
  void set(int limit, int start, int len) { set(limit, start, len, vertical); }

  // set() with an extra argument vert can be used to represent a range in both
  // dimensions simultaneously.
  void set(int limit, int start, int len, bool vert);

  virtual void handleEvent(TXWindow* w, XEvent* ev);

private:
  int scaleToBarX(int x) { return (x * width() + limit[0]/2) / limit[0]; }
  int scaleToBarY(int y) { return (y * height() + limit[1]/2) / limit[1]; }
  int barToScaleX(int x) { return (x * limit[0] + width()/2) / width(); }
  int barToScaleY(int y) { return (y * limit[1] + height()/2) / height(); }
  void paint();

  GC gc;
  TXScrollbarCallback* cb;
  int limit[2];
  int start[2];
  int len[2];
  int xDown, yDown;
  int xStart, yStart;
  bool vertical;
  bool clickedInThumb;
};

class TXScrollbarCallback {
public:
  virtual void scrollbarPos(int x, int y, TXScrollbar* sb)=0;
};
#endif
