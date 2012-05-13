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

// -=- Win32Util.h

// Miscellaneous but useful Win32 API utility functions & classes.
// In particular, a set of classes which wrap GDI objects,
// and some to handle palettes.

#ifndef __RFB_WIN32_GDIUTIL_H__
#define __RFB_WIN32_GDIUTIL_H__

#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    struct FileVersionInfo : public TCharArray {
      FileVersionInfo(const TCHAR* filename=0);
      const TCHAR* getVerString(const TCHAR* name, DWORD langId = 0x080904b0);
    };

    bool splitPath(const TCHAR* path, TCHAR** dir, TCHAR** file);

    // Center the window to a rectangle, or to a parent window.
    // Optionally, resize the window to lay within the rect or parent window
    // If the parent window is NULL then the working area if the window's
    // current monitor is used instead.
    void centerWindow(HWND handle, const RECT& r);
    void centerWindow(HWND handle, HWND parent);

    // resizeWindow resizes a window about its center
    void resizeWindow(HWND handle, int width, int height);

  };

};

#endif
