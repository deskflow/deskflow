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

#ifndef __RFB_WIN32_BITMAP_INFO_H__
#define __RFB_WIN32_BITMAP_INFO_H__

#include <windows.h>
#include <rdr/types.h>

namespace rfb {
  namespace win32 {

    struct BitmapInfo {
      BITMAPINFOHEADER bmiHeader;
      union {
        struct {
          DWORD red;
          DWORD green;
          DWORD blue;
        } mask;
        RGBQUAD color[256];
      };
    };

    inline void initMaxAndShift(DWORD mask, int* max, int* shift) {
      for ((*shift) = 0; (mask & 1) == 0; (*shift)++) mask >>= 1;
        (*max) = (rdr::U16)mask;
    }

  };
};

#endif
