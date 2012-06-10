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
// TXViewport.h
//
// A TXViewport allows a large window to be viewed by adding scrollbars to the
// right and bottom if necessary.  It also has a bump-scroll mode where there
// are no scrollbars, and scrolling is achieved by bumping up against the edge
// of the screen instead.  Note that this only works when the viewport fills
// the entire screen.  If the child window is smaller than the viewport, it is
// always positioned centrally in the viewport.

#ifndef __TXVIEWPORT_H__
#define __TXVIEWPORT_H__

#include <rfb/Timer.h>
#include "TXWindow.h"
#include "TXScrollbar.h"

class TXViewport : public TXWindow, public TXScrollbarCallback,
                   public rfb::Timer::Callback {
public:
  TXViewport(Display* dpy_, int width, int height, TXWindow* parent_=0);
  virtual ~TXViewport();

  // setChild() sets the child window which is to be viewed in the viewport.
  void setChild(TXWindow* child_);

  // setOffset() sets the position of the child in the viewport.  Note that the
  // offsets are negative.  For example when the offset is (-100,-30), position
  // (100,30) in the child window is at the top-left of the viewport.  The
  // offsets given are clipped to keep the child window filling the viewport
  // (except where the child window is smaller than the viewport, in which case
  // it is always positioned centrally in the viewport).  It returns true if
  // the child was repositioned.
  bool setOffset(int x, int y);

  // setBumpScroll() puts the viewport in bump-scroll mode.
  void setBumpScroll(bool b);

  // bumpScrollEvent() can be called with a MotionNotify event which may
  // potentially be against the edge of the screen.  It returns true if the
  // event was used for bump-scrolling, false if it should be processed
  // normally.
  bool bumpScrollEvent(XMotionEvent* ev);

private:
  virtual void resizeNotify();
  virtual void scrollbarPos(int x, int y, TXScrollbar* sb);
  virtual bool handleTimeout(rfb::Timer* timer);
  TXWindow* clipper;
  TXWindow* child;
  TXScrollbar* hScrollbar;
  TXScrollbar* vScrollbar;
  const int scrollbarSize;
  int xOff, yOff;
  rfb::Timer bumpScrollTimer;
  bool bumpScroll;
  bool needScrollbars;
  int bumpScrollX, bumpScrollY;
};
#endif
