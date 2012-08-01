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

// DeviceContext base class, wrapping Windows HDC, plus some
// helper classes tailored to particular types of DC, such as
// window and device DCs.

#ifndef __RFB_WIN32_DEVICECONTEXT_H__
#define __RFB_WIN32_DEVICECONTEXT_H__

#include <windows.h>
#include <rfb/PixelFormat.h>
#include <rfb/Rect.h>
#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    // Base class, providing methods to get the bounding (clip) box,
    // and the pixel format, and access to the HDC itself.
    class DeviceContext {
    public:
      DeviceContext() : dc(0) {}
      virtual ~DeviceContext() {}
      operator HDC() const {return dc;}
      PixelFormat getPF() const;
      static PixelFormat getPF(HDC dc);
      Rect getClipBox() const;
      static Rect getClipBox(HDC dc);
    protected:
      HDC dc;
    };

    // -=- DeviceContext that opens a specific display device
    class DeviceDC : public DeviceContext {
    public:
      DeviceDC(const TCHAR* deviceName);
      ~DeviceDC();
    };

    // Get a DC for a particular window's client area.
    class WindowDC : public DeviceContext {
    public:
      WindowDC(HWND wnd);
      virtual ~WindowDC();
    protected:
      HWND hwnd;
    };

    // Create a new DC, compatible with an existing one.
    class CompatibleDC : public DeviceContext {
    public:
      CompatibleDC(HDC existing);
      virtual ~CompatibleDC();
    };

    // Create a new DC, compatible with an existing one, and
    // select the specified bitmap into it.
    class BitmapDC : public CompatibleDC {
    public:
      BitmapDC(HDC hdc, HBITMAP hbitmap);
      ~BitmapDC();
    protected:
      HBITMAP oldBitmap;
    };

  };
};

#endif
