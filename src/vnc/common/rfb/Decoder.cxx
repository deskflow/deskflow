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
#include <rfb/Exception.h>
#include <rfb/Decoder.h>
#include <rfb/RawDecoder.h>
#include <rfb/RREDecoder.h>
#include <rfb/HextileDecoder.h>
#include <rfb/ZRLEDecoder.h>

using namespace rfb;

Decoder::~Decoder()
{
}

DecoderCreateFnType Decoder::createFns[encodingMax+1] = { 0 };

bool Decoder::supported(unsigned int encoding)
{
  return encoding <= encodingMax && createFns[encoding];
}

Decoder* Decoder::createDecoder(unsigned int encoding, CMsgReader* reader)
{
  if (encoding <= encodingMax && createFns[encoding])
    return (*createFns[encoding])(reader);
  return 0;
}

void Decoder::registerDecoder(unsigned int encoding,
                              DecoderCreateFnType createFn)
{
  if (encoding > encodingMax)
    throw Exception("Decoder::registerDecoder: encoding out of range");

  if (createFns[encoding])
    fprintf(stderr,"Replacing existing decoder for encoding %s (%d)\n",
            encodingName(encoding), encoding);
  createFns[encoding] = createFn;
}

int DecoderInit::count = 0;

DecoderInit::DecoderInit()
{
  if (count++ != 0) return;

  Decoder::registerDecoder(encodingRaw, RawDecoder::create);
  Decoder::registerDecoder(encodingRRE, RREDecoder::create);
  Decoder::registerDecoder(encodingHextile, HextileDecoder::create);
  Decoder::registerDecoder(encodingZRLE, ZRLEDecoder::create);
}
