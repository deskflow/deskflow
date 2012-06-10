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
#include <rfb/RREDecoder.h>

using namespace rfb;

#define EXTRA_ARGS CMsgHandler* handler
#define FILL_RECT(r, p) handler->fillRect(r, p)
#define IMAGE_RECT(r, p) handler->imageRect(r, p)
#define BPP 8
#include <rfb/rreDecode.h>
#undef BPP
#define BPP 16
#include <rfb/rreDecode.h>
#undef BPP
#define BPP 32
#include <rfb/rreDecode.h>
#undef BPP

Decoder* RREDecoder::create(CMsgReader* reader)
{
  return new RREDecoder(reader);
}

RREDecoder::RREDecoder(CMsgReader* reader_) : reader(reader_)
{
}

RREDecoder::~RREDecoder()
{
}

void RREDecoder::readRect(const Rect& r, CMsgHandler* handler)
{
  rdr::InStream* is = reader->getInStream();
  switch (reader->bpp()) {
  case 8:  rreDecode8 (r, is, handler); break;
  case 16: rreDecode16(r, is, handler); break;
  case 32: rreDecode32(r, is, handler); break;
  }
}
