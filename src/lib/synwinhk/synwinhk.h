/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// hack: vs2005 doesn't declare _WIN32_WINNT, so we need to hard code it.
// however, some say that this should be hard coded since it defines the
// target system, but since this is suposed to compile on pre-XP, maybe
// we should just leave it like this.
#if _MSC_VER == 1400
#define _WIN32_WINNT 0x0400
#endif

#include "base/EventTypes.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined(synwinhk_EXPORTS)
#define CSYNERGYHOOK_API __declspec(dllexport)
#else
#define CSYNERGYHOOK_API __declspec(dllimport)
#endif

#define SYNERGY_MSG_MARK WM_APP + 0x0011         // mark id; <unused>
#define SYNERGY_MSG_KEY WM_APP + 0x0012          // vk code; key data
#define SYNERGY_MSG_MOUSE_BUTTON WM_APP + 0x0013 // button msg; <unused>
#define SYNERGY_MSG_MOUSE_WHEEL WM_APP + 0x0014  // delta; <unused>
#define SYNERGY_MSG_MOUSE_MOVE WM_APP + 0x0015   // x; y
#define SYNERGY_MSG_POST_WARP WM_APP + 0x0016    // <unused>; <unused>
#define SYNERGY_MSG_PRE_WARP WM_APP + 0x0017     // x; y
#define SYNERGY_MSG_SCREEN_SAVER WM_APP + 0x0018 // activated; <unused>
#define SYNERGY_MSG_DEBUG WM_APP + 0x0019        // data, data
#define SYNERGY_MSG_INPUT_FIRST SYNERGY_MSG_KEY
#define SYNERGY_MSG_INPUT_LAST SYNERGY_MSG_PRE_WARP
#define SYNERGY_HOOK_LAST_MSG SYNERGY_MSG_DEBUG

#define SYNERGY_HOOK_FAKE_INPUT_VIRTUAL_KEY VK_CANCEL
#define SYNERGY_HOOK_FAKE_INPUT_SCANCODE 0

extern "C" {

enum EHookResult { kHOOK_FAILED, kHOOK_OKAY, kHOOK_OKAY_LL };

enum EHookMode { kHOOK_DISABLE, kHOOK_WATCH_JUMP_ZONE, kHOOK_RELAY_EVENTS };

typedef int (*InitFunc) (DWORD targetQueueThreadID);
typedef int (*CleanupFunc) (void);
typedef EHookResult (*InstallFunc) (void);
typedef int (*UninstallFunc) (void);
typedef int (*InstallScreenSaverFunc) (void);
typedef int (*UninstallScreenSaverFunc) (void);
typedef void (*SetSidesFunc) (UInt32);
typedef void (*SetZoneFunc) (SInt32, SInt32, SInt32, SInt32, SInt32);
typedef void (*SetModeFunc) (int);

CSYNERGYHOOK_API int init (DWORD);
CSYNERGYHOOK_API int cleanup (void);
CSYNERGYHOOK_API EHookResult install (void);
CSYNERGYHOOK_API int uninstall (void);
CSYNERGYHOOK_API int installScreenSaver (void);
CSYNERGYHOOK_API int uninstallScreenSaver (void);
CSYNERGYHOOK_API void setSides (UInt32 sides);
CSYNERGYHOOK_API void
setZone (SInt32 x, SInt32 y, SInt32 w, SInt32 h, SInt32 jumpZoneSize);
CSYNERGYHOOK_API void setMode (EHookMode mode);
}
