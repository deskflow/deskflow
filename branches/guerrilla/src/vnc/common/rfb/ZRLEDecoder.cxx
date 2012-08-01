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
#include <rfb/CMsgReader.h>
#include <rfb/CMsgHandler.h>
#include <rfb/ZRLEDecoder.h>

using namespace rfb;

#define EXTRA_ARGS CMsgHandler* handler
#define FILL_RECT(r, p) handler->fillRect(r, p)
#define IMAGE_RECT(r, p) handler->imageRect(r, p)
#define BPP 8
#include <rfb/zrleDecode.h>
#undef BPP
#define BPP 16
#include <rfb/zrleDecode.h>
#undef BPP
#define BPP 32
#include <rfb/zrleDecode.h>
#define CPIXEL 24A
#include <rfb/zrleDecode.h>
#undef CPIXEL
#define CPIXEL 24B
#include <rfb/zrleDecode.h>
#undef CPIXEL
#undef BPP

Decoder* ZRLEDecoder::create(CMsgReader* reader)
{
  return new ZRLEDecoder(reader);
}

ZRLEDecoder::ZRLEDecoder(CMsgReader* reader_) : reader(reader_)
{
}

ZRLEDecoder::~ZRLEDecoder()
{
}

void ZRLEDecoder::readRect(const Rect& r, CMsgHandler* handler)
{
  rdr::InStream* is = reader->getInStream();
  rdr::U8* buf = reader->getImageBuf(64 * 64 * 4);
  switch (reader->bpp()) {
  case 8:  zrleDecode8 (r, is, &zis, (rdr::U8*) buf, handler); break;
  case 16: zrleDecode16(r, is, &zis, (rdr::U16*)buf, handler); break;
  case 32:
    {
      const rfb::PixelFormat& pf = handler->cp.pf();
      bool fitsInLS3Bytes = ((pf.redMax   << pf.redShift)   < (1<<24) &&
                             (pf.greenMax << pf.greenShift) < (1<<24) &&
                             (pf.blueMax  << pf.blueShift)  < (1<<24));

      bool fitsInMS3Bytes = (pf.redShift   > 7  &&
                             pf.greenShift > 7  &&
                             pf.blueShift  > 7);

      if ((fitsInLS3Bytes && !pf.bigEndian) ||
          (fitsInMS3Bytes && pf.bigEndian))
      {
        zrleDecode24A(r, is, &zis, (rdr::U32*)buf, handler);
      }
      else if ((fitsInLS3Bytes && pf.bigEndian) ||
               (fitsInMS3Bytes && !pf.bigEndian))
      {
        zrleDecode24B(r, is, &zis, (rdr::U32*)buf, handler);
      }
      else
      {
        zrleDecode32(r, is, &zis, (rdr::U32*)buf, handler);
      }
      break;
    }
  }
}
