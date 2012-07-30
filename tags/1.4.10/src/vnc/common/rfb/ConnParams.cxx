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
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/Exception.h>
#include <rfb/encodings.h>
#include <rfb/Encoder.h>
#include <rfb/ConnParams.h>
#include <rfb/util.h>

using namespace rfb;

ConnParams::ConnParams()
  : majorVersion(0), minorVersion(0), width(0), height(0), useCopyRect(false),
    supportsLocalCursor(false), supportsDesktopResize(true),
    name_(0), nEncodings_(0), encodings_(0),
    currentEncoding_(encodingRaw), verStrPos(0)
{
  setName("");
}

ConnParams::~ConnParams()
{
  delete [] name_;
  delete [] encodings_;
}

bool ConnParams::readVersion(rdr::InStream* is, bool* done)
{
  if (verStrPos >= 12) return false;
  while (is->checkNoWait(1) && verStrPos < 12) {
    verStr[verStrPos++] = is->readU8();
  }

  if (verStrPos < 12) {
    *done = false;
    return true;
  }
  *done = true;
  verStr[12] = 0;
  return (sscanf(verStr, "RFB %03d.%03d\n", &majorVersion,&minorVersion) == 2);
}

void ConnParams::writeVersion(rdr::OutStream* os)
{
  char str[13];
  sprintf(str, "RFB %03d.%03d\n", majorVersion, minorVersion);
  os->writeBytes(str, 12);
  os->flush();
}

void ConnParams::setPF(const PixelFormat& pf)
{
  pf_ = pf;

  if (pf.bpp != 8 && pf.bpp != 16 && pf.bpp != 32)
    throw Exception("setPF: not 8, 16 or 32 bpp?");
}

void ConnParams::setName(const char* name)
{
  delete [] name_;
  name_ = strDup(name);
}

void ConnParams::setEncodings(int nEncodings, const rdr::U32* encodings)
{
  if (nEncodings > nEncodings_) {
    delete [] encodings_;
    encodings_ = new rdr::U32[nEncodings];
  }
  nEncodings_ = nEncodings;
  useCopyRect = false;
  supportsLocalCursor = false;
  supportsDesktopResize = false;
  currentEncoding_ = encodingRaw;

  for (int i = nEncodings-1; i >= 0; i--) {
    encodings_[i] = encodings[i];

    if (encodings[i] == encodingCopyRect)
      useCopyRect = true;
    else if (encodings[i] == pseudoEncodingCursor)
      supportsLocalCursor = true;
    else if (encodings[i] == pseudoEncodingDesktopSize)
      supportsDesktopResize = true;
    else if (encodings[i] <= encodingMax && Encoder::supported(encodings[i]))
      currentEncoding_ = encodings[i];
  }
}
