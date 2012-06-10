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

// -=- SDisplayCoreWMHooks.cxx

#include <rfb_win32/SDisplayCoreWMHooks.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SDisplayCoreWMHooks");

const int SDisplayCoreWMHooks::cursorTimerId = 2;
const int SDisplayCoreWMHooks::consolePollTimerId = 3;


SDisplayCoreWMHooks::SDisplayCoreWMHooks(SDisplay* d, UpdateTracker* ut)
  : SDisplayCorePolling(d, ut, 5000),
  cursorTimer(getHandle(), cursorTimerId),
  consolePollTimer(getHandle(), consolePollTimerId),
  pollConsoles(false) {
  if (!hooks.setEvent(display->getUpdateEvent()))
    throw rdr::Exception("hook subsystem failed to initialise");
  poller.setUpdateTracker(updateTracker);
  cursorTimer.start(20);
  consolePollTimer.start(200);
}

SDisplayCoreWMHooks::~SDisplayCoreWMHooks() {
}

LRESULT SDisplayCoreWMHooks::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_TIMER) {
    if (wParam == cursorTimerId) {
      SetEvent(display->getUpdateEvent());
      return 0;
    } else if (wParam == consolePollTimerId) {
      pollConsoles = true;
      SetEvent(display->getUpdateEvent());
      return 0;
    }
  }
  return SDisplayCorePolling::processMessage(msg, wParam, lParam);
}

void SDisplayCoreWMHooks::flushUpdates() {
  // Poll any visible console windows
  if (pollConsoles) {
    pollConsoles = false;
    poller.processEvent();
  }

  // Check for updates from the hooks
  hooks.getUpdates(updateTracker);

  // Check for updates from the polling Core
  SDisplayCorePolling::flushUpdates();
}
