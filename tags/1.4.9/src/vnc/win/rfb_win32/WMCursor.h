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

// -=- WMCursor.h

// WMCursor provides a single API through which the cursor state can be obtained
// The underlying implementation will use either GetCursorInfo, or use the
// wm_hooks library if GetCursorInfo is not available.

#ifndef __RFB_WIN32_WM_CURSOR_H__
#define __RFB_WIN32_WM_CURSOR_H__

#include <windows.h>
#include <rfb_win32/WMHooks.h>

namespace rfb {
  namespace win32 {

    class WMCursor {
    public:
      WMCursor();
      ~WMCursor();

      struct Info {
        HCURSOR cursor;
        Point position;
        bool visible;
        Info() : cursor(0), visible(false) {}
        bool operator!=(const Info& info) {
          return ((cursor != info.cursor) ||
            (!position.equals(info.position)) ||
            (visible != info.visible));
        }
      };

      Info getCursorInfo();
    protected:
      WMCursorHooks* hooks;
      bool use_getCursorInfo;
      HCURSOR cursor;
    };

  };
};

#endif // __RFB_WIN32_WM_CURSOR_H__
