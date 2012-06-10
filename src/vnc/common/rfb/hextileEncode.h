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
// Hextile encoding function.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// GET_IMAGE_INTO_BUF - gets a rectangle of pixel data into a buffer

#include <rdr/OutStream.h>
#include <rfb/hextileConstants.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define WRITE_PIXEL CONCAT2E(writeOpaque,BPP)
#define HEXTILE_ENCODE CONCAT2E(hextileEncode,BPP)
#define HEXTILE_ENCODE_TILE CONCAT2E(hextileEncodeTile,BPP)
#define TEST_TILE_TYPE CONCAT2E(hextileTestTileType,BPP)

int TEST_TILE_TYPE (PIXEL_T* data, int w, int h, PIXEL_T* bg, PIXEL_T* fg);
int HEXTILE_ENCODE_TILE (PIXEL_T* data, int w, int h, int tileType,
                         rdr::U8* encoded, PIXEL_T bg);

void HEXTILE_ENCODE(const Rect& r, rdr::OutStream* os
#ifdef EXTRA_ARGS
                    , EXTRA_ARGS
#endif
                    )
{
  Rect t;
  PIXEL_T buf[256];
  PIXEL_T oldBg = 0, oldFg = 0;
  bool oldBgValid = false;
  bool oldFgValid = false;
  rdr::U8 encoded[256*(BPP/8)];

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

    t.br.y = __rfbmin(r.br.y, t.tl.y + 16);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {

      t.br.x = __rfbmin(r.br.x, t.tl.x + 16);

      GET_IMAGE_INTO_BUF(t,buf);

      PIXEL_T bg, fg;
      int tileType = TEST_TILE_TYPE(buf, t.width(), t.height(), &bg, &fg);

      if (!oldBgValid || oldBg != bg) {
        tileType |= hextileBgSpecified;
        oldBg = bg;
        oldBgValid = true;
      }

      int encodedLen = 0;

      if (tileType & hextileAnySubrects) {

        if (tileType & hextileSubrectsColoured) {
          oldFgValid = false;
        } else {
          if (!oldFgValid || oldFg != fg) {
            tileType |= hextileFgSpecified;
            oldFg = fg;
            oldFgValid = true;
          }
        }

        encodedLen = HEXTILE_ENCODE_TILE(buf, t.width(), t.height(), tileType,
                                         encoded, bg);

        if (encodedLen < 0) {
          GET_IMAGE_INTO_BUF(t,buf);
          os->writeU8(hextileRaw);
          os->writeBytes(buf, t.width() * t.height() * (BPP/8));
          oldBgValid = oldFgValid = false;
          continue;
        }
      }

      os->writeU8(tileType);
      if (tileType & hextileBgSpecified) os->WRITE_PIXEL(bg);
      if (tileType & hextileFgSpecified) os->WRITE_PIXEL(fg);
      if (tileType & hextileAnySubrects) os->writeBytes(encoded, encodedLen);
    }
  }
}


int HEXTILE_ENCODE_TILE (PIXEL_T* data, int w, int h, int tileType,
                         rdr::U8* encoded, PIXEL_T bg)
{
  rdr::U8* nSubrectsPtr = encoded;
  *nSubrectsPtr = 0;
  encoded++;

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

      (*nSubrectsPtr)++;

      if (tileType & hextileSubrectsColoured) {
        if (encoded - nSubrectsPtr + (BPP/8) > w*h*(BPP/8)) return -1;
#if (BPP == 8)
        *encoded++ = *data;
#elif (BPP == 16)
        *encoded++ = ((rdr::U8*)data)[0];
        *encoded++ = ((rdr::U8*)data)[1];
#elif (BPP == 32)
        *encoded++ = ((rdr::U8*)data)[0];
        *encoded++ = ((rdr::U8*)data)[1];
        *encoded++ = ((rdr::U8*)data)[2];
        *encoded++ = ((rdr::U8*)data)[3];
#endif
      }

      if (encoded - nSubrectsPtr + 2 > w*h*(BPP/8)) return -1;
      *encoded++ = (x << 4) | y;
      *encoded++ = ((sw-1) << 4) | (sh-1);

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
  return encoded - nSubrectsPtr;
}


int TEST_TILE_TYPE (PIXEL_T* data, int w, int h, PIXEL_T* bg, PIXEL_T* fg)
{
  int tileType = 0;
  PIXEL_T pix1 = *data, pix2 = 0;
  int count1 = 0, count2 = 0;
  PIXEL_T* end = data + w*h;

  for (PIXEL_T* ptr = data; ptr < end; ptr++) {
    if (*ptr == pix1) {
      count1++;
      continue;
    }

    if (count2 == 0) {
      tileType |= hextileAnySubrects;
      pix2 = *ptr;
    }

    if (*data == pix2) {
      count2++;
      continue;
    }

    tileType |= hextileSubrectsColoured;
    break;
  }

  if (count1 >= count2) {
    *bg = pix1; *fg = pix2;
  } else {
    *bg = pix2; *fg = pix1;
  }
  return tileType;
}

#undef PIXEL_T
#undef WRITE_PIXEL
#undef HEXTILE_ENCODE
#undef HEXTILE_ENCODE_TILE
#undef TEST_TILE_TYPE
}
