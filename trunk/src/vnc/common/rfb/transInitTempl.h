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
//
// transInitTempl.h - templates for functions to initialise lookup tables for
// the translation functions.
//
// This file is #included after having set the following macros:
// BPPOUT - 8, 16 or 32

#if !defined(BPPOUT)
#error "transInitTempl.h: BPPOUT not defined"
#endif

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#ifndef SWAP16
#define SWAP16(n) ((((n) & 0xff) << 8) | (((n) >> 8) & 0xff))
#endif

#ifndef SWAP32
#define SWAP32(n) (((n) >> 24) | (((n) & 0x00ff0000) >> 8) | \
                   (((n) & 0x0000ff00) << 8) | ((n) << 24))
#endif

#define OUTPIXEL rdr::CONCAT2E(U,BPPOUT)
#define SWAPOUT CONCAT2E(SWAP,BPPOUT)
#define initSimpleCMtoTCOUT    CONCAT2E(initSimpleCMtoTC,BPPOUT)
#define initSimpleTCtoTCOUT    CONCAT2E(initSimpleTCtoTC,BPPOUT)
#define initSimpleCMtoCubeOUT  CONCAT2E(initSimpleCMtoCube,BPPOUT)
#define initSimpleTCtoCubeOUT  CONCAT2E(initSimpleTCtoCube,BPPOUT)
#define initRGBTCtoTCOUT       CONCAT2E(initRGBTCtoTC,BPPOUT)
#define initRGBTCtoCubeOUT     CONCAT2E(initRGBTCtoCube,BPPOUT)
#define initOneRGBTableOUT     CONCAT2E(initOneRGBTable,BPPOUT)
#define initOneRGBCubeTableOUT CONCAT2E(initOneRGBCubeTable,BPPOUT)

#ifndef TRANS_INIT_TEMPL_ENDIAN_TEST
#define TRANS_INIT_TEMPL_ENDIAN_TEST
  static rdr::U32 endianTest = 1;
  static bool nativeBigEndian = *(rdr::U8*)(&endianTest) != 1;
#endif

void initSimpleCMtoTCOUT (rdr::U8** tablep, const PixelFormat& inPF,
                          ColourMap* cm, const PixelFormat& outPF)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = 1 << inPF.bpp;

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];
  OUTPIXEL* table = (OUTPIXEL*)*tablep;

  for (int i = 0; i < size; i++) {
    int r,g,b;
    cm->lookup(i,&r,&g,&b);

    table[i] = ((((r * outPF.redMax   + 32767) / 65535) << outPF.redShift) |
                (((g * outPF.greenMax + 32767) / 65535) << outPF.greenShift) |
                (((b * outPF.blueMax  + 32767) / 65535) << outPF.blueShift));
#if (BPPOUT != 8)
    if (outPF.bigEndian != nativeBigEndian)
      table[i] = SWAPOUT (table[i]);
#endif
  }
}

void initSimpleTCtoTCOUT (rdr::U8** tablep, const PixelFormat& inPF,
                          const PixelFormat& outPF)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = 1 << inPF.bpp;

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];
  OUTPIXEL* table = (OUTPIXEL*)*tablep;

  for (int i = 0; i < size; i++) {
    int r = (i >> inPF.redShift)   & inPF.redMax;
    int g = (i >> inPF.greenShift) & inPF.greenMax;
    int b = (i >> inPF.blueShift)  & inPF.blueMax;
      
    r = (r * outPF.redMax   + inPF.redMax/2)   / inPF.redMax;
    g = (g * outPF.greenMax + inPF.greenMax/2) / inPF.greenMax;
    b = (b * outPF.blueMax  + inPF.blueMax/2)  / inPF.blueMax;
      
    table[i] = ((r << outPF.redShift)   |
                (g << outPF.greenShift) |
                (b << outPF.blueShift));
#if (BPPOUT != 8)
    if (outPF.bigEndian != nativeBigEndian)
      table[i] = SWAPOUT (table[i]);
#endif
  }
}

