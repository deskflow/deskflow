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
#ifndef __RFB_COLOURMAP_H__
#define __RFB_COLOURMAP_H__
namespace rfb {
  struct Colour {
    Colour() : r(0), g(0), b(0) {}
    Colour(int r_, int g_, int b_) : r(r_), g(g_), b(b_) {}
    int r, g, b;
    bool operator==(const Colour& c) const {return c.r == r && c.g == g && c.b == b;}
    bool operator!=(const Colour& c) const {return !(c == *this);}
  };

  class ColourMap {
  public:
    virtual void lookup(int index, int* r, int* g, int* b)=0;
  };
}
#endif
