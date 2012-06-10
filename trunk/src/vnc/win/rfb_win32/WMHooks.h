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

// -=- WMHooks.h

#ifndef __RFB_WIN32_WM_HOOKS_H__
#define __RFB_WIN32_WM_HOOKS_H__

#include <windows.h>
#include <rfb/UpdateTracker.h>
#include <rdr/Exception.h>
#include <rfb_win32/Win32Util.h>

namespace rfb {

  namespace win32 {

    // -=- WMHooks
    //     Uses the wm_hooks DLL to intercept window messages, to get _hints_ as
    //     to what may have changed on-screen.  Updates are notified via a Win32
    //     event, and retrieved using the getUpdates method, which is thread-safe.
    class WMHooks {
    public:
      WMHooks();
      ~WMHooks();

      // Specify the event object to notify.  Starts the hook subsystem if it is
      // not already active, and returns false if the hooks fail to start.
      bool setEvent(HANDLE updateEvent);

      // Copies any new updates to the UpdateTracker.  Returns true if new updates
      // were added, false otherwise.
      bool getUpdates(UpdateTracker* ut);

      // Determine whether the hooks DLL is installed on the system
      static bool areAvailable();

#ifdef _DEBUG
      // Get notifications of any messages in the given range, to any hooked window
      void setDiagnosticRange(UINT min, UINT max);
#endif

      // * INTERNAL NOTIFICATION FUNCTION * 
      void NotifyHooksRegion(const Region& r);
    protected:
      HANDLE updateEvent;
      bool updatesReady;
      SimpleUpdateTracker updates;
    };

    // -=- Support for filtering out local input events while remote connections are
    //     active.  Implemented using SetWindowsHookEx for portability.
    class WMBlockInput {
    public:
      WMBlockInput();
      ~WMBlockInput();
      bool blockInputs(bool block);
    protected:
      bool active;
    };

    // - Legacy cursor handling support
    class WMCursorHooks {
    public:
      WMCursorHooks();
      ~WMCursorHooks();

      bool start();

      HCURSOR getCursor() const;
    };

  };

};

#endif // __RFB_WIN32_WM_HOOKS_H__
