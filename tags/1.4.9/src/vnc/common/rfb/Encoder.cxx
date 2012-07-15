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
#include <rfb/Encoder.h>
#include <rfb/RawEncoder.h>
#include <rfb/RREEncoder.h>
#include <rfb/HextileEncoder.h>
#include <rfb/ZRLEEncoder.h>

using namespace rfb;

Encoder::~Encoder()
{
}

EncoderCreateFnType Encoder::createFns[encodingMax+1] = { 0 };

bool Encoder::supported(unsigned int encoding)
{
  return encoding <= encodingMax && createFns[encoding];
}

Encoder* Encoder::createEncoder(unsigned int encoding, SMsgWriter* writer)
{
  if (encoding <= encodingMax && createFns[encoding])
    return (*createFns[encoding])(writer);
  return 0;
}

void Encoder::registerEncoder(unsigned int encoding,
                              EncoderCreateFnType createFn)
{
  if (encoding > encodingMax)
    throw Exception("Encoder::registerEncoder: encoding out of range");

  if (createFns[encoding])
    fprintf(stderr,"Replacing existing encoder for encoding %s (%d)\n",
            encodingName(encoding), encoding);
  createFns[encoding] = createFn;
}

void Encoder::unregisterEncoder(unsigned int encoding)
{
  if (encoding > encodingMax)
    throw Exception("Encoder::unregisterEncoder: encoding out of range");
  createFns[encoding] = 0;
}

int EncoderInit::count = 0;

EncoderInit::EncoderInit()
{
  if (count++ != 0) return;

  Encoder::registerEncoder(encodingRaw, RawEncoder::create);
  Encoder::registerEncoder(encodingRRE, RREEncoder::create);
  Encoder::registerEncoder(encodingHextile, HextileEncoder::create);
  Encoder::registerEncoder(encodingZRLE, ZRLEEncoder::create);
}
