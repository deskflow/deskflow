/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined(dfwhook_EXPORTS)
#define CDESKFLOWHOOK_API __declspec(dllexport)
#else
#define CDESKFLOWHOOK_API __declspec(dllimport)
#endif

#define DESKFLOW_MSG_MARK WM_APP + 0x0011         // mark id; <unused>
#define DESKFLOW_MSG_KEY WM_APP + 0x0012          // vk code; key data
#define DESKFLOW_MSG_MOUSE_BUTTON WM_APP + 0x0013 // button msg; <unused>
#define DESKFLOW_MSG_MOUSE_WHEEL WM_APP + 0x0014  // delta; <unused>
#define DESKFLOW_MSG_MOUSE_MOVE WM_APP + 0x0015   // x; y
#define DESKFLOW_MSG_POST_WARP WM_APP + 0x0016    // <unused>; <unused>
#define DESKFLOW_MSG_PRE_WARP WM_APP + 0x0017     // x; y
#define DESKFLOW_MSG_SCREEN_SAVER WM_APP + 0x0018 // activated; <unused>
#define DESKFLOW_MSG_DEBUG WM_APP + 0x0019        // data, data
#define DESKFLOW_MSG_INPUT_FIRST DESKFLOW_MSG_KEY
#define DESKFLOW_MSG_INPUT_LAST DESKFLOW_MSG_PRE_WARP
#define DESKFLOW_HOOK_LAST_MSG DESKFLOW_MSG_DEBUG

#define DESKFLOW_HOOK_FAKE_INPUT_VIRTUAL_KEY VK_CANCEL
#define DESKFLOW_HOOK_FAKE_INPUT_SCANCODE 0

extern "C"
{

  enum EHookResult
  {
    kHOOK_FAILED,
    kHOOK_OKAY,
    kHOOK_OKAY_LL
  };

  enum EHookMode
  {
    kHOOK_DISABLE,
    kHOOK_WATCH_JUMP_ZONE,
    kHOOK_RELAY_EVENTS
  };
}
