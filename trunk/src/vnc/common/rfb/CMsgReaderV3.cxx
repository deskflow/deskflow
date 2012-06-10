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
#include <rfb/PixelFormat.h>
#include <rfb/msgTypes.h>
#include <rfb/Exception.h>
#include <rdr/InStream.h>
#include <rfb/CMsgReaderV3.h>
#include <rfb/CMsgHandler.h>
#include <rfb/util.h>

using namespace rfb;

CMsgReaderV3::CMsgReaderV3(CMsgHandler* handler, rdr::InStream* is)
  : CMsgReader(handler, is), nUpdateRectsLeft(0)
{
}

CMsgReaderV3::~CMsgReaderV3()
{
}

void CMsgReaderV3::readServerInit()
{
  int width = is->readU16();
  int height = is->readU16();
  handler->setDesktopSize(width, height);
  PixelFormat pf;
  pf.read(is);
  handler->setPixelFormat(pf);
  CharArray name(is->readString());
  handler->setName(name.buf);
  handler->serverInit();
}

void CMsgReaderV3::readMsg()
{
  if (nUpdateRectsLeft == 0) {

    int type = is->readU8();
    switch (type) {
    case msgTypeFramebufferUpdate:   readFramebufferUpdate(); break;
    case msgTypeSetColourMapEntries: readSetColourMapEntries(); break;
    case msgTypeBell:                readBell(); break;
    case msgTypeServerCutText:       readServerCutText(); break;
    default:
      fprintf(stderr, "unknown message type %d\n", type);
      throw Exception("unknown message type");
    }

  } else {

    int x = is->readU16();
    int y = is->readU16();
    int w = is->readU16();
    int h = is->readU16();
    unsigned int encoding = is->readU32();

    switch (encoding) {
    case pseudoEncodingDesktopSize:
      handler->setDesktopSize(w, h);
      break;
    case pseudoEncodingCursor:
      readSetCursor(w, h, Point(x,y));
      break;
    default:
      readRect(Rect(x, y, x+w, y+h), encoding);
      break;
    };

    nUpdateRectsLeft--;
    if (nUpdateRectsLeft == 0) handler->framebufferUpdateEnd();
  }
}

void CMsgReaderV3::readFramebufferUpdate()
{
  is->skip(1);
  nUpdateRectsLeft = is->readU16();
  handler->framebufferUpdateStart();
}
