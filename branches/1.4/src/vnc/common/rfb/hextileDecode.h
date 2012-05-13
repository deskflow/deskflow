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
// Hextile decoding function.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// FILL_RECT          - fill a rectangle with a single colour
// IMAGE_RECT         - draw a rectangle of pixel data from a buffer

#include <rdr/InStream.h>
#include <rfb/hextileConstants.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define READ_PIXEL CONCAT2E(readOpaque,BPP)
#define HEXTILE_DECODE CONCAT2E(hextileDecode,BPP)

void HEXTILE_DECODE (const Rect& r, rdr::InStream* is, PIXEL_T* buf
#ifdef EXTRA_ARGS
                     , EXTRA_ARGS
#endif
                     )
{
  Rect t;
  PIXEL_T bg = 0;
  PIXEL_T fg = 0;

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

    t.br.y = __rfbmin(r.br.y, t.tl.y + 16);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {

      t.br.x = __rfbmin(r.br.x, t.tl.x + 16);

      int tileType = is->readU8();

      if (tileType & hextileRaw) {
	is->readBytes(buf, t.area() * (BPP/8));
	IMAGE_RECT(t, buf);
	continue;
      }

      if (tileType & hextileBgSpecified)
	bg = is->READ_PIXEL();

#ifdef FAVOUR_FILL_RECT
      FILL_RECT(t, bg);
#else
      int len = t.area();
      PIXEL_T* ptr = (PIXEL_T*)buf;
      while (len-- > 0) *ptr++ = bg;
#endif

      if (tileType & hextileFgSpecified)
	fg = is->READ_PIXEL();

      if (tileType & hextileAnySubrects) {
        int nSubrects = is->readU8();

        for (int i = 0; i < nSubrects; i++) {

          if (tileType & hextileSubrectsColoured)
            fg = is->READ_PIXEL();

          int xy = is->readU8();
          int wh = is->readU8();

#ifdef FAVOUR_FILL_RECT
          Rect s;
          s.tl.x = t.tl.x + ((xy >> 4) & 15);
          s.tl.y = t.tl.y + (xy & 15);
          s.br.x = s.tl.x + ((wh >> 4) & 15) + 1;
          s.br.y = s.tl.y + (wh & 15) + 1;
          FILL_RECT(s, fg);
#else
          int x = ((xy >> 4) & 15);
          int y = (xy & 15);
          int w = ((wh >> 4) & 15) + 1;
          int h = (wh & 15) + 1;
          PIXEL_T* ptr = (PIXEL_T*)buf + y * t.width() + x;
          int rowAdd = t.width() - w;
          while (h-- > 0) {
            int len = w;
            while (len-- > 0) *ptr++ = fg;
            ptr += rowAdd;
          }
#endif
        }
      }
#ifndef FAVOUR_FILL_RECT
      IMAGE_RECT(t, buf);
#endif
    }
  }
}

#undef PIXEL_T
#undef READ_PIXEL
#undef HEXTILE_DECODE
}
