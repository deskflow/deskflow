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
// PixelFormat - structure to represent a pixel format.  Also has useful
// methods for reading & writing to streams, etc.
//

#ifndef __RFB_PIXELFORMAT_H__
#define __RFB_PIXELFORMAT_H__

#include <rfb/Pixel.h>
#include <rfb/ColourMap.h>

namespace rdr { class InStream; class OutStream; }

namespace rfb {

  class PixelFormat {
  public:
    PixelFormat(int b, int d, bool e, bool t,
                int rm=0, int gm=0, int bm=0, int rs=0, int gs=0, int bs=0);
    PixelFormat();
    bool equal(const PixelFormat& other) const;
    void read(rdr::InStream* is);
    void write(rdr::OutStream* os) const;
    Pixel pixelFromRGB(rdr::U16 red, rdr::U16 green, rdr::U16 blue, ColourMap* cm=0) const;
    void rgbFromPixel(Pixel pix, ColourMap* cm, Colour* rgb) const;
    void print(char* str, int len) const;
    bool parse(const char* str);

    int bpp;
    int depth;
    bool bigEndian;
    bool trueColour;
    int redMax;
    int greenMax;
    int blueMax;
    int redShift;
    int greenShift;
    int blueShift;
  };
}
#endif
