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
#include <rdr/InStream.h>
#include <rfb/CMsgReader.h>
#include <rfb/CMsgHandler.h>
#include <rfb/RawDecoder.h>

using namespace rfb;

Decoder* RawDecoder::create(CMsgReader* reader)
{
  return new RawDecoder(reader);
}

RawDecoder::RawDecoder(CMsgReader* reader_) : reader(reader_)
{
}

RawDecoder::~RawDecoder()
{
}

void RawDecoder::readRect(const Rect& r, CMsgHandler* handler)
{
  int x = r.tl.x;
  int y = r.tl.y;
  int w = r.width();
  int h = r.height();
  int nPixels;
  rdr::U8* imageBuf = reader->getImageBuf(w, w*h, &nPixels);
  int bytesPerRow = w * (reader->bpp() / 8);
  while (h > 0) {
    int nRows = nPixels / w;
    if (nRows > h) nRows = h;
    reader->getInStream()->readBytes(imageBuf, nRows * bytesPerRow);
    handler->imageRect(Rect(x, y, x+w, y+nRows), imageBuf);
    h -= nRows;
    y += nRows;
  }
}
