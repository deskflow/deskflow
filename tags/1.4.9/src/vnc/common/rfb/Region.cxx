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

// Cross-platform Region class based on the X11 region implementation.  Note
// that for efficiency this code manipulates the Xlib region structure
// directly.  Apart from the layout of the structure, there is one other key
// assumption made: a Region returned from XCreateRegion must always have its
// rects member allocated so that there is space for at least one rectangle.
//

#include <rfb/Region.h>
#include <Xregion/Xregion.h>
#include <Xregion/region.h>
#include <assert.h>
#include <stdio.h>

#pragma warning(disable: 4800)

// A _RectRegion must never be passed as a return parameter to the Xlib region
// operations.  This is because for efficiency its "rects" member has not been
// allocated with Xmalloc.  It is however safe to pass it as an input
// parameter.

class _RectRegion {
public:
  _RectRegion(const rfb::Rect& r) {
    region.rects = &region.extents;
    region.numRects = 1;
    region.extents.x1 = r.tl.x;
    region.extents.y1 = r.tl.y;
    region.extents.x2 = r.br.x;
    region.extents.y2 = r.br.y;
    region.size = 1;
    if (r.is_empty())
      region.numRects = 0;
  }
  REGION region;
};


rfb::Region::Region() {
  xrgn = XCreateRegion();
  assert(xrgn);
}

rfb::Region::Region(const Rect& r) {
  xrgn = XCreateRegion();
  assert(xrgn);
  reset(r);
}

rfb::Region::Region(const rfb::Region& r) {
  xrgn = XCreateRegion();
  assert(xrgn);
  XUnionRegion(xrgn, r.xrgn, xrgn);
}

rfb::Region::~Region() {
  XDestroyRegion(xrgn);
}

rfb::Region& rfb::Region::operator=(const rfb::Region& r) {
  clear();
  XUnionRegion(xrgn, r.xrgn, xrgn);
  return *this;
}

void rfb::Region::clear() {
  xrgn->numRects = 0;
  xrgn->extents.x1 = 0;
  xrgn->extents.y1 = 0;
  xrgn->extents.x2 = 0;
  xrgn->extents.y2 = 0;
}

void rfb::Region::reset(const Rect& r) {
  if (r.is_empty()) {
    clear();
  } else {
    xrgn->numRects = 1;
    xrgn->rects[0].x1 = xrgn->extents.x1 = r.tl.x;
    xrgn->rects[0].y1 = xrgn->extents.y1 = r.tl.y;
    xrgn->rects[0].x2 = xrgn->extents.x2 = r.br.x;
    xrgn->rects[0].y2 = xrgn->extents.y2 = r.br.y;
  }
}

void rfb::Region::translate(const Point& delta) {
  XOffsetRegion(xrgn, delta.x, delta.y);
}

void rfb::Region::setOrderedRects(const std::vector<Rect>& rects) {
  clear();
  std::vector<Rect>::const_iterator i;
  for (i=rects.begin(); i != rects.end(); i++) {
    _RectRegion rr(*i);
    XUnionRegion(xrgn, &rr.region, xrgn);
  }
}

void rfb::Region::setExtentsAndOrderedRects(const ShortRect* extents,
                                            int nRects, const ShortRect* rects)
{
  if (xrgn->size < nRects)
  {
    BOX* prevRects = xrgn->rects;
    xrgn->rects = (BOX*)Xrealloc((char*)xrgn->rects, nRects * sizeof(BOX));
    if (!xrgn->rects) {
      fprintf(stderr,"Xrealloc failed\n");
      Xfree(prevRects);
      return;
    }
    xrgn->size = nRects;
  }

  xrgn->numRects = nRects;
  xrgn->extents.x1 = extents->x1;
  xrgn->extents.y1 = extents->y1;
  xrgn->extents.x2 = extents->x2;
  xrgn->extents.y2 = extents->y2;
  for (int i = 0; i < nRects; i++) {
    xrgn->rects[i].x1 = rects[i].x1;
    xrgn->rects[i].y1 = rects[i].y1;
    xrgn->rects[i].x2 = rects[i].x2;
    xrgn->rects[i].y2 = rects[i].y2;
  }
}

