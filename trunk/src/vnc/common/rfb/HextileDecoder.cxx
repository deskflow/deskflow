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
#include <rfb/HextileDecoder.h>

using namespace rfb;

#define EXTRA_ARGS CMsgHandler* handler
#define FILL_RECT(r, p) handler->fillRect(r, p)
#define IMAGE_RECT(r, p) handler->imageRect(r, p)
#define BPP 8
#include <rfb/hextileDecode.h>
#undef BPP
#define BPP 16
#include <rfb/hextileDecode.h>
#undef BPP
#define BPP 32
#include <rfb/hextileDecode.h>
#undef BPP

Decoder* HextileDecoder::create(CMsgReader* reader)
{
  return new HextileDecoder(reader);
}

HextileDecoder::HextileDecoder(CMsgReader* reader_) : reader(reader_)
{
}

HextileDecoder::~HextileDecoder()
{
}

void HextileDecoder::readRect(const Rect& r, CMsgHandler* handler)
{
  rdr::InStream* is = reader->getInStream();
  rdr::U8* buf = reader->getImageBuf(16 * 16 * 4);
  switch (reader->bpp()) {
  case 8:  hextileDecode8 (r, is, (rdr::U8*) buf, handler); break;
  case 16: hextileDecode16(r, is, (rdr::U16*)buf, handler); break;
  case 32: hextileDecode32(r, is, (rdr::U32*)buf, handler); break;
  }
}
