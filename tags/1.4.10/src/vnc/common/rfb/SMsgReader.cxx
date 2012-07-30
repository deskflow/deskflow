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
#include <stdio.h>
#include <rdr/InStream.h>
#include <rfb/Exception.h>
#include <rfb/util.h>
#include <rfb/SMsgHandler.h>
#include <rfb/SMsgReader.h>
#include <rfb/Configuration.h>

#pragma warning(disable: 4800)

using namespace rfb;

static IntParameter maxCutText("MaxCutText", "Maximum permitted length of an incoming clipboard update", 256*1024);

SMsgReader::SMsgReader(SMsgHandler* handler_, rdr::InStream* is_)
  : handler(handler_), is(is_)
{
}

SMsgReader::~SMsgReader()
{
}

void SMsgReader::readSetPixelFormat()
{
  is->skip(3);
  PixelFormat pf;
  pf.read(is);
  handler->setPixelFormat(pf);
}

void SMsgReader::readSetEncodings()
{
  is->skip(1);
  int nEncodings = is->readU16();
  rdr::U32Array encodings(nEncodings);
  for (int i = 0; i < nEncodings; i++)
    encodings.buf[i] = is->readU32();
  handler->setEncodings(nEncodings, encodings.buf);
}

void SMsgReader::readFramebufferUpdateRequest()
{
  bool inc = is->readU8();
  int x = is->readU16();
  int y = is->readU16();
  int w = is->readU16();
  int h = is->readU16();
  handler->framebufferUpdateRequest(Rect(x, y, x+w, y+h), inc);
}

void SMsgReader::readKeyEvent()
{
  bool down = is->readU8();
  is->skip(2);
  rdr::U32 key = is->readU32();
  handler->keyEvent(key, down);
}

void SMsgReader::readPointerEvent()
{
  int mask = is->readU8();
  int x = is->readU16();
  int y = is->readU16();
  handler->pointerEvent(Point(x, y), mask);
}


void SMsgReader::readClientCutText()
{
  is->skip(3);
  int len = is->readU32();
  if (len > maxCutText) {
    is->skip(len);
    fprintf(stderr,"cut text too long (%d bytes) - ignoring\n",len);
    return;
  }
  CharArray ca(len+1);
  ca.buf[len] = 0;
  is->readBytes(ca.buf, len);
  handler->clientCutText(ca.buf, len);
}
