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
#include <string.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/PixelFormat.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#pragma warning(disable: 4800)

using namespace rfb;

PixelFormat::PixelFormat(int b, int d, bool e, bool t,
                         int rm, int gm, int bm, int rs, int gs, int bs)
  : bpp(b), depth(d), bigEndian(e), trueColour(t),
    redMax(rm), greenMax(gm), blueMax(bm),
    redShift(rs), greenShift(gs), blueShift(bs)
{
}

PixelFormat::PixelFormat()
  : bpp(8), depth(8), bigEndian(false), trueColour(true),
    redMax(7), greenMax(7), blueMax(3),
    redShift(0), greenShift(3), blueShift(6)
{
}

bool PixelFormat::equal(const PixelFormat& other) const
{
  return (bpp == other.bpp &&
          depth == other.depth &&
          (bigEndian == other.bigEndian || bpp == 8) &&
          trueColour == other.trueColour &&
          (!trueColour || (redMax == other.redMax &&
                           greenMax == other.greenMax &&
                           blueMax == other.blueMax &&
                           redShift == other.redShift &&
                           greenShift == other.greenShift &&
                           blueShift == other.blueShift)));
}

void PixelFormat::read(rdr::InStream* is)
{
  bpp = is->readU8();
  depth = is->readU8();
  bigEndian = is->readU8();
  trueColour = is->readU8();
  redMax = is->readU16();
  greenMax = is->readU16();
  blueMax = is->readU16();
  redShift = is->readU8();
  greenShift = is->readU8();
  blueShift = is->readU8();
  is->skip(3);
}

void PixelFormat::write(rdr::OutStream* os) const
{
  os->writeU8(bpp);
  os->writeU8(depth);
  os->writeU8(bigEndian);
  os->writeU8(trueColour);
  os->writeU16(redMax);
  os->writeU16(greenMax);
  os->writeU16(blueMax);
  os->writeU8(redShift);
  os->writeU8(greenShift);
  os->writeU8(blueShift);
  os->pad(3);
}

Pixel PixelFormat::pixelFromRGB(rdr::U16 red, rdr::U16 green, rdr::U16 blue,
                                ColourMap* cm) const
{
  if (trueColour) {
    rdr::U32 r = ((rdr::U32)red   * redMax   + 32767) / 65535;
    rdr::U32 g = ((rdr::U32)green * greenMax + 32767) / 65535;
    rdr::U32 b = ((rdr::U32)blue  * blueMax  + 32767) / 65535;

    return (r << redShift) | (g << greenShift) | (b << blueShift);
  } else if (cm) {
    // Try to find the closest pixel by Cartesian distance
    int colours = 1 << depth;
    int diff = 256 * 256 * 4;
    int col = 0;
    for (int i=0; i<colours; i++) {
      int r, g, b;
      cm->lookup(i, &r, &g, &b);
      int rd = (r-red) >> 8;
      int gd = (g-green) >> 8;
      int bd = (b-blue) >> 8;
      int d = rd*rd + gd*gd + bd*bd;
      if (d < diff) {
        col = i;
        diff = d;
      }
    }
    return col;
  }
  // XXX just return 0 for colour map?
  return 0;
}


void PixelFormat::rgbFromPixel(Pixel p, ColourMap* cm, Colour* rgb) const
{
  if (trueColour) {
    rgb->r = (((p >> redShift  ) & redMax  ) * 65535 + redMax  /2) / redMax;
    rgb->g = (((p >> greenShift) & greenMax) * 65535 + greenMax/2) / greenMax;
    rgb->b = (((p >> blueShift ) & blueMax ) * 65535 + blueMax /2) / blueMax;
  } else {
    cm->lookup(p, &rgb->r, &rgb->g, &rgb->b);
  }
}


void PixelFormat::print(char* str, int len) const
{
  // Unfortunately snprintf is not widely available so we build the string up
  // using strncat - not pretty, but should be safe against buffer overruns.

  char num[20];
  if (len < 1) return;
  str[0] = 0;
  strncat(str, "depth ", len-1-strlen(str));
  sprintf(num,"%d",depth);
  strncat(str, num, len-1-strlen(str));
  strncat(str, " (", len-1-strlen(str));
  sprintf(num,"%d",bpp);
  strncat(str, num, len-1-strlen(str));
  strncat(str, "bpp)", len-1-strlen(str));
  if (bpp != 8) {
    if (bigEndian)
      strncat(str, " big-endian", len-1-strlen(str));
    else
      strncat(str, " little-endian", len-1-strlen(str));
  }

  if (!trueColour) {
    strncat(str, " colour-map", len-1-strlen(str));
    return;
  }

  if (blueShift == 0 && greenShift > blueShift && redShift > greenShift &&
      blueMax  == (1 << greenShift) - 1 &&
      greenMax == (1 << (redShift-greenShift)) - 1 &&
      redMax   == (1 << (depth-redShift)) - 1)
  {
    strncat(str, " rgb", len-1-strlen(str));
    sprintf(num,"%d",depth-redShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",redShift-greenShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",greenShift);
    strncat(str, num, len-1-strlen(str));
    return;
  }

  if (redShift == 0 && greenShift > redShift && blueShift > greenShift &&
      redMax   == (1 << greenShift) - 1 &&
      greenMax == (1 << (blueShift-greenShift)) - 1 &&
      blueMax  == (1 << (depth-blueShift)) - 1)
  {
    strncat(str, " bgr", len-1-strlen(str));
    sprintf(num,"%d",depth-blueShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",blueShift-greenShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",greenShift);
    strncat(str, num, len-1-strlen(str));
    return;
  }

  strncat(str, " rgb max ", len-1-strlen(str));
  sprintf(num,"%d,",redMax);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d,",greenMax);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d",blueMax);
  strncat(str, num, len-1-strlen(str));
  strncat(str, " shift ", len-1-strlen(str));
  sprintf(num,"%d,",redShift);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d,",greenShift);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d",blueShift);
  strncat(str, num, len-1-strlen(str));
}


bool PixelFormat::parse(const char* str)
{
  char rgbbgr[4];
  int bits1, bits2, bits3;
  if (sscanf(str, "%3s%1d%1d%1d", rgbbgr, &bits1, &bits2, &bits3) < 4)
    return false;
  
  depth = bits1 + bits2 + bits3;
  bpp = depth <= 8 ? 8 : ((depth <= 16) ? 16 : 32);
  trueColour = true;
  rdr::U32 endianTest = 1;
  bigEndian = (*(rdr::U8*)&endianTest == 0);

  greenShift = bits3;
  greenMax = (1 << bits2) - 1;

  if (strcasecmp(rgbbgr, "bgr") == 0) {
    redShift = 0;
    redMax = (1 << bits3) - 1;
    blueShift = bits3 + bits2;
    blueMax = (1 << bits1) - 1;
  } else if (strcasecmp(rgbbgr, "rgb") == 0) {
    blueShift = 0;
    blueMax = (1 << bits3) - 1;
    redShift = bits3 + bits2;
    redMax = (1 << bits1) - 1;
  } else {
    return false;
  }
  return true;
}
