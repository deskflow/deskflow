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

// -=- WMPoller.cxx

#include <rfb_win32/WMPoller.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>

#include <tchar.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMPoller");

BoolParameter rfb::win32::WMPoller::poll_console_windows("PollConsoleWindows",
  "Server should poll console windows for updates", true);

// -=- WMPoller class

bool
rfb::win32::WMPoller::processEvent() {
  PollInfo info;
  if (poll_console_windows && ut) {
    ::EnumWindows(WMPoller::enumWindowProc, (LPARAM) &info);
    ut->add_changed(info.poll_include);
  }
  return false;
}

bool
rfb::win32::WMPoller::setUpdateTracker(UpdateTracker* ut_) {
  ut = ut_;
  return true;
}

bool
rfb::win32::WMPoller::checkPollWindow(HWND w) {
  TCHAR buffer[128];
  if (!GetClassName(w, buffer, 128))
    throw rdr::SystemException("unable to get window class:%u", GetLastError());
  if ((_tcscmp(buffer, _T("tty")) != 0) &&
    (_tcscmp(buffer, _T("ConsoleWindowClass")) != 0)) {
    return false;
  }
  return true;
}

void
rfb::win32::WMPoller::pollWindow(HWND w, PollInfo* i) {
  RECT r;
  if (IsWindowVisible(w) && GetWindowRect(w, &r)) {
    if (IsRectEmpty(&r)) return;
    Region wrgn(Rect(r.left, r.top, r.right, r.bottom));
    if (checkPollWindow(w)) {
      wrgn.assign_subtract(i->poll_exclude);
      i->poll_include.assign_union(wrgn);
    } else {
      i->poll_exclude.assign_union(wrgn);
    }
  }
}

BOOL CALLBACK
rfb::win32::WMPoller::enumWindowProc(HWND w, LPARAM lp) {
  pollWindow(w, (PollInfo*)lp);
  return TRUE;
}
