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
// transTempl.h - templates for translation functions.
//
// This file is #included after having set the following macros:
// BPPIN  - 8, 16 or 32
// BPPOUT - 8, 16 or 32

#if !defined(BPPIN) || !defined(BPPOUT)
#error "transTempl.h: BPPIN or BPPOUT not defined"
#endif

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#ifndef CONCAT4E
#define CONCAT4(a,b,c,d) a##b##c##d
#define CONCAT4E(a,b,c,d) CONCAT4(a,b,c,d)
#endif

#define INPIXEL rdr::CONCAT2E(U,BPPIN)
#define OUTPIXEL rdr::CONCAT2E(U,BPPOUT)
#define transSimpleINtoOUT CONCAT4E(transSimple,BPPIN,to,BPPOUT)
#define transRGBINtoOUT CONCAT4E(transRGB,BPPIN,to,BPPOUT)
#define transRGBCubeINtoOUT CONCAT4E(transRGBCube,BPPIN,to,BPPOUT)

#if (BPPIN <= 16)

// transSimpleINtoOUT uses a single table.  This can be used for any incoming
// and outgoing pixel formats, as long as the incoming pixel format is not too
// large (for 16bpp, the table needs 64K entries).

void transSimpleINtoOUT (void* table_,
                         const PixelFormat& inPF, void* inPtr, int inStride,
                         const PixelFormat& outPF, void* outPtr, int outStride,
                         int width, int height)
{
  OUTPIXEL* table = (OUTPIXEL*)table_;
  INPIXEL* ip = (INPIXEL*)inPtr;
  OUTPIXEL* op = (OUTPIXEL*)outPtr;
  int inExtra = inStride - width;
  int outExtra = outStride - width;

  while (height > 0) {
    OUTPIXEL* opEndOfRow = op + width;
    while (op < opEndOfRow)
      *op++ = table[*ip++];
    ip += inExtra;
    op += outExtra;
    height--;
  }
}

#endif

#if (BPPIN >= 16)

// transRGBINtoOUT uses three tables, one each for red, green and blue
// components and adds the values to produce the result.  This can be used
// where a single table would be too large (e.g. 32bpp).  It only works for a
// trueColour incoming pixel format.  Usually the outgoing pixel format is
// trueColour, but we add rather than ORing the three values so that it is also
// possible to generate an index into a colour cube.  I believe that in most
// cases adding is just as fast as ORing - if not then we should split this
// into two different functions for efficiency.

void transRGBINtoOUT (void* table,
                      const PixelFormat& inPF, void* inPtr, int inStride,
                      const PixelFormat& outPF, void* outPtr, int outStride,
                      int width, int height)
{
  OUTPIXEL* redTable = (OUTPIXEL*)table;
  OUTPIXEL* greenTable = redTable + inPF.redMax + 1;
  OUTPIXEL* blueTable = greenTable + inPF.greenMax + 1;
  INPIXEL* ip = (INPIXEL*)inPtr;
  OUTPIXEL* op = (OUTPIXEL*)outPtr;
  int inExtra = inStride - width;
  int outExtra = outStride - width;

  while (height > 0) {
    OUTPIXEL* opEndOfRow = op + width;
    while (op < opEndOfRow) {
      *op++ = (redTable  [(*ip >> inPF.redShift)   & inPF.redMax] +
               greenTable[(*ip >> inPF.greenShift) & inPF.greenMax] +
               blueTable [(*ip >> inPF.blueShift)  & inPF.blueMax]);
      ip++;
    }
    ip += inExtra;
    op += outExtra;
    height--;
  }
}

// transRGBCubeINtoOUT is similar to transRGBINtoOUT but also looks up the
// colour cube index in a fourth table to yield a pixel value.

void transRGBCubeINtoOUT (void* table,
                          const PixelFormat& inPF, void* inPtr, int inStride,
                          const PixelFormat& outPF, void* outPtr,
                          int outStride, int width, int height)
{
  OUTPIXEL* redTable = (OUTPIXEL*)table;
  OUTPIXEL* greenTable = redTable + inPF.redMax + 1;
  OUTPIXEL* blueTable = greenTable + inPF.greenMax + 1;
  OUTPIXEL* cubeTable = blueTable + inPF.blueMax + 1;
  INPIXEL* ip = (INPIXEL*)inPtr;
  OUTPIXEL* op = (OUTPIXEL*)outPtr;
  int inExtra = inStride - width;
  int outExtra = outStride - width;

  while (height > 0) {
    OUTPIXEL* opEndOfRow = op + width;
    while (op < opEndOfRow) {
      *op++ = cubeTable[(redTable  [(*ip >> inPF.redShift)   & inPF.redMax] +
                         greenTable[(*ip >> inPF.greenShift) & inPF.greenMax] +
                         blueTable [(*ip >> inPF.blueShift)  & inPF.blueMax])];
      ip++;
    }
    ip += inExtra;
    op += outExtra;
    height--;
  }
}

#endif

#undef INPIXEL
#undef OUTPIXEL
#undef transSimpleINtoOUT
#undef transRGBINtoOUT
#undef transRGBCubeINtoOUT
