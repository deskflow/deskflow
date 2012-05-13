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
#include <stdlib.h>
#include <rfb/PixelFormat.h>
#include <rfb/Exception.h>
#include <rfb/ConnParams.h>
#include <rfb/SMsgWriter.h>
#include <rfb/ColourMap.h>
#include <rfb/TrueColourMap.h>
#include <rfb/PixelBuffer.h>
#include <rfb/ColourCube.h>
#include <rfb/TransImageGetter.h>

using namespace rfb;

const PixelFormat bgr233PF(8, 8, false, true, 7, 7, 3, 0, 3, 6);

static void noTransFn(void* table_,
                      const PixelFormat& inPF, void* inPtr, int inStride,
                      const PixelFormat& outPF, void* outPtr, int outStride,
                      int width, int height)
{
  rdr::U8* ip = (rdr::U8*)inPtr;
  rdr::U8* op = (rdr::U8*)outPtr;
  int inStrideBytes = inStride * (inPF.bpp/8);
  int outStrideBytes = outStride * (outPF.bpp/8);
  int widthBytes = width * (outPF.bpp/8);

  while (height > 0) {
    memcpy(op, ip, widthBytes);
    ip += inStrideBytes;
    op += outStrideBytes;
    height--;
  }
}

#define BPPOUT 8
#include "transInitTempl.h"
#define BPPIN 8
#include "transTempl.h"
#undef BPPIN
#define BPPIN 16
#include "transTempl.h"
#undef BPPIN
#define BPPIN 32
#include "transTempl.h"
#undef BPPIN
#undef BPPOUT

#define BPPOUT 16
#include "transInitTempl.h"
#define BPPIN 8
#include "transTempl.h"
#undef BPPIN
#define BPPIN 16
#include "transTempl.h"
#undef BPPIN
#define BPPIN 32
#include "transTempl.h"
#undef BPPIN
#undef BPPOUT

#define BPPOUT 32
#include "transInitTempl.h"
#define BPPIN 8
#include "transTempl.h"
#undef BPPIN
#define BPPIN 16
#include "transTempl.h"
#undef BPPIN
#define BPPIN 32
#include "transTempl.h"
#undef BPPIN
#undef BPPOUT


// Translation functions.  Note that transSimple* is only used for 8/16bpp and
// transRGB* is used for 16/32bpp

static transFnType transSimpleFns[][3] = {
  { transSimple8to8,  transSimple8to16,  transSimple8to32 },
  { transSimple16to8, transSimple16to16, transSimple16to32 },
};
static transFnType transRGBFns[][3] = {
  { transRGB16to8, transRGB16to16, transRGB16to32 },
  { transRGB32to8, transRGB32to16, transRGB32to32 }
};
static transFnType transRGBCubeFns[][3] = {
  { transRGBCube16to8, transRGBCube16to16, transRGBCube16to32 },
  { transRGBCube32to8, transRGBCube32to16, transRGBCube32to32 }
};

// Table initialisation functions.

typedef void (*initCMtoTCFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                 ColourMap* cm, const PixelFormat& outPF);
typedef void (*initTCtoTCFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                 const PixelFormat& outPF);
typedef void (*initCMtoCubeFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                   ColourMap* cm, ColourCube* cube);
typedef void (*initTCtoCubeFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                   ColourCube* cube);


static initCMtoTCFnType initSimpleCMtoTCFns[] = {
    initSimpleCMtoTC8, initSimpleCMtoTC16, initSimpleCMtoTC32
};

static initTCtoTCFnType initSimpleTCtoTCFns[] = {
    initSimpleTCtoTC8, initSimpleTCtoTC16, initSimpleTCtoTC32
};

static initCMtoCubeFnType initSimpleCMtoCubeFns[] = {
    initSimpleCMtoCube8, initSimpleCMtoCube16, initSimpleCMtoCube32
};

static initTCtoCubeFnType initSimpleTCtoCubeFns[] = {
    initSimpleTCtoCube8, initSimpleTCtoCube16, initSimpleTCtoCube32
};

static initTCtoTCFnType initRGBTCtoTCFns[] = {
    initRGBTCtoTC8, initRGBTCtoTC16, initRGBTCtoTC32
};

static initTCtoCubeFnType initRGBTCtoCubeFns[] = {
    initRGBTCtoCube8, initRGBTCtoCube16, initRGBTCtoCube32
};


TransImageGetter::TransImageGetter(bool econ)
  : economic(econ), pb(0), table(0), transFn(0), cube(0)
{
}

TransImageGetter::~TransImageGetter()
{
  delete [] table;
}

