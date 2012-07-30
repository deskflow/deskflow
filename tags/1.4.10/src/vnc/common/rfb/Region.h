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

// Cross-platform Region class based on the X11 region implementation

#ifndef __RFB_REGION_INCLUDED__
#define __RFB_REGION_INCLUDED__

#include <rfb/Rect.h>
#include <vector>

struct _XRegion;

namespace rfb {

  struct ShortRect {
    short x1, y1, x2, y2;
  };

  class Region {
  public:
    // Create an empty region
    Region();
    // Create a rectangular region
    Region(const Rect& r);

    Region(const Region& r);
    Region &operator=(const Region& src);

    ~Region();

    // the following methods alter the region in place:

    void clear();
    void reset(const Rect& r);
    void translate(const rfb::Point& delta);
    void setOrderedRects(const std::vector<Rect>& rects);
    void setExtentsAndOrderedRects(const ShortRect* extents, int nRects,
                                   const ShortRect* rects);
    void copyFrom(const Region& r);

    void assign_intersect(const Region& r);
    void assign_union(const Region& r);
    void assign_subtract(const Region& r);

    // the following three operations return a new region:

    Region intersect(const Region& r) const;
    Region union_(const Region& r) const;
    Region subtract(const Region& r) const;

    bool equals(const Region& b) const;
    int numRects() const;
    bool is_empty() const { return numRects() == 0; }

    bool get_rects(std::vector<Rect>* rects, bool left2right=true,
                   bool topdown=true, int maxArea=0) const;
    Rect get_bounding_rect() const;

    void debug_print(const char *prefix) const;

  protected:

    struct _XRegion* xrgn;
  };

};

#endif // __RFB_REGION_INCLUDED__
