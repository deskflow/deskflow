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
// RRE decoding function.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// FILL_RECT          - fill a rectangle with a single colour

#include <rdr/InStream.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define READ_PIXEL CONCAT2E(readOpaque,BPP)
#define RRE_DECODE CONCAT2E(rreDecode,BPP)

void RRE_DECODE (const Rect& r, rdr::InStream* is
#ifdef EXTRA_ARGS
                 , EXTRA_ARGS
#endif
                 )
{
  int nSubrects = is->readU32();
  PIXEL_T bg = is->READ_PIXEL();
  FILL_RECT(r, bg);

  for (int i = 0; i < nSubrects; i++) {
    PIXEL_T pix = is->READ_PIXEL();
    int x = is->readU16();
    int y = is->readU16();
    int w = is->readU16();
    int h = is->readU16();
    FILL_RECT(Rect(r.tl.x+x, r.tl.y+y, r.tl.x+x+w, r.tl.y+y+h), pix);
  }
}

#undef PIXEL_T
#undef READ_PIXEL
#undef RRE_DECODE
}
