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

// -=- DIBSectionBuffer.h

// A DIBSectionBuffer acts much like a standard PixelBuffer, but is associated
// with a particular window on-screen and can be drawn into that window if
// required, using the standard Win32 drawing operations.

#ifndef __RFB_WIN32_DIB_SECTION_BUFFER_H__
#define __RFB_WIN32_DIB_SECTION_BUFFER_H__

#include <windows.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Region.h>
#include <rfb/ColourMap.h>
#include <rfb/Exception.h>

namespace rfb {

  namespace win32 {

    //
    // -=- DIBSectionBuffer
    //

    class DIBSectionBuffer : public FullFramePixelBuffer, ColourMap {
    public:
      DIBSectionBuffer(HWND window);
      DIBSectionBuffer(HDC device);
      virtual ~DIBSectionBuffer();

      virtual void setPF(const PixelFormat &pf);
      virtual void setSize(int w, int h);

      virtual int getStride() const {return stride;}

      virtual ColourMap* getColourMap() const {return (ColourMap*)this;}

      // - ColourMap interface
      virtual void lookup(int index, int* r, int *g, int* b) {
        *r = palette[index].r;
        *g = palette[index].g;
        *b = palette[index].b;
      }
  
      // Custom colourmap interface
      void setColour(int index, int r, int g, int b) {
        palette[index].r = r;
        palette[index].g = g;
        palette[index].b = b;
      }
      void refreshPalette();

      // *** virtual void copyRect(const Rect &dest, const Point &move_by_delta);
    public:
      HBITMAP bitmap;
    protected:
      void recreateBuffer();
      Colour palette[256];
      int stride;
      HWND window;
      HDC device;
    };

  };

};

#endif // __RFB_WIN32_DIB_SECTION_BUFFER_H__
