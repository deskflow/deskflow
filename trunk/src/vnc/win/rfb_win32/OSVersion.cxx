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

// -=- OSVersion.cxx

#include <rfb_win32/OSVersion.h>
#include <rdr/Exception.h>
#include <tchar.h>

using namespace rfb;
using namespace win32;


OSVersionInfo::OSVersionInfo() {
  // Get OS Version Info
  ZeroMemory(static_cast<OSVERSIONINFO*>(this), sizeof(this));
  dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (!GetVersionEx(this))
    throw rdr::SystemException("unable to get system version info", GetLastError());

  // Set the special extra flags
  isPlatformNT = dwPlatformId == VER_PLATFORM_WIN32_NT;
  isPlatformWindows = dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;

  cannotSwitchDesktop = isPlatformNT && (dwMajorVersion==4) &&
    ((_tcscmp(szCSDVersion, _T("")) == 0) ||
     (_tcscmp(szCSDVersion, _T("Service Pack 1")) == 0) ||
     (_tcscmp(szCSDVersion, _T("Service Pack 2")) == 0));

}

OSVersionInfo rfb::win32::osVersion;