void TransImageGetter::init(PixelBuffer* pb_, const PixelFormat& out,
                            SMsgWriter* writer, ColourCube* cube_)
{
  pb = pb_;
  outPF = out;
  transFn = 0;
  cube = cube_;
  const PixelFormat& inPF = pb->getPF();

  if ((inPF.bpp != 8) && (inPF.bpp != 16) && (inPF.bpp != 32))
    throw Exception("TransImageGetter: bpp in not 8, 16 or 32");

  if ((outPF.bpp != 8) && (outPF.bpp != 16) && (outPF.bpp != 32))
    throw Exception("TransImageGetter: bpp out not 8, 16 or 32");

  if (!outPF.trueColour) {
    if (outPF.bpp != 8)
      throw Exception("TransImageGetter: outPF has colour map but not 8bpp");

    if (!inPF.trueColour) {
      if (inPF.bpp != 8)
        throw Exception("TransImageGetter: inPF has colourMap but not 8bpp");

      // CM to CM/Cube

      if (cube) {
        transFn = transSimpleFns[inPF.bpp/16][outPF.bpp/16];
        (*initSimpleCMtoCubeFns[outPF.bpp/16]) (&table, inPF,
                                                pb->getColourMap(), cube);
      } else {
        transFn = noTransFn;
        setColourMapEntries(0, 256, writer);
      }
      return;
    }

    // TC to CM/Cube

    ColourCube defaultCube(6,6,6);
    if (!cube) cube = &defaultCube;

    if ((inPF.bpp > 16) || (economic && (inPF.bpp == 16))) {
      transFn = transRGBCubeFns[inPF.bpp/32][outPF.bpp/16];
      (*initRGBTCtoCubeFns[outPF.bpp/16]) (&table, inPF, cube);
    } else {
      transFn = transSimpleFns[inPF.bpp/16][outPF.bpp/16];
      (*initSimpleTCtoCubeFns[outPF.bpp/16]) (&table, inPF, cube);
    }

    if (cube != &defaultCube)
      return;

    if (writer) writer->writeSetColourMapEntries(0, 216, cube);
    cube = 0;
    return;
  }

  if (inPF.equal(outPF)) {
    transFn = noTransFn;
    return;
  }

  if (!inPF.trueColour) {

    // CM to TC

    if (inPF.bpp != 8)
      throw Exception("TransImageGetter: inPF has colourMap but not 8bpp");
    transFn = transSimpleFns[inPF.bpp/16][outPF.bpp/16];
    (*initSimpleCMtoTCFns[outPF.bpp/16]) (&table, inPF, pb->getColourMap(),
                                          outPF);
    return;
  }

  // TC to TC

  if ((inPF.bpp > 16) || (economic && (inPF.bpp == 16))) {
    transFn = transRGBFns[inPF.bpp/32][outPF.bpp/16];
    (*initRGBTCtoTCFns[outPF.bpp/16]) (&table, inPF, outPF);
  } else {
    transFn = transSimpleFns[inPF.bpp/16][outPF.bpp/16];
    (*initSimpleTCtoTCFns[outPF.bpp/16]) (&table, inPF, outPF);
  }
}

void TransImageGetter::setColourMapEntries(int firstCol, int nCols,
                                           SMsgWriter* writer)
{
  if (nCols == 0)
    nCols = (1 << pb->getPF().depth) - firstCol;
  if (pb->getPF().trueColour) return; // shouldn't be called in this case

  if (outPF.trueColour) {
    (*initSimpleCMtoTCFns[outPF.bpp/16]) (&table, pb->getPF(),
                                          pb->getColourMap(), outPF);
  } else if (cube) {
    (*initSimpleCMtoCubeFns[outPF.bpp/16]) (&table, pb->getPF(),
                                            pb->getColourMap(), cube);
  } else if (writer && pb->getColourMap()) {
    writer->writeSetColourMapEntries(firstCol, nCols, pb->getColourMap());
  }
}

void TransImageGetter::getImage(void* outPtr, const Rect& r, int outStride)
{
  if (!transFn)
    throw Exception("TransImageGetter: not initialised yet");

  int inStride;
  const rdr::U8* inPtr = pb->getPixelsR(r.translate(offset.negate()), &inStride);

  if (!outStride) outStride = r.width();

  (*transFn)(table, pb->getPF(), (void*)inPtr, inStride,
             outPF, outPtr, outStride, r.width(), r.height());
}

void TransImageGetter::translatePixels(void* inPtr, void* outPtr,
                                       int nPixels) const
{
  (*transFn)(table, pb->getPF(), inPtr, nPixels,
             outPF, outPtr, nPixels, nPixels, 1);
}
