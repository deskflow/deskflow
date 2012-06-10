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

#include <rfb_win32/DynamicFn.h>
#include <rfb_win32/MonitorInfo.h>
#include <rfb_win32/Win32Util.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>

#pragma warning(disable: 4996)

using namespace rfb;
using namespace win32;

static LogWriter vlog("MonitorInfo");


// If we are building in multi-monitor support (i.e. the headers support it)
//   then do dynamic imports of the required system calls, and provide any
//   other code that wouldn't otherwise compile.
#ifdef RFB_HAVE_MONITORINFO
#include <tchar.h>
typedef HMONITOR (WINAPI *_MonitorFromWindow_proto)(HWND,DWORD);
static rfb::win32::DynamicFn<_MonitorFromWindow_proto> _MonitorFromWindow(_T("user32.dll"), "MonitorFromWindow");
typedef HMONITOR (WINAPI *_MonitorFromRect_proto)(LPCRECT,DWORD);
static rfb::win32::DynamicFn<_MonitorFromRect_proto> _MonitorFromRect(_T("user32.dll"), "MonitorFromRect");
typedef BOOL (WINAPI *_GetMonitorInfo_proto)(HMONITOR,LPMONITORINFO);
static rfb::win32::DynamicFn<_GetMonitorInfo_proto> _GetMonitorInfo(_T("user32.dll"), "GetMonitorInfoA");
typedef BOOL (WINAPI *_EnumDisplayMonitors_proto)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
static rfb::win32::DynamicFn<_EnumDisplayMonitors_proto> _EnumDisplayMonitors(_T("user32.dll"), "EnumDisplayMonitors");
static void fillMonitorInfo(HMONITOR monitor, MonitorInfo* mi) {
  vlog.debug("monitor=%lx", monitor);
  if (!_GetMonitorInfo.isValid())
    throw rdr::Exception("no GetMonitorInfo");
  memset(mi, 0, sizeof(MONITORINFOEXA));
  mi->cbSize = sizeof(MONITORINFOEXA);
  if (!(*_GetMonitorInfo)(monitor, mi))
    throw rdr::SystemException("failed to GetMonitorInfo", GetLastError());
  vlog.debug("monitor is %d,%d-%d,%d", mi->rcMonitor.left, mi->rcMonitor.top, mi->rcMonitor.right, mi->rcMonitor.bottom);
  vlog.debug("work area is %d,%d-%d,%d", mi->rcWork.left, mi->rcWork.top, mi->rcWork.right, mi->rcWork.bottom);
  vlog.debug("device is \"%s\"", mi->szDevice);
}
#else
#pragma message("  NOTE: Not building Multi-Monitor support.")
#endif


MonitorInfo::MonitorInfo(HWND window) {
  cbSize = sizeof(MonitorInfo);
  szDevice[0] = 0;

#ifdef RFB_HAVE_MONITORINFO
  try {
    if (_MonitorFromWindow.isValid()) {
      HMONITOR monitor = (*_MonitorFromWindow)(window, MONITOR_DEFAULTTONEAREST);
      if (!monitor)
        throw rdr::SystemException("failed to get monitor", GetLastError());
      fillMonitorInfo(monitor, this);
      return;
    }
  } catch (rdr::Exception& e) {
    vlog.error(e.str());
  }
#endif

  // Legacy fallbacks - just return the desktop settings
  vlog.debug("using legacy fall-backs");
  HWND desktop = GetDesktopWindow();
  GetWindowRect(desktop, &rcMonitor);
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
  dwFlags = 0;
}

MonitorInfo::MonitorInfo(const RECT& r) {
  cbSize = sizeof(MonitorInfo);
  szDevice[0] = 0;

#ifdef RFB_HAVE_MONITORINFO
  try {
    if (_MonitorFromRect.isValid()) {
      HMONITOR monitor = (*_MonitorFromRect)(&r, MONITOR_DEFAULTTONEAREST);
      if (!monitor)
        throw rdr::SystemException("failed to get monitor", GetLastError());
      fillMonitorInfo(monitor, this);
      return;
    }
  } catch (rdr::Exception& e) {
    vlog.error(e.str());
  }
#endif

  // Legacy fallbacks - just return the desktop settings
  vlog.debug("using legacy fall-backs");
  HWND desktop = GetDesktopWindow();
  GetWindowRect(desktop, &rcMonitor);
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
  dwFlags = 0;
}


#ifdef RFB_HAVE_MONITORINFO

struct monitorByNameData {
  MonitorInfo* info;
  const char* monitorName;
};

static BOOL CALLBACK monitorByNameEnumProc(HMONITOR monitor,
                                    HDC dc,
                                    LPRECT pos,
                                    LPARAM d) {
  monitorByNameData* data = (monitorByNameData*)d;
  memset(data->info, 0, sizeof(MONITORINFOEXA));
  data->info->cbSize = sizeof(MONITORINFOEXA);
  if ((*_GetMonitorInfo)(monitor, data->info)) {
    if (stricmp(data->monitorName, data->info->szDevice) == 0)
      return FALSE;
  }

  return TRUE;
}

#endif

MonitorInfo::MonitorInfo(const char* devName) {
#ifdef RFB_HAVE_MONITORINFO
  if (!_EnumDisplayMonitors.isValid()) {
    vlog.debug("EnumDisplayMonitors not found");
  } else {
    monitorByNameData data;
    data.info = this;
    data.monitorName = devName;

    (*_EnumDisplayMonitors)(0, 0, &monitorByNameEnumProc, (LPARAM)&data);
    if (stricmp(data.monitorName, szDevice) == 0)
      return;
  }
#endif
  // If multi-monitor is not built, or not supported by the OS,
  //   or if the named monitor is not found, revert to the primary monitor.
  vlog.debug("reverting to primary monitor");
  cbSize = sizeof(MonitorInfo);
  szDevice[0] = 0;

  HWND desktop = GetDesktopWindow();
  GetWindowRect(desktop, &rcMonitor);
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
  dwFlags = 0;
}

void MonitorInfo::moveTo(HWND handle) {
  vlog.debug("moveTo monitor=%s", szDevice);

#ifdef RFB_HAVE_MONITORINFO
  MonitorInfo mi(handle);
  if (strcmp(szDevice, mi.szDevice) != 0) {
    centerWindow(handle, rcWork);
    clipTo(handle);
  }
#endif
}

void MonitorInfo::clipTo(RECT* r) {
  vlog.debug("clipTo monitor=%s", szDevice);

  if (r->top < rcWork.top) {
    r->bottom += rcWork.top - r->top; r->top = rcWork.top;
  }
  if (r->left < rcWork.left) {
    r->right += rcWork.left - r->left; r->left = rcWork.left;
  }
  if (r->bottom > rcWork.bottom) {
    r->top += rcWork.bottom - r->bottom; r->bottom = rcWork.bottom;
  }
  if (r->right > rcWork.right) {
    r->left += rcWork.right - r->right; r->right = rcWork.right;
  }
  r->left = max(r->left, rcWork.left);
  r->right = min(r->right, rcWork.right);
  r->top = max(r->top, rcWork.top);
  r->bottom = min(r->bottom, rcWork.bottom);
}

void MonitorInfo::clipTo(HWND handle) {
  RECT r;
  GetWindowRect(handle, &r);
  clipTo(&r);
  SetWindowPos(handle, 0, r.left, r.top, r.right-r.left, r.bottom-r.top,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}


