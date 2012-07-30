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
// ZRLE decoding function.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// FILL_RECT          - fill a rectangle with a single colour
// IMAGE_RECT         - draw a rectangle of pixel data from a buffer

#include <rdr/InStream.h>
#include <rdr/ZlibInStream.h>
#include <assert.h>

#pragma warning(disable: 4800)

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#ifdef CPIXEL
#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define READ_PIXEL CONCAT2E(readOpaque,CPIXEL)
#define ZRLE_DECODE CONCAT2E(zrleDecode,CPIXEL)
#else
#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define READ_PIXEL CONCAT2E(readOpaque,BPP)
#define ZRLE_DECODE CONCAT2E(zrleDecode,BPP)
#endif

void ZRLE_DECODE (const Rect& r, rdr::InStream* is,
                      rdr::ZlibInStream* zis, PIXEL_T* buf
#ifdef EXTRA_ARGS
                      , EXTRA_ARGS
#endif
                      )
{
  int length = is->readU32();
  zis->setUnderlying(is, length);
  Rect t;

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 64) {

    t.br.y = __rfbmin(r.br.y, t.tl.y + 64);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 64) {

      t.br.x = __rfbmin(r.br.x, t.tl.x + 64);

      int mode = zis->readU8();
      bool rle = mode & 128;
      int palSize = mode & 127;
      PIXEL_T palette[128];

      for (int i = 0; i < palSize; i++) {
        palette[i] = zis->READ_PIXEL();
      }

      if (palSize == 1) {
        PIXEL_T pix = palette[0];
        FILL_RECT(t,pix);
        continue;
      }

      if (!rle) {
        if (palSize == 0) {

          // raw

#ifdef CPIXEL
          for (PIXEL_T* ptr = buf; ptr < buf+t.area(); ptr++) {
            *ptr = zis->READ_PIXEL();
          }
#else
          zis->readBytes(buf, t.area() * (BPP / 8));
#endif

        } else {

          // packed pixels
          int bppp = ((palSize > 16) ? 8 :
                      ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));

          PIXEL_T* ptr = buf;

          for (int i = 0; i < t.height(); i++) {
            PIXEL_T* eol = ptr + t.width();
            rdr::U8 byte = 0;
            rdr::U8 nbits = 0;

            while (ptr < eol) {
              if (nbits == 0) {
                byte = zis->readU8();
                nbits = 8;
              }
              nbits -= bppp;
              rdr::U8 index = (byte >> nbits) & ((1 << bppp) - 1) & 127;
              *ptr++ = palette[index];
            }
          }
        }

#ifdef FAVOUR_FILL_RECT
       //fprintf(stderr,"copying data to screen %dx%d at %d,%d\n",
        //t.width(),t.height(),t.tl.x,t.tl.y);
        IMAGE_RECT(t,buf);
#endif

      } else {

        if (palSize == 0) {

          // plain RLE

          PIXEL_T* ptr = buf;
          PIXEL_T* end = ptr + t.area();
          while (ptr < end) {
            PIXEL_T pix = zis->READ_PIXEL();
            int len = 1;
            int b;
            do {
              b = zis->readU8();
              len += b;
            } while (b == 255);

            assert(len <= end - ptr);

#ifdef FAVOUR_FILL_RECT
            int i = ptr - buf;
            ptr += len;

            int runX = i % t.width();
            int runY = i / t.width();

            if (runX + len > t.width()) {
              if (runX != 0) {
                FILL_RECT(Rect(t.tl.x+runX, t.tl.y+runY, t.width()-runX, 1),
                          pix);
                len -= t.width()-runX;
                runX = 0;
                runY++;
              }

              if (len > t.width()) {
                FILL_RECT(Rect(t.tl.x, t.tl.y+runY, t.width(), len/t.width()),
                          pix);
                runY += len / t.width();
                len = len % t.width();
              }
            }

            if (len != 0) {
              FILL_RECT(Rect(t.tl.x+runX, t.tl.y+runY, len, 1), pix);
            }
#else
            while (len-- > 0) *ptr++ = pix;
#endif

          }
        } else {

          // palette RLE

          PIXEL_T* ptr = buf;
          PIXEL_T* end = ptr + t.area();
          while (ptr < end) {
            int index = zis->readU8();
            int len = 1;
            if (index & 128) {
              int b;
              do {
                b = zis->readU8();
                len += b;
              } while (b == 255);

              assert(len <= end - ptr);
            }

            index &= 127;

            PIXEL_T pix = palette[index];

#ifdef FAVOUR_FILL_RECT
            int i = ptr - buf;
            ptr += len;

            int runX = i % t.width();
            int runY = i / t.width();

            if (runX + len > t.width()) {
              if (runX != 0) {
                FILL_RECT(Rect(t.tl.x+runX, t.tl.y+runY, t.width()-runX, 1),
                          pix);
                len -= t.width()-runX;
                runX = 0;
                runY++;
              }

              if (len > t.width()) {
                FILL_RECT(Rect(t.tl.x, t.tl.y+runY, t.width(), len/t.width()),
                          pix);
                runY += len / t.width();
                len = len % t.width();
              }
            }

            if (len != 0) {
              FILL_RECT(Rect(t.tl.x+runX, t.tl.y+runY, len, 1), pix);
            }
#else
            while (len-- > 0) *ptr++ = pix;
#endif
          }
        }
      }

#ifndef FAVOUR_FILL_RECT
      //fprintf(stderr,"copying data to screen %dx%d at %d,%d\n",
      //t.width(),t.height(),t.tl.x,t.tl.y);
      IMAGE_RECT(t,buf);
#endif
    }
  }

  zis->reset();
}

#undef ZRLE_DECODE
#undef READ_PIXEL
#undef PIXEL_T
}
