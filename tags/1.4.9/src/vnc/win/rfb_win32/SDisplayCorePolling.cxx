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

// -=- SDisplayCorePolling.cxx

#include <rfb_win32/SDisplayCorePolling.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SDisplayCorePolling");

const int POLLING_SEGMENTS = 16;

const int SDisplayCorePolling::pollTimerId = 1;

SDisplayCorePolling::SDisplayCorePolling(SDisplay* d, UpdateTracker* ut, int pollInterval_)
  : MsgWindow(_T("rfb::win32::SDisplayCorePolling")), updateTracker(ut),
  pollTimer(getHandle(), pollTimerId), pollNextStrip(false), display(d) {
  pollInterval = max(10, (pollInterval_ / POLLING_SEGMENTS));
  copyrect.setUpdateTracker(ut);
}

SDisplayCorePolling::~SDisplayCorePolling() {
}

LRESULT SDisplayCorePolling::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_TIMER && wParam == pollTimerId) {
    pollNextStrip = true;
    SetEvent(display->getUpdateEvent());
    return 0;
  }
  return MsgWindow::processMessage(msg, wParam, lParam);
}

void SDisplayCorePolling::setScreenRect(const Rect& screenRect_) {
  vlog.info("setScreenRect");
  screenRect = screenRect_;
  pollIncrementY = (screenRect.height()+POLLING_SEGMENTS-1)/POLLING_SEGMENTS;
  pollNextY = screenRect.tl.y;
  pollTimer.start(pollInterval);
}

void SDisplayCorePolling::flushUpdates() {
  vlog.write(120, "flushUpdates");

  // Check for window movement
  while (copyrect.processEvent()) {}

  if (pollNextStrip) {
    // Poll the next strip of the screen (in Screen coordinates)
    pollNextStrip = false;
    Rect pollrect = screenRect;
    if (pollNextY >= pollrect.br.y) {
      // Yes.  Reset the counter and return
      pollNextY = pollrect.tl.y;
    } else {
      // No.  Poll the next section
      pollrect.tl.y = pollNextY;
      pollNextY += pollIncrementY;
      pollrect.br.y = min(pollNextY, pollrect.br.y);
      updateTracker->add_changed(pollrect);
    }
  }
}
