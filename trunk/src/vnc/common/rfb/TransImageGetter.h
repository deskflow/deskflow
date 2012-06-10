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
// TransImageGetter - class to perform translation between pixel formats,
// implementing the ImageGetter interface.
//

#ifndef __RFB_TRANSIMAGEGETTER_H__
#define __RFB_TRANSIMAGEGETTER_H__

#include <rfb/Rect.h>
#include <rfb/PixelFormat.h>
#include <rfb/ImageGetter.h>

namespace rfb {
  typedef void (*transFnType)(void* table_,
                              const PixelFormat& inPF, void* inPtr,
                              int inStride,
                              const PixelFormat& outPF, void* outPtr,
                              int outStride, int width, int height);

  class SMsgWriter;
  class ColourMap;
  class PixelBuffer;
  class ColourCube;

  class TransImageGetter : public ImageGetter {
  public:

    TransImageGetter(bool econ=false);
    virtual ~TransImageGetter();

    // init() is called to initialise the translation tables.  The PixelBuffer
    // argument gives the source data and format details, outPF gives the
    // client's pixel format.  If the client has a colour map, then the writer
    // argument is used to send a SetColourMapEntries message to the client.

    void init(PixelBuffer* pb, const PixelFormat& outPF, SMsgWriter* writer=0,
              ColourCube* cube=0);

    // setColourMapEntries() is called when the PixelBuffer has a colour map
    // which has changed.  firstColour and nColours specify which part of the
    // colour map has changed.  If nColours is 0, this means the rest of the
    // colour map.  The PixelBuffer previously passed to init() must have a
    // valid ColourMap object.  If the client also has a colour map, then the
    // writer argument is used to send a SetColourMapEntries message to the
    // client.  If the client is true colour then instead we update the
    // internal translation table - in this case the caller should also make
    // sure that the client receives an update of the relevant parts of the
    // framebuffer (the simplest thing to do is just update the whole
    // framebuffer, though it is possible to be smarter than this).

    void setColourMapEntries(int firstColour, int nColours,
                             SMsgWriter* writer=0);

    // getImage() gets the given rectangle of data from the PixelBuffer,
    // translates it into the client's pixel format and puts it in the buffer
    // pointed to by the outPtr argument.  The optional outStride argument can
    // be used where padding is required between the output scanlines (the
    // padding will be outStride-r.width() pixels).
    void getImage(void* outPtr, const Rect& r, int outStride=0);

    // translatePixels() translates the given number of pixels from inPtr,
    // putting it into the buffer pointed to by outPtr.  The pixels at inPtr
    // should be in the same format as the PixelBuffer, and the translated
    // pixels will be in the format previously given by the outPF argument to
    // init().  Note that this call does not use the PixelBuffer's pixel data.
    void translatePixels(void* inPtr, void* outPtr, int nPixels) const;

    // setPixelBuffer() changes the pixel buffer to be used.  The new pixel
    // buffer MUST have the same pixel format as the old one - if not you
    // should call init() instead.
    void setPixelBuffer(PixelBuffer* pb_) { pb = pb_; }

    // setOffset() sets an offset which is subtracted from the coordinates of
    // the rectangle given to getImage().
    void setOffset(const Point& offset_) { offset = offset_; }

  private:
    bool economic;
    PixelBuffer* pb;
    PixelFormat outPF;
    rdr::U8* table;
    transFnType transFn;
    ColourCube* cube;
    Point offset;
  };
}
#endif
