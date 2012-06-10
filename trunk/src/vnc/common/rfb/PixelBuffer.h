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

// -=- PixelBuffer.h
//
// The PixelBuffer class encapsulates the PixelFormat and dimensions
// of a block of pixel data.

#ifndef __RFB_PIXEL_BUFFER_H__
#define __RFB_PIXEL_BUFFER_H__

#include <rfb/ImageGetter.h>
#include <rfb/PixelFormat.h>
#include <rfb/ColourMap.h>
#include <rfb/Rect.h>
#include <rfb/Pixel.h>

namespace rfb {

  class Region;

  class PixelBuffer : public ImageGetter {
  public:
    PixelBuffer(const PixelFormat& pf, int width, int height, ColourMap* cm);
    virtual ~PixelBuffer();

    ///////////////////////////////////////////////
    // Format / Layout
    //

    // Set/get pixel format & colourmap
    virtual void setPF(const PixelFormat &pf);
    virtual const PixelFormat &getPF() const;
    virtual ColourMap* getColourMap() const;

    // Get width, height and number of pixels
    int width()  const { return width_; }
    int height() const { return height_; }
    int area() const { return width_ * height_; }

    // Get rectangle encompassing this buffer
    //   Top-left of rectangle is either at (0,0), or the specified point.
    Rect getRect() const { return Rect(0, 0, width_, height_); }
    Rect getRect(const Point& pos) const {
      return Rect(pos, pos.translate(Point(width_, height_)));
    }

    ///////////////////////////////////////////////
    // Access to pixel data
    //

    // Get a pointer into the buffer
    //   The pointer is to the top-left pixel of the specified Rect.
    //   The buffer stride (in pixels) is returned.
    virtual const rdr::U8* getPixelsR(const Rect& r, int* stride) = 0;

    // Get pixel data for a given part of the buffer
    //   Data is copied into the supplied buffer, with the specified
    //   stride.
    virtual void getImage(void* imageBuf, const Rect& r, int stride=0);

    // Get the data at (x,y) as a Pixel.
    //   VERY INEFFICIENT!!!
    // *** Pixel getPixel(const Point& p);

    ///////////////////////////////////////////////
    // Framebuffer update methods
    //

    // Ensure that the specified rectangle of buffer is up to date.
    //   Overridden by derived classes implementing framebuffer access
    //   to copy the required display data into place.
    virtual void grabRegion(const Region& region) {}

  protected:
    PixelBuffer();
    PixelFormat format;
    int width_, height_;
    ColourMap* colourmap;
  };

  // FullFramePixelBuffer

  class FullFramePixelBuffer : public PixelBuffer {
  public:
    FullFramePixelBuffer(const PixelFormat& pf, int width, int height,
                         rdr::U8* data_, ColourMap* cm);
    virtual ~FullFramePixelBuffer();

    // - Get the number of pixels per row in the actual pixel buffer data area
    //   This may in some cases NOT be the same as width().
    virtual int getStride() const;

    // Get a pointer to specified pixel data
    virtual rdr::U8* getPixelsRW(const Rect& r, int* stride);
    virtual const rdr::U8* getPixelsR(const Rect& r, int* stride) {
      return getPixelsRW(r, stride);
    }

    ///////////////////////////////////////////////
    // Basic rendering operations
    // These operations DO NOT clip to the pixelbuffer area, or trap overruns.

    // Fill a rectangle
    virtual void fillRect(const Rect &dest, Pixel pix);

    // Copy pixel data to the buffer
    virtual void imageRect(const Rect &dest, const void* pixels, int stride=0);

    // Copy pixel data from one PixelBuffer location to another
    virtual void copyRect(const Rect &dest, const Point &move_by_delta);

    // Copy pixel data to the buffer through a mask
    //   pixels is a pointer to the pixel to be copied to r.tl.
    //   maskPos specifies the pixel offset in the mask to start from.
    //   mask_ is a pointer to the mask bits at (0,0).
    //   pStride and mStride are the strides of the pixel and mask buffers.
    virtual void maskRect(const Rect& r, const void* pixels, const void* mask_);

    //   pixel is the Pixel value to be used where mask_ is set
    virtual void maskRect(const Rect& r, Pixel pixel, const void* mask_);

    // *** Should this be visible?
    rdr::U8* data;

  protected:
    FullFramePixelBuffer();
  };

  // -=- Managed pixel buffer class
  // Automatically allocates enough space for the specified format & area

  class ManagedPixelBuffer : public FullFramePixelBuffer {
  public:
    ManagedPixelBuffer();
    ManagedPixelBuffer(const PixelFormat& pf, int width, int height);
    virtual ~ManagedPixelBuffer();

    // Manage the pixel buffer layout
    virtual void setPF(const PixelFormat &pf);
    virtual void setSize(int w, int h);

    // Assign a colour map to the buffer
    virtual void setColourMap(ColourMap* cm, bool own_cm);

    // Return the total number of bytes of pixel data in the buffer
    int dataLen() const { return width_ * height_ * (format.bpp/8); }

  protected:
    unsigned long datasize;
    bool own_colourmap;
    void checkDataSize();
  };

};

#endif // __RFB_PIXEL_BUFFER_H__
