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
#include <rfb/SMsgReaderV3.h>
#include <rfb/SMsgHandler.h>

#pragma warning(disable: 4800)

using namespace rfb;

SMsgReaderV3::SMsgReaderV3(SMsgHandler* handler, rdr::InStream* is)
  : SMsgReader(handler, is)
{
}

SMsgReaderV3::~SMsgReaderV3()
{
}

void SMsgReaderV3::readClientInit()
{
  bool shared = is->readU8();
  handler->clientInit(shared);
}

void SMsgReaderV3::readMsg()
{
  int msgType = is->readU8();
  switch (msgType) {
  case msgTypeSetPixelFormat:           readSetPixelFormat(); break;
  case msgTypeSetEncodings:             readSetEncodings(); break;
  case msgTypeFramebufferUpdateRequest: readFramebufferUpdateRequest(); break;
  case msgTypeKeyEvent:                 readKeyEvent(); break;
  case msgTypePointerEvent:             readPointerEvent(); break;
  case msgTypeClientCutText:            readClientCutText(); break;
  default:
    fprintf(stderr, "unknown message type %d\n", msgType);
    throw Exception("unknown message type");
  }
}
