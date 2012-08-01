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
// RRE encoding function.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
//
// The data argument to RRE_ENCODE contains the pixel data, and it writes the
// encoded version to the given OutStream.  If the encoded version exceeds w*h
// it aborts and returns -1, otherwise it returns the number of subrectangles.
//

#include <rdr/OutStream.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define WRITE_PIXEL CONCAT2E(writeOpaque,BPP)
#define RRE_ENCODE CONCAT2E(rreEncode,BPP)

int RRE_ENCODE (PIXEL_T* data, int w, int h, rdr::OutStream* os, PIXEL_T bg);

int RRE_ENCODE (void* data, int w, int h, rdr::OutStream* os)
{
  // Find the background colour - count occurrences of up to 4 different pixel
  // values, and choose the one which occurs most often.

  const int nCols = 4;
  PIXEL_T pix[nCols];
  int count[nCols] = { 0, };
  PIXEL_T* ptr = (PIXEL_T*)data;
  PIXEL_T* end = ptr + w*h;

  while (ptr < end) {
    int i;
    for (i = 0; i < nCols; i++) {
      if (count[i] == 0)
        pix[i] = *ptr;

      if (pix[i] == *ptr) {
        count[i]++;
        break;
      }
    }

    if (i == nCols) break;
    ptr++;
  }
  
  int bg = 0;
  for (int i = 1; i < nCols; i++)
    if (count[i] > count[bg]) bg = i;

  // Now call the function to do the encoding.

  return RRE_ENCODE ((PIXEL_T*)data, w, h, os, pix[bg]);
}

int RRE_ENCODE (PIXEL_T* data, int w, int h, rdr::OutStream* os, PIXEL_T bg)
{
  int oldLen = os->length();
  os->WRITE_PIXEL(bg);

  int nSubrects = 0;

  for (int y = 0; y < h; y++)
  {
    int x = 0;
    while (x < w) {
      if (*data == bg) {
        x++;
        data++;
        continue;
      }

      // Find horizontal subrect first
      PIXEL_T* ptr = data+1;
      PIXEL_T* eol = data+w-x;
      while (ptr < eol && *ptr == *data) ptr++;
      int sw = ptr - data;

      ptr = data + w;
      int sh = 1;
      while (sh < h-y) {
        eol = ptr + sw;
        while (ptr < eol)
          if (*ptr++ != *data) goto endOfHorizSubrect;
        ptr += w - sw;
        sh++;
      }
    endOfHorizSubrect:

      // Find vertical subrect
      int vh;
      for (vh = sh; vh < h-y; vh++)
        if (data[vh*w] != *data) break;

      if (vh != sh) {
        ptr = data+1;
        int vw;
        for (vw = 1; vw < sw; vw++) {
          for (int i = 0; i < vh; i++)
            if (ptr[i*w] != *data) goto endOfVertSubrect;
          ptr++;
        }
      endOfVertSubrect:

        // If vertical subrect bigger than horizontal then use that.
        if (sw*sh < vw*vh) {
          sw = vw;
          sh = vh;
        }
      }

      nSubrects++;
      os->WRITE_PIXEL(*data);
      os->writeU16(x);
      os->writeU16(y);
      os->writeU16(sw);
      os->writeU16(sh);
      if (os->length() > oldLen + w*h) return -1;

      ptr = data+w;
      PIXEL_T* eor = data+w*sh;
      while (ptr < eor) {
        eol = ptr + sw;
        while (ptr < eol) *ptr++ = bg;
        ptr += w - sw;
      }
      x += sw;
      data += sw;
    }
  }

  return nSubrects;
}

#undef PIXEL_T
#undef WRITE_PIXEL
#undef RRE_ENCODE
}
