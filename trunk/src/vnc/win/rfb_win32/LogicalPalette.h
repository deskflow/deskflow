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

#ifndef __RFB_WIN32_LOGPALETTE_H__
#define __RFB_WIN32_LOGPALETTE_H__

#include <windows.h>
#include <rdr/Exception.h>

namespace rfb {
  namespace win32 {

    class LogicalPalette {
    public:
      LogicalPalette() {
        BYTE buf[sizeof(LOGPALETTE)+256*sizeof(PALETTEENTRY)];
        LOGPALETTE* logpal = (LOGPALETTE*)buf;
        logpal->palVersion = 0x300;
        logpal->palNumEntries = 256;
        for (int i=0; i<256;i++) {
          logpal->palPalEntry[i].peRed = 0;
          logpal->palPalEntry[i].peGreen = 0;
          logpal->palPalEntry[i].peBlue = 0;
          logpal->palPalEntry[i].peFlags = 0;
        }
        palette = CreatePalette(logpal);
        if (!palette)
          throw rdr::SystemException("failed to CreatePalette", GetLastError());
      }
      ~LogicalPalette() {
        if (palette && !DeleteObject(palette))
          throw rdr::SystemException("del palette failed", GetLastError());
      }
      void setEntries(int start, int count, const Colour* cols) {
        if (numEntries < count) {
          ResizePalette(palette, start+count);
          numEntries = start+count;
        }
        PALETTEENTRY* logpal = new PALETTEENTRY[count];
        for (int i=0; i<count; i++) {
          logpal[i].peRed = cols[i].r >> 8;
          logpal[i].peGreen = cols[i].g >> 8;
          logpal[i].peBlue = cols[i].b >> 8;
          logpal[i].peFlags = 0;
        }
        UnrealizeObject(palette);
        SetPaletteEntries(palette, start, count, logpal);
        delete [] logpal;
      }
      HPALETTE getHandle() {return palette;}
    protected:
      HPALETTE palette;
      int numEntries;
    };

    class PaletteSelector {
    public:
      PaletteSelector(HDC dc, HPALETTE pal) : device(dc), redrawRequired(false) {
        oldPal = SelectPalette(dc, pal, FALSE);
        redrawRequired = RealizePalette(dc) > 0;
      }
      ~PaletteSelector() {
        if (oldPal) SelectPalette(device, oldPal, TRUE);
      }
      bool isRedrawRequired() {return redrawRequired;}
    protected:
      HPALETTE oldPal;
      HDC device;
      bool redrawRequired;
    };

  };
};

#endif
