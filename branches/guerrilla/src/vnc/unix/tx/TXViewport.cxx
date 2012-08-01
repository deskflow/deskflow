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
// TXViewport.cxx
//

#include "TXViewport.h"
#include <stdio.h>

TXViewport::TXViewport(Display* dpy_, int w, int h, TXWindow* parent_)
  : TXWindow(dpy_, w, h, parent_), child(0), hScrollbar(0),
    vScrollbar(0), scrollbarSize(15), xOff(0), yOff(0), bumpScrollTimer(this),
    bumpScroll(false), needScrollbars(false), bumpScrollX(0), bumpScrollY(0)
{
  clipper = new TXWindow(dpy, width()-scrollbarSize, height()-scrollbarSize,
                         this);
  clipper->setBg(black);
  hScrollbar = new TXScrollbar(dpy, width()-scrollbarSize, scrollbarSize,
                               false, this, this);
  vScrollbar = new TXScrollbar(dpy, scrollbarSize, height()-scrollbarSize,
                               true, this, this);
}

TXViewport::~TXViewport()
{
  delete clipper;
  delete hScrollbar;
  delete vScrollbar;
}

void TXViewport::setChild(TXWindow* child_)
{
  child = child_;
  XReparentWindow(dpy, child->win(), clipper->win(), 0, 0);
  xOff = yOff = 0;
  child->map();
  resizeNotify();
}

bool TXViewport::setOffset(int x, int y)
{
  if (clipper->width() >= child->width()) {
    x = (clipper->width() - child->width()) / 2;
  } else {
    if (x > 0) x = 0;
    if (x + child->width() < clipper->width())
      x = clipper->width() - child->width();
  }

  if (clipper->height() >= child->height()) {
    y = (clipper->height() - child->height()) / 2;
  } else {
    if (y > 0) y = 0;
    if (y + child->height() < clipper->height())
      y = clipper->height() - child->height();
  }

  if (x != xOff || y != yOff) {
    xOff = x;
    yOff = y;
    child->move(xOff, yOff);
    return true;
  }

  return false;
}

void TXViewport::setBumpScroll(bool b)
{
  bumpScroll = b;
  resizeNotify();
}

// Note: bumpScrollEvent() only works if the viewport is positioned at 0,0 and
// is the same width and height as the screen.
bool TXViewport::bumpScrollEvent(XMotionEvent* ev)
{
  if (!bumpScroll) return false;
  int bumpScrollPixels = 20;
  bumpScrollX = bumpScrollY = 0;

  if (ev->x_root == width()-1)  bumpScrollX = -bumpScrollPixels;
  else if (ev->x_root == 0)     bumpScrollX = bumpScrollPixels;
  if (ev->y_root == height()-1) bumpScrollY = -bumpScrollPixels;
  else if (ev->y_root == 0)     bumpScrollY = bumpScrollPixels;

  if (bumpScrollX || bumpScrollY) {
    if (bumpScrollTimer.isStarted()) return true;
    if (setOffset(xOff + bumpScrollX, yOff + bumpScrollY)) {
      bumpScrollTimer.start(25);
      return true;
    }
  }

  bumpScrollTimer.stop();
  return false;
}

bool TXViewport::handleTimeout(rfb::Timer* timer) {
  return setOffset(xOff + bumpScrollX, yOff + bumpScrollY);
}

void TXViewport::resizeNotify()
{
  needScrollbars = (!bumpScroll &&
                    (width() < child->width() || height() < child->height()) &&
                    (width() > scrollbarSize && height() > scrollbarSize));
  if (needScrollbars) {
    clipper->resize(width()-scrollbarSize, height()-scrollbarSize);
    hScrollbar->map();
    vScrollbar->map();
  } else {
    clipper->resize(width(), height());
    hScrollbar->unmap();
    vScrollbar->unmap();
  }

  setOffset(xOff, yOff);

  if (needScrollbars) {
    hScrollbar->move(0, height()-scrollbarSize);
    hScrollbar->resize(width()-scrollbarSize, scrollbarSize);
    hScrollbar->set(child->width(), -xOff, width()-scrollbarSize);
    vScrollbar->move(width()-scrollbarSize, 0);
    vScrollbar->resize(scrollbarSize, height()-scrollbarSize);
    vScrollbar->set(child->height(), -yOff, height()-scrollbarSize);
  }
}

void TXViewport::scrollbarPos(int x, int y, TXScrollbar* sb)
{
  if (sb == hScrollbar) {
    x = -x;
    y = yOff;
  } else {
    x = xOff;
    y = -y;
  }
  setOffset(x, y);
}
