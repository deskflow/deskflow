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
#include <rfb/RREEncoder.h>

using namespace rfb;

#define BPP 8
#include <rfb/rreEncode.h>
#undef BPP
#define BPP 16
#include <rfb/rreEncode.h>
#undef BPP
#define BPP 32
#include <rfb/rreEncode.h>
#undef BPP

Encoder* RREEncoder::create(SMsgWriter* writer)
{
  return new RREEncoder(writer);
}

RREEncoder::RREEncoder(SMsgWriter* writer_) : writer(writer_)
{
}

RREEncoder::~RREEncoder()
{
}

bool RREEncoder::writeRect(const Rect& r, ImageGetter* ig, Rect* actual)
{
  int w = r.width();
  int h = r.height();
  rdr::U8* imageBuf = writer->getImageBuf(w*h);
  ig->getImage(imageBuf, r);

  mos.clear();

  int nSubrects = -1;
  switch (writer->bpp()) {
  case 8:  nSubrects = rreEncode8(imageBuf, w, h, &mos);  break;
  case 16: nSubrects = rreEncode16(imageBuf, w, h, &mos); break;
  case 32: nSubrects = rreEncode32(imageBuf, w, h, &mos); break;
  }
  
  if (nSubrects < 0) {
    return writer->writeRect(r, encodingRaw, ig, actual);
  }

  writer->startRect(r, encodingRRE);
  rdr::OutStream* os = writer->getOutStream();
  os->writeU32(nSubrects);
  os->writeBytes(mos.data(), mos.length());
  writer->endRect();
  return true;
}
