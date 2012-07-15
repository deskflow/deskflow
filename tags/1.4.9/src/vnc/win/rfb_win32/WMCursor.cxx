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

// -=- WMCursor.cxx

// *** DOESN'T SEEM TO WORK WITH GetCursorInfo POS CODE BUILT-IN UNDER NT4SP6
// *** INSTEAD, WE LOOK FOR Win2000/Win98 OR ABOVE

#include <rfb_win32/WMCursor.h>
#include <rfb_win32/OSVersion.h>
#include <rfb_win32/DynamicFn.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

using namespace rdr;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMCursor");


#ifdef CURSOR_SHOWING
#define RFB_HAVE_GETCURSORINFO
#else
#pragma message("  NOTE: Not building GetCursorInfo support.")
#endif

#ifdef RFB_HAVE_GETCURSORINFO
typedef BOOL (WINAPI *_GetCursorInfo_proto)(PCURSORINFO pci);
DynamicFn<_GetCursorInfo_proto> _GetCursorInfo(_T("user32.dll"), "GetCursorInfo");
#endif

WMCursor::WMCursor() : hooks(0), use_getCursorInfo(false), cursor(0) {
#ifdef RFB_HAVE_GETCURSORINFO
  // Check the OS version
  bool is_win98 = (osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
    (osVersion.dwMajorVersion > 4) || ((osVersion.dwMajorVersion == 4) && (osVersion.dwMinorVersion > 0));
  bool is_win2K = (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osVersion.dwMajorVersion >= 5);

  // Use GetCursorInfo if OS version is sufficient
  use_getCursorInfo = (is_win98 || is_win2K) && _GetCursorInfo.isValid();
#endif
  cursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
  if (!use_getCursorInfo) {
    hooks = new WMCursorHooks();
    if (hooks && hooks->start()) {
      vlog.info("falling back to cursor hooking: %p", hooks);
    } else {
      delete hooks;
      hooks = 0;
      vlog.error("unable to monitor cursor shape");
    }
  } else {
    vlog.info("using GetCursorInfo");
  }
}

WMCursor::~WMCursor() {
  vlog.debug("deleting WMCursorHooks (%p)", hooks);
  if (hooks)
    delete hooks;
}
  
WMCursor::Info
WMCursor::getCursorInfo() {
  Info result;
#ifdef RFB_HAVE_GETCURSORINFO
  if (use_getCursorInfo) {
    CURSORINFO info;
    info.cbSize = sizeof(CURSORINFO);
    if ((*_GetCursorInfo)(&info)) {
      result.cursor = info.hCursor;
      result.position = Point(info.ptScreenPos.x, info.ptScreenPos.y);
      result.visible = info.flags & CURSOR_SHOWING;
      return result;
    }
  }
#endif
  // Fall back to the old way of doing things
  POINT pos;
  if (hooks)
    cursor = hooks->getCursor();
  result.cursor = cursor;
  result.visible = cursor != 0;
  GetCursorPos(&pos);
  result.position.x = pos.x;
  result.position.y = pos.y;
  return result;
}
