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

// -=- PixelBuffer.cxx
//
// The PixelBuffer class encapsulates the PixelFormat and dimensions
// of a block of pixel data.

#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/PixelBuffer.h>

using namespace rfb;
using namespace rdr;

static LogWriter vlog("PixelBuffer");


// -=- Generic pixel buffer class

PixelBuffer::PixelBuffer(const PixelFormat& pf, int w, int h, ColourMap* cm)
  : format(pf), width_(w), height_(h), colourmap(cm) {}
PixelBuffer::PixelBuffer() : width_(0), height_(0), colourmap(0) {}

PixelBuffer::~PixelBuffer() {}


void PixelBuffer::setPF(const PixelFormat &pf) {format = pf;}
const PixelFormat& PixelBuffer::getPF() const {return format;}
ColourMap* PixelBuffer::getColourMap() const {return colourmap;}


void
PixelBuffer::getImage(void* imageBuf, const Rect& r, int outStride) {
  int inStride;
  const U8* data = getPixelsR(r, &inStride);
  // We assume that the specified rectangle is pre-clipped to the buffer
  int bytesPerPixel = format.bpp/8;
  int inBytesPerRow = inStride * bytesPerPixel;
  if (!outStride) outStride = r.width();
  int outBytesPerRow = outStride * bytesPerPixel;
  int bytesPerMemCpy = r.width() * bytesPerPixel;
  U8* imageBufPos = (U8*)imageBuf;
  const U8* end = data + (inBytesPerRow * r.height());
  while (data < end) {
    memcpy(imageBufPos, data, bytesPerMemCpy);
    imageBufPos += outBytesPerRow;
    data += inBytesPerRow;
  }
}

/* ***
Pixel PixelBuffer::getPixel(const Point& p) {
  int stride;
  Rect r = Rect(p.x, p.y, p.x+1, p.y+1);
  switch(format.bpp) {
  case 8: return *((rdr::U8*)getDataAt(r, &stride));
  case 16: return *((rdr::U16*)getDataAt(r, &stride));
  case 32: return *((rdr::U32*)getDataAt(r, &stride));
  default: return 0;
  };
}
*/


FullFramePixelBuffer::FullFramePixelBuffer(const PixelFormat& pf, int w, int h,
                                           rdr::U8* data_, ColourMap* cm)
  : PixelBuffer(pf, w, h, cm), data(data_)
{
}

FullFramePixelBuffer::FullFramePixelBuffer() : data(0) {}

FullFramePixelBuffer::~FullFramePixelBuffer() {}


int FullFramePixelBuffer::getStride() const { return width(); }

rdr::U8* FullFramePixelBuffer::getPixelsRW(const Rect& r, int* stride)
{
  *stride = getStride();
  return &data[(r.tl.x + (r.tl.y * *stride)) * format.bpp/8];
}


void FullFramePixelBuffer::fillRect(const Rect& r, Pixel pix) {
  int stride;
  U8* data = getPixelsRW(r, &stride);
  int bytesPerPixel = getPF().bpp/8;
  int bytesPerRow = bytesPerPixel * stride;
  int bytesPerFill = bytesPerPixel * r.width();

  U8* end = data + (bytesPerRow * r.height());
  while (data < end) {
    switch (bytesPerPixel) {
    case 1:
      memset(data, pix, bytesPerFill);
      break;
    case 2:
      {
        U16* optr = (U16*)data;
        U16* eol = optr + r.width();
        while (optr < eol)
          *optr++ = pix;
      }
      break;
    case 4:
      {
        U32* optr = (U32*)data;
        U32* eol = optr + r.width();
        while (optr < eol)
          *optr++ = pix;
      }
      break;
    }
    data += bytesPerRow;
  }
}

void FullFramePixelBuffer::imageRect(const Rect& r, const void* pixels, int srcStride) {
  int bytesPerPixel = getPF().bpp/8;
  int destStride;
  U8* dest = getPixelsRW(r, &destStride);
  int bytesPerDestRow = bytesPerPixel * destStride;
  if (!srcStride) srcStride = r.width();
  int bytesPerSrcRow = bytesPerPixel * srcStride;
  int bytesPerFill = bytesPerPixel * r.width();
  const U8* src = (const U8*)pixels;
  U8* end = dest + (bytesPerDestRow * r.height());
  while (dest < end) {
    memcpy(dest, src, bytesPerFill);
    dest += bytesPerDestRow;
    src += bytesPerSrcRow;
  }
}