void rfb::Region::copyFrom(const rfb::Region& r) {
  XUnionRegion(r.xrgn, r.xrgn, xrgn);
}

void rfb::Region::assign_intersect(const rfb::Region& r) {
  XIntersectRegion(xrgn, r.xrgn, xrgn);
}

void rfb::Region::assign_union(const rfb::Region& r) {
  XUnionRegion(xrgn, r.xrgn, xrgn);
}

void rfb::Region::assign_subtract(const rfb::Region& r) {
  XSubtractRegion(xrgn, r.xrgn, xrgn);
}

rfb::Region rfb::Region::intersect(const rfb::Region& r) const {
  rfb::Region ret;
  XIntersectRegion(xrgn, r.xrgn, ret.xrgn);
  return ret;
}

rfb::Region rfb::Region::union_(const rfb::Region& r) const {
  rfb::Region ret;
  XUnionRegion(xrgn, r.xrgn, ret.xrgn);
  return ret;
}

rfb::Region rfb::Region::subtract(const rfb::Region& r) const {
  rfb::Region ret;
  XSubtractRegion(xrgn, r.xrgn, ret.xrgn);
  return ret;
}

bool rfb::Region::equals(const rfb::Region& r) const {
  return XEqualRegion(xrgn, r.xrgn);
}

int rfb::Region::numRects() const {
  return xrgn->numRects;
}

bool rfb::Region::get_rects(std::vector<Rect>* rects,
                            bool left2right, bool topdown, int maxArea) const
{
  int nRects = xrgn->numRects;
  int xInc = left2right ? 1 : -1;
  int yInc = topdown ? 1 : -1;
  int i = topdown ? 0 : nRects-1;
  rects->clear();
  rects->reserve(nRects);

  while (nRects > 0) {
    int firstInNextBand = i;
    int nRectsInBand = 0;

    while (nRects > 0 && xrgn->rects[firstInNextBand].y1 == xrgn->rects[i].y1)
    {
      firstInNextBand += yInc;
      nRects--;
      nRectsInBand++;
    }

    if (xInc != yInc)
      i = firstInNextBand - yInc;

    while (nRectsInBand > 0) {
      int y = xrgn->rects[i].y1;
      int h = maxArea / (xrgn->rects[i].x2 - xrgn->rects[i].x1);
      if (!h) h = xrgn->rects[i].y2 - y;
      do {
        if (h > xrgn->rects[i].y2 - y)
          h = xrgn->rects[i].y2 - y;
        Rect r(xrgn->rects[i].x1, y, xrgn->rects[i].x2, y+h);
        rects->push_back(r);
        y += h;
      } while (y < xrgn->rects[i].y2);
      i += xInc;
      nRectsInBand--;
    }

    i = firstInNextBand;
  }

  return !rects->empty();
}

rfb::Rect rfb::Region::get_bounding_rect() const {
  return Rect(xrgn->extents.x1, xrgn->extents.y1,
              xrgn->extents.x2, xrgn->extents.y2);
}


void rfb::Region::debug_print(const char* prefix) const
{
  fprintf(stderr,"%s num rects %3ld extents %3d,%3d %3dx%3d\n",
          prefix, xrgn->numRects, xrgn->extents.x1, xrgn->extents.y1,
          xrgn->extents.x2-xrgn->extents.x1,
          xrgn->extents.y2-xrgn->extents.y1);

  for (int i = 0; i < xrgn->numRects; i++) {
    fprintf(stderr,"    rect %3d,%3d %3dx%3d\n",
            xrgn->rects[i].x1, xrgn->rects[i].y1,
            xrgn->rects[i].x2-xrgn->rects[i].x1,
            xrgn->rects[i].y2-xrgn->rects[i].y1);
  }
}
