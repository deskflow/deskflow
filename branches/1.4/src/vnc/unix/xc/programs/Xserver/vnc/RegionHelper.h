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
#ifndef __REGIONHELPER_H__
#define __REGIONHELPER_H__

// RegionHelper is a class which helps in using X server regions by
// automatically freeing them in the destructor.  It also fixes a problem with
// REGION_INIT when given an empty rectangle.

// REGION_NULL was introduced in the Xorg tree as the way to initialise an
// empty region.  If it's not already defined do it the old way.  Note that the
// old way causes a segfault in the new tree...
#ifndef REGION_NULL
#define REGION_NULL(pScreen,pReg) REGION_INIT(pScreen,pReg,NullBox,0)
#endif

class RegionHelper {
public:

  // constructor from a single rect
  RegionHelper(ScreenPtr pScreen_, BoxPtr rect, int size)
    : pScreen(pScreen_), reg(0)
  {
    init(rect, size);
  }

  // constructor from an existing X server region
  RegionHelper(ScreenPtr pScreen_, RegionPtr pRegion)
    : pScreen(pScreen_), reg(&regRec)
  {
    REGION_NULL(pScreen, reg);
    REGION_COPY(pScreen, reg, pRegion);
  }

  // constructor from an array of rectangles
  RegionHelper(ScreenPtr pScreen_, int nrects, xRectanglePtr rects,
               int ctype=CT_NONE)
    : pScreen(pScreen_)
  {
    reg = RECTS_TO_REGION(pScreen, nrects, rects, ctype);
  }

  // constructor for calling init() later
  RegionHelper(ScreenPtr pScreen_) : pScreen(pScreen_), reg(0) {
  }

  void init(BoxPtr rect, int size) {
    reg = &regRec;
    if (!rect || (rect && (rect->x2 == rect->x1 || rect->y2 == rect->y1))) {
      REGION_NULL(pScreen, reg);
    } else {
      REGION_INIT(pScreen, reg, rect, size);
    }
  }

  // destructor frees as appropriate
  ~RegionHelper() {
    if (reg == &regRec) {
      REGION_UNINIT(pScreen, reg);
    } else if (reg) {
      REGION_DESTROY(pScreen, reg);
    }
  }
  ScreenPtr pScreen;
  RegionRec regRec;
  RegionPtr reg;
};

#endif
