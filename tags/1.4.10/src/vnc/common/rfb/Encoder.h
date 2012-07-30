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
#ifndef __RFB_ENCODER_H__
#define __RFB_ENCODER_H__

#include <rfb/Rect.h>
#include <rfb/encodings.h>

namespace rfb {
  class SMsgWriter;
  class Encoder;
  class ImageGetter;
  typedef Encoder* (*EncoderCreateFnType)(SMsgWriter*);

  class Encoder {
  public:
    virtual ~Encoder();

    // writeRect() tries to write the given rectangle.  If it is unable to
    // write the whole rectangle it returns false and sets actual to the actual
    // rectangle which was updated.
    virtual bool writeRect(const Rect& r, ImageGetter* ig, Rect* actual)=0;

    static bool supported(unsigned int encoding);
    static Encoder* createEncoder(unsigned int encoding, SMsgWriter* writer);
    static void registerEncoder(unsigned int encoding,
                                EncoderCreateFnType createFn);
    static void unregisterEncoder(unsigned int encoding);
  private:
    static EncoderCreateFnType createFns[encodingMax+1];
  };

  class EncoderInit {
    static int count;
  public:
    EncoderInit();
  };

  static EncoderInit encoderInitObj;
}

#endif
