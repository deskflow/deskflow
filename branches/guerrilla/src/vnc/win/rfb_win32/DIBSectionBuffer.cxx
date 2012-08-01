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

#include <rfb_win32/DIBSectionBuffer.h>
#include <rfb_win32/DeviceContext.h>
#include <rfb_win32/BitmapInfo.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("DIBSectionBuffer");


DIBSectionBuffer::DIBSectionBuffer(HWND window_)
  : bitmap(0), device(0), window(window_) {
  memset(&format, 0, sizeof(format));
  memset(palette, 0, sizeof(palette));
}

DIBSectionBuffer::DIBSectionBuffer(HDC device_)
  : bitmap(0), window(0), device(device_) {
  memset(&format, 0, sizeof(format));
  memset(palette, 0, sizeof(palette));
}

DIBSectionBuffer::~DIBSectionBuffer() {
  if (bitmap)
    DeleteObject(bitmap);
}


void DIBSectionBuffer::setPF(const PixelFormat& pf) {
  if (memcmp(&getPF(), &pf, sizeof(pf)) == 0) {
    vlog.debug("pixel format unchanged by setPF()");
    return;
  }
  format = pf;
  recreateBuffer();
  if ((pf.bpp <= 8) && pf.trueColour) {
    vlog.info("creating %d-bit TrueColour palette", pf.depth);
    for (int i=0; i < (1<<(pf.depth)); i++) {
      palette[i].b = ((((i >> pf.blueShift) & pf.blueMax) * 65535) + pf.blueMax/2) / pf.blueMax;
      palette[i].g = ((((i >> pf.greenShift) & pf.greenMax) * 65535) + pf.greenMax/2) / pf.greenMax;
      palette[i].r = ((((i >> pf.redShift) & pf.redMax) * 65535) + pf.redMax/2) / pf.redMax;
    }
    refreshPalette();
  }
}

void DIBSectionBuffer::setSize(int w, int h) {
  if (width_ == w && height_ == h) {
    vlog.debug("size unchanged by setSize()");
    return;
  }
  width_ = w;
  height_ = h;
  recreateBuffer();
}


// * copyPaletteToDIB MUST NEVER be called on a truecolour DIB! *

void copyPaletteToDIB(Colour palette[256], HDC wndDC, HBITMAP dib) {
  BitmapDC dibDC(wndDC, dib);
  RGBQUAD rgb[256];
  for (unsigned int i=0;i<256;i++) {
    rgb[i].rgbRed = palette[i].r >> 8;
    rgb[i].rgbGreen = palette[i].g >> 8;
    rgb[i].rgbBlue = palette[i].b >> 8;
  }
  if (!SetDIBColorTable(dibDC, 0, 256, (RGBQUAD*) rgb))
    throw rdr::SystemException("unable to SetDIBColorTable", GetLastError());
}

inline void initMaxAndShift(DWORD mask, int* max, int* shift) {
  for ((*shift) = 0; (mask & 1) == 0; (*shift)++) mask >>= 1;
  (*max) = (rdr::U16)mask;
}

