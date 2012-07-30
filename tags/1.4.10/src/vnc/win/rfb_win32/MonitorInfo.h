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

// Helper class used to obtain information about a particular monitor.
// This class wraps the Windows MONITORINFOEX ASCII structure, providing
// methods that can safely be called on both multi-monitor aware systems
// and older "legacy" systems.


#ifndef __RFB_WIN32_MONITORINFO_H__
#define __RFB_WIN32_MONITORINFO_H__

#include <windows.h>
#ifdef MONITOR_DEFAULTTONULL
#define RFB_HAVE_MONITORINFO
#endif

namespace rfb {
  namespace win32 {

    // Structure containing info on the monitor nearest the window.
    // Copes with multi-monitor OSes and older ones.
#ifdef RFB_HAVE_MONITORINFO
    struct MonitorInfo : MONITORINFOEXA {
#else
    struct MonitorInfo {
      DWORD cbSize;
      RECT rcMonitor;
      RECT rcWork;
      DWORD dwFlags;
      char szDevice[1]; // Always null...
#endif

      // Constructor: Obtains monitor info for the monitor that has the
      //   greatest overlap with the supplied window or rectangle.
      MonitorInfo(HWND hwnd);
      MonitorInfo(const RECT& r);

      // Constructor: Obtains monitor info for the name monitor.  Monitor
      //   names should be those obtained from the MonitorInfo
      //   szDevice field, and usually look like "\\.\DISPLAY<n>"
      MonitorInfo(const char* devName);

      // Move the specified window to reside on the monitor.
      void moveTo(HWND handle);

      // Clip the specified rectangle or window to the monitor's working area.
      //   The rectangle/window is moved so that as much as possible resides
      //   on the working area of the monitor, and is then intersected with it.
      void clipTo(HWND handle);
      void clipTo(RECT* r);
    };

  };
};

#endif
