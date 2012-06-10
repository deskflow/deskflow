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

// -=- WMCopyRect.cxx

#include <rfb_win32/WMWindowCopyRect.h>
#include <rfb/LogWriter.h>
#include <windows.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMCopyRect");

// -=- WMHooks class

rfb::win32::WMCopyRect::WMCopyRect() : ut(0), fg_window(0) {
}

bool
rfb::win32::WMCopyRect::processEvent() {
  // See if the foreground window has moved
  HWND window = GetForegroundWindow();
  if (window) {
    RECT wrect;
    if (IsWindow(window) && IsWindowVisible(window) && GetWindowRect(window, &wrect)) {
      Rect winrect(wrect.left, wrect.top, wrect.right, wrect.bottom);
      if (fg_window == window) {
        if (!fg_window_rect.tl.equals(winrect.tl)  && ut) {
          // Window has moved - send a copyrect event to the client
          Point delta = Point(winrect.tl.x-fg_window_rect.tl.x, winrect.tl.y-fg_window_rect.tl.y);
          Region copy_dest = winrect;
          ut->add_copied(copy_dest, delta);
          ut->add_changed(Region(fg_window_rect).subtract(copy_dest));
        }
      }
      fg_window = window;
      fg_window_rect = winrect;
    } else {
      fg_window = 0;
    }
  } else {
    fg_window = 0;
  }
  return false;
}

bool
rfb::win32::WMCopyRect::setUpdateTracker(UpdateTracker* ut_) {
  ut = ut_;
  return true;
}