void FullFramePixelBuffer::maskRect(const Rect& r, const void* pixels, const void* mask_) {
  Rect cr = getRect().intersect(r);
  if (cr.is_empty()) return;
  int stride;
  U8* data = getPixelsRW(cr, &stride);
  U8* mask = (U8*) mask_;
  int w = cr.width();
  int h = cr.height();
  int bpp = getPF().bpp;
  int pixelStride = r.width();
  int maskStride = (r.width() + 7) / 8;

  Point offset = Point(cr.tl.x-r.tl.x, cr.tl.y-r.tl.y);
  mask += offset.y * maskStride;
  for (int y = 0; y < h; y++) {
    int cy = offset.y + y;
    for (int x = 0; x < w; x++) {
      int cx = offset.x + x;
      U8* byte = mask + (cx / 8);
      int bit = 7 - cx % 8;
      if ((*byte) & (1 << bit)) {
        switch (bpp) {
        case 8:
          ((U8*)data)[y * stride + x] = ((U8*)pixels)[cy * pixelStride + cx];
          break;
        case 16:
          ((U16*)data)[y * stride + x] = ((U16*)pixels)[cy * pixelStride + cx];
          break;
        case 32:
          ((U32*)data)[y * stride + x] = ((U32*)pixels)[cy * pixelStride + cx];
          break;
        }
      }
    }
    mask += maskStride;
  }
}

void FullFramePixelBuffer::maskRect(const Rect& r, Pixel pixel, const void* mask_) {
  Rect cr = getRect().intersect(r);
  if (cr.is_empty()) return;
  int stride;
  U8* data = getPixelsRW(cr, &stride);
  U8* mask = (U8*) mask_;
  int w = cr.width();
  int h = cr.height();
  int bpp = getPF().bpp;
  int maskStride = (r.width() + 7) / 8;

  Point offset = Point(cr.tl.x-r.tl.x, cr.tl.y-r.tl.y);
  mask += offset.y * maskStride;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int cx = offset.x + x;
      U8* byte = mask + (cx / 8);
      int bit = 7 - cx % 8;
      if ((*byte) & (1 << bit)) {
        switch (bpp) {
        case 8:
          ((U8*)data)[y * stride + x] = pixel;
          break;
        case 16:
          ((U16*)data)[y * stride + x] = pixel;
          break;
        case 32:
          ((U32*)data)[y * stride + x] = pixel;
          break;
        }
      }
    }
    mask += maskStride;
  }
}

void FullFramePixelBuffer::copyRect(const Rect &rect, const Point &move_by_delta) {
  int stride;
  U8* data = getPixelsRW(getRect(), &stride);
  // We assume that the specified rectangle is pre-clipped to the buffer
  unsigned int bytesPerPixel, bytesPerRow, bytesPerMemCpy;
  Rect srect = rect.translate(move_by_delta.negate());
  bytesPerPixel = getPF().bpp/8;
  bytesPerRow = stride * bytesPerPixel;
  bytesPerMemCpy = rect.width() * bytesPerPixel;
  if (move_by_delta.y <= 0) {
    U8* dest = data + rect.tl.x*bytesPerPixel + rect.tl.y*bytesPerRow;
    U8* src = data + srect.tl.x*bytesPerPixel + srect.tl.y*bytesPerRow;
    for (int i=rect.tl.y; i<rect.br.y; i++) {
      memmove(dest, src, bytesPerMemCpy);
      dest += bytesPerRow;
      src += bytesPerRow;
    }
  } else {
    U8* dest = data + rect.tl.x*bytesPerPixel + (rect.br.y-1)*bytesPerRow;
    U8* src = data + srect.tl.x*bytesPerPixel + (srect.br.y-1)*bytesPerRow;
    for (int i=rect.tl.y; i<rect.br.y; i++) {
      memmove(dest, src, bytesPerMemCpy);
      dest -= bytesPerRow;
      src -= bytesPerRow;
    }
  }
}


// -=- Managed pixel buffer class
// Automatically allocates enough space for the specified format & area

ManagedPixelBuffer::ManagedPixelBuffer()
  : datasize(0), own_colourmap(false)
{
  checkDataSize();
};

ManagedPixelBuffer::ManagedPixelBuffer(const PixelFormat& pf, int w, int h)
  : FullFramePixelBuffer(pf, w, h, 0, 0), datasize(0), own_colourmap(false)
{
  checkDataSize();
};

ManagedPixelBuffer::~ManagedPixelBuffer() {
  if (data) delete [] data;
  if (colourmap && own_colourmap) delete colourmap;
};


void
ManagedPixelBuffer::setPF(const PixelFormat &pf) {
  format = pf; checkDataSize();
};
void
ManagedPixelBuffer::setSize(int w, int h) {
  width_ = w; height_ = h; checkDataSize();
};


void
ManagedPixelBuffer::setColourMap(ColourMap* cm, bool own_cm) {
  if (colourmap && own_colourmap) delete colourmap;
  colourmap = cm;
  own_colourmap = own_cm;
}

inline void
ManagedPixelBuffer::checkDataSize() {
  unsigned long new_datasize = width_ * height_ * (format.bpp/8);
  if (datasize < new_datasize) {
    vlog.debug("reallocating managed buffer (%dx%d)", width_, height_);
    if (data) {
      delete [] data;
      datasize = 0; data = 0;
    }
    if (new_datasize) {
      data = new U8[new_datasize];
      if (!data)
        throw Exception("rfb::ManagedPixelBuffer unable to allocate buffer");
      datasize = new_datasize;
    }
  }
};
