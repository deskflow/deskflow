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
#include <rdr/MemOutStream.h>
#include <rfb/msgTypes.h>
#include <rfb/Exception.h>
#include <rfb/ConnParams.h>
#include <rfb/SMsgWriterV3.h>

using namespace rfb;

SMsgWriterV3::SMsgWriterV3(ConnParams* cp, rdr::OutStream* os)
  : SMsgWriter(cp, os), updateOS(0), realOS(os), nRectsInUpdate(0),
    nRectsInHeader(0), wsccb(0),
    needSetDesktopSize(false)
{
}

SMsgWriterV3::~SMsgWriterV3()
{
  delete updateOS;
}

void SMsgWriterV3::writeServerInit()
{
  os->writeU16(cp->width);
  os->writeU16(cp->height);
  cp->pf().write(os);
  os->writeString(cp->name());
  endMsg();
}

void SMsgWriterV3::startMsg(int type)
{
  if (os != realOS)
    throw Exception("startMsg called while writing an update?");

  os->writeU8(type);
}

void SMsgWriterV3::endMsg()
{
  os->flush();
}

bool SMsgWriterV3::writeSetDesktopSize() {
  if (!cp->supportsDesktopResize) return false;
  needSetDesktopSize = true;
  return true;
}

void SMsgWriterV3::cursorChange(WriteSetCursorCallback* cb)
{
  wsccb = cb;
}

void SMsgWriterV3::writeSetCursor(int width, int height, const Point& hotspot,
                                  void* data, void* mask)
{
  if (!wsccb) return;
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriterV3::writeSetCursor: nRects out of sync");
  os->writeS16(hotspot.x);
  os->writeS16(hotspot.y);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingCursor);
  os->writeBytes(data, width * height * (cp->pf().bpp/8));
  os->writeBytes(mask, (width+7)/8 * height);
}

void SMsgWriterV3::writeFramebufferUpdateStart(int nRects)
{
  startMsg(msgTypeFramebufferUpdate);
  os->pad(1);
  if (wsccb) nRects++;
  if (needSetDesktopSize) nRects++;
  os->writeU16(nRects);
  nRectsInUpdate = 0;
  nRectsInHeader = nRects;
  if (wsccb) {
    wsccb->writeSetCursorCallback();
    wsccb = 0;
  }
}

void SMsgWriterV3::writeFramebufferUpdateStart()
{
  nRectsInUpdate = nRectsInHeader = 0;
  if (!updateOS)
    updateOS = new rdr::MemOutStream;
  os = updateOS;
}

void SMsgWriterV3::writeFramebufferUpdateEnd()
{
  if (needSetDesktopSize) {
    if (!cp->supportsDesktopResize)
      throw Exception("Client does not support desktop resize");
    if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
      throw Exception("SMsgWriterV3 setDesktopSize: nRects out of sync");
    os->writeS16(0);
    os->writeS16(0);
    os->writeU16(cp->width);
    os->writeU16(cp->height);
    os->writeU32(pseudoEncodingDesktopSize);
    needSetDesktopSize = false;
  }

  if (nRectsInUpdate != nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriterV3::writeFramebufferUpdateEnd: "
                    "nRects out of sync");
  if (os == updateOS) {
    os = realOS;
    startMsg(msgTypeFramebufferUpdate);
    os->pad(1);
    os->writeU16(nRectsInUpdate);
    os->writeBytes(updateOS->data(), updateOS->length());
    updateOS->clear();
  }

  updatesSent++;
  endMsg();
}

bool SMsgWriterV3::needFakeUpdate()
{
  return wsccb || needSetDesktopSize;
}

void SMsgWriterV3::startRect(const Rect& r, unsigned int encoding)
{
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriterV3::startRect: nRects out of sync");

  currentEncoding = encoding;
  lenBeforeRect = os->length();
  if (encoding != encodingCopyRect)
    rawBytesEquivalent += 12 + r.width() * r.height() * (bpp()/8);

  os->writeS16(r.tl.x);
  os->writeS16(r.tl.y);
  os->writeU16(r.width());
  os->writeU16(r.height());
  os->writeU32(encoding);
}

void SMsgWriterV3::endRect()
{
  if (currentEncoding <= encodingMax) {
    bytesSent[currentEncoding] += os->length() - lenBeforeRect;
    rectsSent[currentEncoding]++;
  }
}
