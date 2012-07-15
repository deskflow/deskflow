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

// rfb::Rect and rfb::Point structures

#ifndef __RFB_RECT_INCLUDED__
#define __RFB_RECT_INCLUDED__

// Some platforms (e.g. Windows) include max() and min() macros in their
// standard headers, but they are also standard C++ template functions, so some
// C++ headers will undefine them.  So we steer clear of the names min and max
// and define __rfbmin and __rfbmax instead.

#ifndef __rfbmax
#define __rfbmax(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __rfbmin
#define __rfbmin(a,b) (((a) < (b)) ? (a) : (b))
#endif

namespace rfb {

  // rfb::Point
  //
  // Represents a point in 2D space, by X and Y coordinates.
  // Can also be used to represent a delta, or offset, between
  // two Points.
  // Functions are provided to allow Points to be compared for
  // equality and translated by a supplied offset.
  // Functions are also provided to negate offset Points.

  struct Point {
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
    inline Point negate() const {return Point(-x, -y);}
    inline bool equals(const Point &p) const {return x==p.x && y==p.y;}
    inline Point translate(const Point &p) const {return Point(x+p.x, y+p.y);}
    inline Point subtract(const Point &p) const {return Point(x-p.x, y-p.y);}
    int x, y;
  };

  // rfb::Rect
  //
  // Represents a rectangular region defined by its top-left (tl)
  // and bottom-right (br) Points.
  // Rects may be compared for equality, checked to determine whether
  // or not they are empty, cleared (made empty), or intersected with
  // one another.  The bounding rectangle of two existing Rects
  // may be calculated, as may the area of a Rect.
  // Rects may also be translated, in the same way as Points, by
  // an offset specified in a Point structure.

  struct Rect {
    Rect() {}
    Rect(Point tl_, Point br_) : tl(tl_), br(br_) {}
    Rect(int x1, int y1, int x2, int y2) : tl(x1, y1), br(x2, y2) {}
    inline void setXYWH(int x, int y, int w, int h) {
      tl.x = x; tl.y = y; br.x = x+w; br.y = y+h;
    }
    inline Rect intersect(const Rect &r) const {
      Rect result;
      result.tl.x = __rfbmax(tl.x, r.tl.x);
      result.tl.y = __rfbmax(tl.y, r.tl.y);
      result.br.x = __rfbmax(__rfbmin(br.x, r.br.x), result.tl.x);
      result.br.y = __rfbmax(__rfbmin(br.y, r.br.y), result.tl.y);
      return result;
    }
    inline Rect union_boundary(const Rect &r) const {
      if (r.is_empty()) return *this;
      if (is_empty()) return r;
      Rect result;
      result.tl.x = __rfbmin(tl.x, r.tl.x);
      result.tl.y = __rfbmin(tl.y, r.tl.y);
      result.br.x = __rfbmax(br.x, r.br.x);
      result.br.y = __rfbmax(br.y, r.br.y);
      return result;
    }
    inline Rect translate(const Point &p) const {
      return Rect(tl.translate(p), br.translate(p));
    }
    inline bool equals(const Rect &r) const {return r.tl.equals(tl) && r.br.equals(br);}
    inline bool is_empty() const {return (tl.x >= br.x) || (tl.y >= br.y);}
    inline void clear() {tl = Point(); br = Point();}
    inline bool enclosed_by(const Rect &r) const {
      return (tl.x>=r.tl.x) && (tl.y>=r.tl.y) && (br.x<=r.br.x) && (br.y<=r.br.y);
    }
    inline bool overlaps(const Rect &r) const {
      return tl.x < r.br.x && tl.y < r.br.y && br.x > r.tl.x && br.y > r.tl.y;
    }
    inline unsigned int area() const {return is_empty() ? 0 : (br.x-tl.x)*(br.y-tl.y);}
    inline Point dimensions() const {return Point(width(), height());}
    inline int width() const {return br.x-tl.x;}
    inline int height() const {return br.y-tl.y;}
    inline bool contains(const Point &p) const {
      return (tl.x<=p.x) && (tl.y<=p.y) && (br.x>p.x) && (br.y>p.y);
    }
    Point tl;
    Point br;
  };
}
#endif // __RFB_RECT_INCLUDED__
