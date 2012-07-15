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

// -=- OSVersion.h

// Operating system version info.
// GetVersionInfo is called once at process initialisation, and any
// extra flags (such as isWinNT) are calculated and saved at that
// point.  It is assumed that the OS Version seldom changes during a
// program's execution...

#ifndef __RFB_WIN32_OS_VERSION_H__
#define __RFB_WIN32_OS_VERSION_H__

#include <windows.h>

namespace rfb {

  namespace win32 {

    extern struct OSVersionInfo : OSVERSIONINFO {
      OSVersionInfo();

      // Is the OS one of the NT family (NT 3.51, NT4.0, 2K, XP, etc.)?
      bool isPlatformNT;
      // Is one of the Windows family?
      bool isPlatformWindows;

      // Is this OS one of those that blue-screens when grabbing another desktop (NT4 pre SP3)?
      bool cannotSwitchDesktop;

    } osVersion;

  };

};

#endif // __RFB_WIN32_OS_VERSION_H__