void initSimpleCMtoCubeOUT (rdr::U8** tablep, const PixelFormat& inPF,
                            ColourMap* cm, ColourCube* cube)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = 1 << inPF.bpp;

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];
  OUTPIXEL* table = (OUTPIXEL*)*tablep;

  for (int i = 0; i < size; i++) {
    int r,g,b;
    cm->lookup(i,&r,&g,&b);
    r = (r * (cube->nRed-1)   + 32767) / 65535;
    g = (g * (cube->nGreen-1) + 32767) / 65535;
    b = (b * (cube->nBlue-1)  + 32767) / 65535;
    table[i] = cube->lookup(r, g, b);
  }
}

void initSimpleTCtoCubeOUT (rdr::U8** tablep, const PixelFormat& inPF,
                            ColourCube* cube)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = 1 << inPF.bpp;

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];
  OUTPIXEL* table = (OUTPIXEL*)*tablep;

  for (int i = 0; i < size; i++) {
    int r = (i >> inPF.redShift)   & inPF.redMax;
    int g = (i >> inPF.greenShift) & inPF.greenMax;
    int b = (i >> inPF.blueShift)  & inPF.blueMax;

    r = (r * (cube->nRed-1)   + inPF.redMax/2)   / inPF.redMax;
    g = (g * (cube->nGreen-1) + inPF.greenMax/2) / inPF.greenMax;
    b = (b * (cube->nBlue-1)  + inPF.blueMax/2)  / inPF.blueMax;

    table[i] = cube->lookup(r, g, b);
  }
}

void initOneRGBTableOUT (OUTPIXEL* table, int inMax, int outMax,
                         int outShift, bool swap)
{
  int size = inMax + 1;

  for (int i = 0; i < size; i++) {
    table[i] = ((i * outMax + inMax / 2) / inMax) << outShift;
#if (BPPOUT != 8)
    if (swap)
      table[i] = SWAPOUT (table[i]);
#endif
  }
}

void initRGBTCtoTCOUT (rdr::U8** tablep, const PixelFormat& inPF,
                       const PixelFormat& outPF)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = inPF.redMax + inPF.greenMax + inPF.blueMax + 3;

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];

  OUTPIXEL* redTable = (OUTPIXEL*)*tablep;
  OUTPIXEL* greenTable = redTable + inPF.redMax + 1;
  OUTPIXEL* blueTable = greenTable + inPF.greenMax + 1;

  bool swap = (outPF.bigEndian != nativeBigEndian);

  initOneRGBTableOUT (redTable, inPF.redMax, outPF.redMax, 
                           outPF.redShift, swap);
  initOneRGBTableOUT (greenTable, inPF.greenMax, outPF.greenMax,
                           outPF.greenShift, swap);
  initOneRGBTableOUT (blueTable, inPF.blueMax, outPF.blueMax,
                           outPF.blueShift, swap);
}


void initOneRGBCubeTableOUT (OUTPIXEL* table, int inMax, int outMax,
                             int outMult)
{
  int size = inMax + 1;

  for (int i = 0; i < size; i++) {
    table[i] = ((i * outMax + inMax / 2) / inMax) * outMult;
  }
}

void initRGBTCtoCubeOUT (rdr::U8** tablep, const PixelFormat& inPF,
                         ColourCube* cube)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = inPF.redMax + inPF.greenMax + inPF.blueMax + 3 + cube->size();

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];

  OUTPIXEL* redTable = (OUTPIXEL*)*tablep;
  OUTPIXEL* greenTable = redTable + inPF.redMax + 1;
  OUTPIXEL* blueTable = greenTable + inPF.greenMax + 1;
  OUTPIXEL* cubeTable = blueTable + inPF.blueMax + 1;

  initOneRGBCubeTableOUT (redTable,   inPF.redMax,   cube->nRed-1,
                               cube->redMult());
  initOneRGBCubeTableOUT (greenTable, inPF.greenMax, cube->nGreen-1,
                               cube->greenMult());
  initOneRGBCubeTableOUT (blueTable,  inPF.blueMax,  cube->nBlue-1,
                               cube->blueMult());
  for (int i = 0; i < cube->size(); i++) {
    cubeTable[i] = cube->table[i];
  }
}

#undef OUTPIXEL
#undef initSimpleCMtoTCOUT
#undef initSimpleTCtoTCOUT
#undef initSimpleCMtoCubeOUT
#undef initSimpleTCtoCubeOUT
#undef initRGBTCtoTCOUT
#undef initRGBTCtoCubeOUT
#undef initOneRGBTableOUT
#undef initOneRGBCubeTableOUT
}
