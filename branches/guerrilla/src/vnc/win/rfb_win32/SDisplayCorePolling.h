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

// -=- SDisplayCorePolling.h
//
// SDisplayCore implementation that simply polls the screen, in sections,
// in order to detect changes.  This Core will signal the SDisplay's
// updateEvent regularly, causing it to call the Core back to propagate
// changes to the VNC Server.


#ifndef __RFB_SDISPLAY_CORE_POLLING_H__
#define __RFB_SDISPLAY_CORE_POLLING_H__

#include <rfb_win32/SDisplay.h>
#include <rfb_win32/IntervalTimer.h>
#include <rfb_win32/WMWindowCopyRect.h>

namespace rfb {
  namespace win32 {

    class SDisplayCorePolling : public SDisplayCore, protected MsgWindow {
    public:
      SDisplayCorePolling(SDisplay* display, UpdateTracker* ut, int pollIntervalMs=50);
      ~SDisplayCorePolling();

      // - Called by SDisplay to inform Core of the screen size
      virtual void setScreenRect(const Rect& screenRect_);

      // - Called by SDisplay to flush updates to the specified tracker
      virtual void flushUpdates();

      virtual const char* methodName() const { return "Polling"; }

    protected:
      // - MsgWindow overrides
      //   processMessage is used to service the cursor & polling timers
      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

      // - Hooking subcomponents used to track the desktop state
      WMCopyRect copyrect;

      // - Background full screen polling fields
      IntervalTimer pollTimer;
      static const int pollTimerId;
      Rect screenRect;
      int pollInterval;
      int pollNextY;
      int pollIncrementY;
      bool pollNextStrip;

      // - Handle back to the owning SDisplay, and to the UpdateTracker to flush to
      SDisplay* display;
      UpdateTracker* updateTracker;
    };

  };
};

#endif