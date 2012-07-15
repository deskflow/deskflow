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
#include <rfb/CMsgHandler.h>
#include <rfb/CMsgReader.h>

using namespace rfb;

CMsgReader::CMsgReader(CMsgHandler* handler_, rdr::InStream* is_)
  : imageBufIdealSize(0), handler(handler_), is(is_),
    imageBuf(0), imageBufSize(0)
{
  for (unsigned int i = 0; i <= encodingMax; i++) {
    decoders[i] = 0;
  }
}

CMsgReader::~CMsgReader()
{
  for (unsigned int i = 0; i <= encodingMax; i++) {
    delete decoders[i];
  }
  delete [] imageBuf;
}

void CMsgReader::readSetColourMapEntries()
{
  is->skip(1);
  int firstColour = is->readU16();
  int nColours = is->readU16();
  rdr::U16Array rgbs(nColours * 3);
  for (int i = 0; i < nColours * 3; i++)
    rgbs.buf[i] = is->readU16();
  handler->setColourMapEntries(firstColour, nColours, rgbs.buf);
}

void CMsgReader::readBell()
{
  handler->bell();
}

void CMsgReader::readServerCutText()
{
  is->skip(3);
  int len = is->readU32();
  if (len > 256*1024) {
    is->skip(len);
    fprintf(stderr,"cut text too long (%d bytes) - ignoring\n",len);
    return;
  }
  CharArray ca(len+1);
  ca.buf[len] = 0;
  is->readBytes(ca.buf, len);
  handler->serverCutText(ca.buf, len);
}

void CMsgReader::readFramebufferUpdateStart()
{
  handler->framebufferUpdateStart();
}

void CMsgReader::readFramebufferUpdateEnd()
{
  handler->framebufferUpdateEnd();
}

void CMsgReader::readRect(const Rect& r, unsigned int encoding)
{
  if ((r.br.x > handler->cp.width) || (r.br.y > handler->cp.height)) {
    fprintf(stderr, "Rect too big: %dx%d at %d,%d exceeds %dx%d\n",
	    r.width(), r.height(), r.tl.x, r.tl.y,
            handler->cp.width, handler->cp.height);
    throw Exception("Rect too big");
  }

  if (r.is_empty())
    fprintf(stderr, "Warning: zero size rect\n");

  handler->beginRect(r, encoding);

  if (encoding == encodingCopyRect) {
    readCopyRect(r);
  } else {
    if (encoding > encodingMax)
      throw Exception("Unknown rect encoding");
    if (!decoders[encoding]) {
      decoders[encoding] = Decoder::createDecoder(encoding, this);
      if (!decoders[encoding]) {
        fprintf(stderr, "Unknown rect encoding %d\n", encoding);
        throw Exception("Unknown rect encoding");
      }
    }
    decoders[encoding]->readRect(r, handler);
  }

  handler->endRect(r, encoding);
}

void CMsgReader::readCopyRect(const Rect& r)
{
  int srcX = is->readU16();
  int srcY = is->readU16();
  handler->copyRect(r, srcX, srcY);
}

void CMsgReader::readSetCursor(int width, int height, const Point& hotspot)
{
  int data_len = width * height * (handler->cp.pf().bpp/8);
  int mask_len = ((width+7)/8) * height;
  rdr::U8Array data(data_len);
  rdr::U8Array mask(mask_len);

  is->readBytes(data.buf, data_len);
  is->readBytes(mask.buf, mask_len);

  handler->setCursor(width, height, hotspot, data.buf, mask.buf);
}

rdr::U8* CMsgReader::getImageBuf(int required, int requested, int* nPixels)
{
  int requiredBytes = required * (handler->cp.pf().bpp / 8);
  int requestedBytes = requested * (handler->cp.pf().bpp / 8);
  int size = requestedBytes;
  if (size > imageBufIdealSize) size = imageBufIdealSize;

  if (size < requiredBytes)
    size = requiredBytes;

  if (imageBufSize < size) {
    imageBufSize = size;
    delete [] imageBuf;
    imageBuf = new rdr::U8[imageBufSize];
  }
  if (nPixels)
    *nPixels = imageBufSize / (handler->cp.pf().bpp / 8);
  return imageBuf;
}

int CMsgReader::bpp()
{
  return handler->cp.pf().bpp;
}
