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
// TXScrollbar.cxx
//

#include "TXScrollbar.h"
#include <stdio.h>
#include <assert.h>

TXScrollbar::TXScrollbar(Display* dpy_, int width, int height, bool vert,
                         TXScrollbarCallback* cb_, TXWindow* parent_)
  : TXWindow(dpy_, width, height, parent_), cb(cb_), vertical(vert),
    clickedInThumb(false)
{
  setEventHandler(this);
  gc = XCreateGC(dpy, win(), 0, 0);
  addEventMask(ExposureMask | ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask);
  setBg(scrollbarBg);
  limit[0] = len[0] = limit[1] = len[1] = 1;
  start[0] = start[1] = 0;
}

TXScrollbar::~TXScrollbar()
{
  XFreeGC(dpy, gc);
}

void TXScrollbar::set(int limit_, int start_, int len_, bool vert)
{
  assert(limit_ > 0 && len_ >= 0 && len_ <= limit_);

  if (start_ < 0) start_ = 0;
  if (start_ > limit_ - len_) start_ = limit_ - len_;

  if (limit[vert] != limit_ || start[vert] != start_ || len[vert] != len_) {
    limit[vert] = limit_;
    start[vert] = start_;
    len[vert] = len_;
    paint();
  }
}

void TXScrollbar::paint()
{
  int x = scaleToBarX(start[0]);
  int y = scaleToBarY(start[1]);
  int w = scaleToBarX(len[0]);
  int h = scaleToBarY(len[1]);
  if (y > 0) XClearArea(dpy, win(), 0, 0, 0, y, false);
  if (x > 0) XClearArea(dpy, win(), 0, y, x, y+h, false);
  XClearArea(dpy, win(), x+w, y, 0, y+h, false);
  XClearArea(dpy, win(), 0, y+h, 0, 0, false);
  drawBevel(gc, x, y, w, h, bevel, defaultBg, lightBg, darkBg);
}

void TXScrollbar::handleEvent(TXWindow* w, XEvent* ev)
{
  switch (ev->type) {
  case Expose:
    paint();
    break;

  case ButtonPress:
    {
      xDown = ev->xbutton.x;
      yDown = ev->xbutton.y;
      xStart = start[0];
      yStart = start[1];
      bool clickedInThumbX = false;
      if (xDown < scaleToBarX(start[0])) {
        set(limit[0], start[0] - len[0], len[0], false);
      } else if (xDown >= scaleToBarX(start[0]+len[0])) {
        set(limit[0], start[0] + len[0], len[0], false);
      } else {
        clickedInThumbX = true;
      }
      bool clickedInThumbY = false;
      if (yDown < scaleToBarY(start[1])) {
        set(limit[1], start[1] - len[1], len[1], true);
      } else if (yDown >= scaleToBarY(start[1]+len[1])) {
        set(limit[1], start[1] + len[1], len[1], true);
      } else {
        clickedInThumbY = true;
      }
      clickedInThumb = clickedInThumbX && clickedInThumbY;
      if (cb) cb->scrollbarPos(start[0], start[1], this);
    }
    break;

  case ButtonRelease:
  case MotionNotify:
    while (XCheckTypedWindowEvent(dpy, win(), MotionNotify, ev));
    if (clickedInThumb) {
      int dx = ev->xmotion.x - xDown;
      int dy = ev->xmotion.y - yDown;
      set(limit[0], xStart + barToScaleX(dx), len[0], false);
      set(limit[1], yStart + barToScaleY(dy), len[1], true);
      if (cb) cb->scrollbarPos(start[0], start[1], this);
    }
    break;
  }
}
