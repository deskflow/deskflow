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

// -=- LowLevelKeyEvents.h
//
// This interface allows keyboard events destined for a particular window
// to be intercepted early in the keyboard message queue and posted directly
// to the window.  This is used to avoid having the operating system process
// keys such as VK_LWIN, VK_RWIN, etc.
//

#ifndef __RFB_WIN32_LOW_LEVEL_KEY_EVENTS_H__
#define __RFB_WIN32_LOW_LEVEL_KEY_EVENTS_H__

namespace rfb {

  namespace win32 {

    // enableLowLevelKeyEvents
    //   Specifies that keyboard events destined for the specified window should
    //   be posted directly to the window, rather than being passed via the normal
    //   Windows keyboard message queue.
    bool enableLowLevelKeyEvents(HWND hwnd);

    // disableLowLevelKeyEvents
    //   Causes the specified window to revert to the normal Windows keyboard
    //   event processing mechanism.
    void disableLowLevelKeyEvents(HWND hwnd);

  };

};

#endif // __RFB_WIN32_LOW_LEVEL_KEY_EVENTS_H__
