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

// -=- WMShatter.h
//
// WMShatter provides the IsSafeWM routine, which returns true iff the
// supplied window message is safe to pass to DispatchMessage, or to
// process in the window procedure.
//
// This is only required, of course, to avoid so-called "shatter" attacks
// to be made against the VNC server, which take advantage of the noddy
// design of the Win32 window messaging system.
//
// The API here is designed to hopefully be future proof, so that if they
// ever come up with a proper way to determine whether a message is safe
// or not then it can just be reimplemented here...

#ifndef __RFB_WIN32_SHATTER_H__
#define __RFB_WIN32_SHATTER_H__

#include <windows.h>

namespace rfb {
  namespace win32 {

    bool IsSafeWM(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT SafeDefWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT SafeDispatchMessage(const MSG* msg);

  };
};

#endif // __RFB_WIN32_SHATTER_H__
