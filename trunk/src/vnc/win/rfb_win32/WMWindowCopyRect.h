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

// -=- WMWindowCopyRect.h
//
// Helper class which produces copyRect actions by monitoring the location
// of the current foreground window.
// Whenever processEvent is called, the foreground window's position is
// recalculated and a copy event flushed to the supplied UpdateTracker
// if appropriate.

#ifndef __RFB_WIN32_WM_WINDOW_COPYRECT_H__
#define __RFB_WIN32_WM_WINDOW_COPYRECT_H__

#include <rfb/UpdateTracker.h>

namespace rfb {

  namespace win32 {

    class WMCopyRect {
    public:
      WMCopyRect();

      bool processEvent();
      bool setUpdateTracker(UpdateTracker* ut);

    protected:
      UpdateTracker* ut;
      void* fg_window;
      Rect fg_window_rect;
    };

  };

};

#endif // __RFB_WIN32_WM_WINDOW_COPYRECT_H__
