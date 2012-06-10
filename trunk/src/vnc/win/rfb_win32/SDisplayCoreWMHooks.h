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

// -=- SDisplayCoreWMHooks.h
//
// SDisplayCore implementation that uses WMHooks to capture changes to
// the display.
// Whenever changes are detected, the SDisplay's updateEvent is signalled,
// so that it can perform housekeeping tasks (like ensuring the currently
// active desktop is the correct one), before flushing changes from the
// Core to the VNC Server.  The SDisplay will clip the changes before they
// reach the VNC Server.


#ifndef __RFB_SDISPLAY_CORE_WMHOOKS_H__
#define __RFB_SDISPLAY_CORE_WMHOOKS_H__

#include <rfb_win32/SDisplayCorePolling.h>
#include <rfb_win32/WMHooks.h>
#include <rfb_win32/WMPoller.h>

namespace rfb {
  namespace win32 {

    class SDisplayCoreWMHooks : public SDisplayCorePolling {
    public:
      SDisplayCoreWMHooks(SDisplay* display, UpdateTracker* ut);
      ~SDisplayCoreWMHooks();

      // - Called by SDisplay to flush updates to the specified tracker
      virtual void flushUpdates();

      virtual const char* methodName() const { return "VNC Hooks"; }

    protected:
      // - MsgWindow overrides
      //   processMessage is used to service the cursor & polling timers
      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

      // - Hooking subcomponents used to track the desktop state
      WMHooks hooks;
      WMPoller poller;
      IntervalTimer cursorTimer;
      IntervalTimer consolePollTimer;
      bool pollConsoles;
      static const int consolePollTimerId;
      static const int cursorTimerId;
    };

  };
};

#endif