void DIBSectionBuffer::recreateBuffer() {
  HBITMAP new_bitmap = 0;
  rdr::U8* new_data = 0;

  if (width_ && height_ && (format.depth != 0)) {
    BitmapInfo bi;
    memset(&bi, 0, sizeof(bi));
    // *** wrong?
    UINT iUsage = format.trueColour ? DIB_RGB_COLORS : DIB_PAL_COLORS;
    // ***
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biBitCount = format.bpp;
    bi.bmiHeader.biSizeImage = (format.bpp / 8) * width_ * height_;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biWidth = width_;
    bi.bmiHeader.biHeight = -height_;
    bi.bmiHeader.biCompression = (format.bpp > 8) ? BI_BITFIELDS : BI_RGB;
    bi.mask.red = format.redMax << format.redShift;
    bi.mask.green = format.greenMax << format.greenShift;
    bi.mask.blue = format.blueMax << format.blueShift;

    // Create a DIBSection to draw into
    if (device)
      new_bitmap = ::CreateDIBSection(device, (BITMAPINFO*)&bi.bmiHeader, iUsage,
                                      (void**)&new_data, NULL, 0);
    else
      new_bitmap = ::CreateDIBSection(WindowDC(window), (BITMAPINFO*)&bi.bmiHeader, iUsage,
                                      (void**)&new_data, NULL, 0);

    if (!new_bitmap) {
      int err = GetLastError();
      throw rdr::SystemException("unable to create DIB section", err);
    }

    vlog.debug("recreateBuffer()");
  } else {
    vlog.debug("one of area or format not set");
  }

  if (new_bitmap && bitmap) {
    vlog.debug("preserving bitmap contents");

    // Copy the contents across
    if (device) {
      if (format.bpp <= 8)
        copyPaletteToDIB(palette, device, new_bitmap);
      BitmapDC src_dev(device, bitmap);
      BitmapDC dest_dev(device, new_bitmap);
      BitBlt(dest_dev, 0, 0, width_, height_, src_dev, 0, 0, SRCCOPY);
    } else {
      WindowDC wndDC(window);
      if (format.bpp <= 8)
        copyPaletteToDIB(palette, wndDC, new_bitmap);
      BitmapDC src_dev(wndDC, bitmap);
      BitmapDC dest_dev(wndDC, new_bitmap);
      BitBlt(dest_dev, 0, 0, width_, height_, src_dev, 0, 0, SRCCOPY);
    }
  }
  
  if (bitmap) {
    // Delete the old bitmap
    DeleteObject(bitmap);
    bitmap = 0;
    data = 0;
  }

  if (new_bitmap) {
    // Set up the new bitmap
    bitmap = new_bitmap;
    data = new_data;

    // Determine the *actual* DIBSection format
    DIBSECTION ds;
    if (!GetObject(bitmap, sizeof(ds), &ds))
      throw rdr::SystemException("GetObject", GetLastError());

    // Correct the "stride" of the DIB
    // *** This code DWORD aligns each row - is that right???
    stride = width_;
    int bytesPerRow = stride * format.bpp/8;
    if (bytesPerRow % 4) {
      bytesPerRow += 4 - (bytesPerRow % 4);
      stride = (bytesPerRow * 8) / format.bpp;
      vlog.info("adjusting DIB stride: %d to %d", width_, stride);
    }

    // Calculate the PixelFormat for the DIB
    format.bigEndian = 0;
    format.bpp = format.depth = ds.dsBm.bmBitsPixel;
    format.trueColour = format.trueColour || format.bpp > 8;
    if (format.bpp > 8) {

      // Get the truecolour format used by the DIBSection
      initMaxAndShift(ds.dsBitfields[0], &format.redMax, &format.redShift);
      initMaxAndShift(ds.dsBitfields[1], &format.greenMax, &format.greenShift);
      initMaxAndShift(ds.dsBitfields[2], &format.blueMax, &format.blueShift);

      // Calculate the effective depth
      format.depth = 0;
      Pixel bits = ds.dsBitfields[0] | ds.dsBitfields[1] | ds.dsBitfields[2];
      while (bits) {
        format.depth++;
        bits = bits >> 1;
      }
      if (format.depth > format.bpp)
        throw Exception("Bad DIBSection format (depth exceeds bpp)");
    } else {
      // Set the DIBSection's palette
      refreshPalette();
    }

  }
}

void DIBSectionBuffer::refreshPalette() {
  if (format.bpp > 8) {
    vlog.error("refresh palette called for truecolour DIB");
    return;
  }
  vlog.debug("refreshing palette");
  if (device)
    copyPaletteToDIB(palette, device, bitmap);
  else
    copyPaletteToDIB(palette, WindowDC(window), bitmap);
}


