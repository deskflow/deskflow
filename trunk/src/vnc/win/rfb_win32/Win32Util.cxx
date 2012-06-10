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

// Win32Util.cxx

#include <rfb_win32/ModuleFileName.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/MonitorInfo.h>
#include <rfb_win32/Handle.h>
#include <rdr/HexOutStream.h>
#include <rdr/Exception.h>

namespace rfb {
namespace win32 {


FileVersionInfo::FileVersionInfo(const TCHAR* filename) {
  // Get executable name
  ModuleFileName exeName;
  if (!filename)
    filename = exeName.buf;

  // Attempt to open the file, to cause Access Denied, etc, errors
  // to be correctly reported, since the GetFileVersionInfoXXX calls lie...
  {
    Handle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	  if (file.h == INVALID_HANDLE_VALUE)
      throw rdr::SystemException("Failed to open file", GetLastError());
  }

  // Get version info size
  DWORD handle;
  int size = GetFileVersionInfoSize((TCHAR*)filename, &handle);
  if (!size)
    throw rdr::SystemException("GetVersionInfoSize failed", GetLastError());

  // Get version info
  buf = new TCHAR[size];
  if (!GetFileVersionInfo((TCHAR*)filename, handle, size, buf))
    throw rdr::SystemException("GetVersionInfo failed", GetLastError());
}

const TCHAR* FileVersionInfo::getVerString(const TCHAR* name, DWORD langId) {
  char langIdBuf[sizeof(langId)];
  for (int i=sizeof(langIdBuf)-1; i>=0; i--) {
    langIdBuf[i] = (char) (langId & 0xff);
    langId = langId >> 8;
  }

  TCharArray langIdStr = rdr::HexOutStream::binToHexStr(langIdBuf, sizeof(langId));
  TCharArray infoName(_tcslen(_T("StringFileInfo")) + 4 + _tcslen(name) + _tcslen(langIdStr.buf));
  _stprintf(infoName.buf, _T("\\StringFileInfo\\%s\\%s"), langIdStr.buf, name);

  // Locate the required version string within the version info
  TCHAR* buffer = 0;
  UINT length = 0;
  if (!VerQueryValue(buf, infoName.buf, (void**)&buffer, &length)) {
    printf("unable to find %s version string", CStr(infoName.buf));
    throw rdr::Exception("VerQueryValue failed");
  }
  return buffer;
}


bool splitPath(const TCHAR* path, TCHAR** dir, TCHAR** file) {
  return tstrSplit(path, '\\', dir, file, true);
}


void centerWindow(HWND handle, HWND parent) {
  RECT r;
  MonitorInfo mi(parent ? parent : handle);
  if (!parent || !IsWindowVisible(parent) || !GetWindowRect(parent, &r))
    r=mi.rcWork;
  centerWindow(handle, r);
  mi.clipTo(handle);
}

void centerWindow(HWND handle, const RECT& r) {
  RECT wr;
  if (!GetWindowRect(handle, &wr)) return;
  int w = wr.right-wr.left;
  int h = wr.bottom-wr.top;
  int x = (r.left + r.right - w)/2;
  int y = (r.top + r.bottom - h)/2;
  UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE;
  SetWindowPos(handle, 0, x, y, 0, 0, flags);
}

void resizeWindow(HWND handle, int width, int height) {
  RECT r;
  GetWindowRect(handle, &r);
  SetWindowPos(handle, 0, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
  centerWindow(handle, r);
}


};
};
