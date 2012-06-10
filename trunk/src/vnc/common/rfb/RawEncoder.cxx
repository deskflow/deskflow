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
#include <rdr/OutStream.h>
#include <rfb/ImageGetter.h>
#include <rfb/encodings.h>
#include <rfb/SMsgWriter.h>
#include <rfb/RawEncoder.h>

using namespace rfb;

Encoder* RawEncoder::create(SMsgWriter* writer)
{
  return new RawEncoder(writer);
}

RawEncoder::RawEncoder(SMsgWriter* writer_) : writer(writer_)
{
}

RawEncoder::~RawEncoder()
{
}

bool RawEncoder::writeRect(const Rect& r, ImageGetter* ig, Rect* actual)
{
  int x = r.tl.x;
  int y = r.tl.y;
  int w = r.width();
  int h = r.height();
  int nPixels;
  rdr::U8* imageBuf = writer->getImageBuf(w, w*h, &nPixels);
  int bytesPerRow = w * (writer->bpp() / 8);
  writer->startRect(r, encodingRaw);
  while (h > 0) {
    int nRows = nPixels / w;
    if (nRows > h) nRows = h;
    ig->getImage(imageBuf, Rect(x, y, x+w, y+nRows));
    writer->getOutStream()->writeBytes(imageBuf, nRows * bytesPerRow);
    h -= nRows;
    y += nRows;
  }
  writer->endRect();
  return true;
}
