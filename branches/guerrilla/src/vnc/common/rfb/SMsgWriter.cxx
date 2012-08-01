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
#include <assert.h>
#include <rdr/OutStream.h>
#include <rfb/msgTypes.h>
#include <rfb/ColourMap.h>
#include <rfb/ConnParams.h>
#include <rfb/UpdateTracker.h>
#include <rfb/SMsgWriter.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("SMsgWriter");

SMsgWriter::SMsgWriter(ConnParams* cp_, rdr::OutStream* os_)
  : imageBufIdealSize(0), cp(cp_), os(os_), lenBeforeRect(0),
    currentEncoding(0), updatesSent(0), rawBytesEquivalent(0),
    imageBuf(0), imageBufSize(0)
{
  for (unsigned int i = 0; i <= encodingMax; i++) {
    encoders[i] = 0;
    bytesSent[i] = 0;
    rectsSent[i] = 0;
  }
}

SMsgWriter::~SMsgWriter()
{
  vlog.info("framebuffer updates %d",updatesSent);
  int bytes = 0;
  for (unsigned int i = 0; i <= encodingMax; i++) {
    delete encoders[i];
    if (i != encodingCopyRect)
      bytes += bytesSent[i];
    if (rectsSent[i])
      vlog.info("  %s rects %d, bytes %d",
                encodingName(i), rectsSent[i], bytesSent[i]);
  }
  vlog.info("  raw bytes equivalent %d, compression ratio %f",
          rawBytesEquivalent, (double)rawBytesEquivalent / bytes);
  delete [] imageBuf;
}

void SMsgWriter::writeSetColourMapEntries(int firstColour, int nColours,
                                          ColourMap* cm)
{
  startMsg(msgTypeSetColourMapEntries);
  os->pad(1);
  os->writeU16(firstColour);
  os->writeU16(nColours);
  for (int i = firstColour; i < firstColour+nColours; i++) {
    int r, g, b;
    cm->lookup(i, &r, &g, &b);
    os->writeU16(r);
    os->writeU16(g);
    os->writeU16(b);
  }
  endMsg();
}

void SMsgWriter::writeBell()
{
  startMsg(msgTypeBell);
  endMsg();
}

void SMsgWriter::writeServerCutText(const char* str, int len)
{
  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeU32(len);
  os->writeBytes(str, len);
  endMsg();
}

void SMsgWriter::writeFramebufferUpdate(const UpdateInfo& ui, ImageGetter* ig,
                                        Region* updatedRegion)
{
  writeFramebufferUpdateStart(ui.numRects());
  writeRects(ui, ig, updatedRegion);
  writeFramebufferUpdateEnd();
}

void SMsgWriter::writeRects(const UpdateInfo& ui, ImageGetter* ig,
                            Region* updatedRegion)
{
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator i;
  updatedRegion->copyFrom(ui.changed);
  updatedRegion->assign_union(ui.copied);

  ui.copied.get_rects(&rects, ui.copy_delta.x <= 0, ui.copy_delta.y <= 0);
  for (i = rects.begin(); i != rects.end(); i++)
    writeCopyRect(*i, i->tl.x - ui.copy_delta.x, i->tl.y - ui.copy_delta.y);

  ui.changed.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    Rect actual;
    if (!writeRect(*i, ig, &actual)) {
      updatedRegion->assign_subtract(*i);
      updatedRegion->assign_union(actual);
    }
  }
}


bool SMsgWriter::needFakeUpdate()
{
  return false;
}

bool SMsgWriter::writeRect(const Rect& r, ImageGetter* ig, Rect* actual)
{
  return writeRect(r, cp->currentEncoding(), ig, actual);
}

bool SMsgWriter::writeRect(const Rect& r, unsigned int encoding,
                           ImageGetter* ig, Rect* actual)
{
  if (!encoders[encoding]) {
    encoders[encoding] = Encoder::createEncoder(encoding, this);
    assert(encoders[encoding]);
  }
  return encoders[encoding]->writeRect(r, ig, actual);
}

void SMsgWriter::writeCopyRect(const Rect& r, int srcX, int srcY)
{
  startRect(r,encodingCopyRect);
  os->writeU16(srcX);
  os->writeU16(srcY);
  endRect();
}

rdr::U8* SMsgWriter::getImageBuf(int required, int requested, int* nPixels)
{
  int requiredBytes = required * (cp->pf().bpp / 8);
  int requestedBytes = requested * (cp->pf().bpp / 8);
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
    *nPixels = imageBufSize / (cp->pf().bpp / 8);
  return imageBuf;
}

int SMsgWriter::bpp()
{
  return cp->pf().bpp;
}
