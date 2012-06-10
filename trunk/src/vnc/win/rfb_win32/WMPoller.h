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

// -=- WMPoller.h
//
// Polls the foreground window.  If the pollOnlyConsoles flag is set,
// then checks the window class of the foreground window first and
// only polls it if it's a console.
// If the pollAllWindows flag is set then iterates through visible
// windows, and polls the visible bits.  If pollOnlyConsoles is also
// set then only visible parts of console windows will be polled.

#ifndef __RFB_WIN32_WM_POLLER_H__
#define __RFB_WIN32_WM_POLLER_H__

#include <windows.h>
#include <rfb/UpdateTracker.h>
#include <rfb/Configuration.h>

namespace rfb {

  namespace win32 {

    class WMPoller {
    public:
      WMPoller() : ut(0) {}

      bool processEvent();
      bool setUpdateTracker(UpdateTracker* ut);

      static BoolParameter poll_console_windows;
    protected:
      struct PollInfo {
        Region poll_include;
        Region poll_exclude;
      };
      static bool checkPollWindow(HWND w);
      static void pollWindow(HWND w, PollInfo* info);
      static BOOL CALLBACK enumWindowProc(HWND w, LPARAM lp);
      UpdateTracker* ut;
    };

  };

};

#endif // __RFB_WIN32_WM_POLLER_H__
