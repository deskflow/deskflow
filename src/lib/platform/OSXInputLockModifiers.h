/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <Carbon/Carbon.h>
#include <IOKit/hidsystem/IOLLEvent.h>
#include <bitset>

// Use the event's current flags, not a HID table that can still contain the
// preceding state while inside an event tap. No input API calls are made.
inline void updateInputLockModifiers(std::bitset<128> &keys, CGEventFlags flags, unsigned onlyKey = 128)
{
  const bool allKeys = onlyKey >= keys.size();
  const auto pair = [&](CGKeyCode left, CGKeyCode right, CGEventFlags aggregate, CGEventFlags leftMask,
                        CGEventFlags rightMask) {
    const bool down = (flags & aggregate) != 0;
    // Some synthetic sources only supply aggregate flags. For a local-state
    // snapshot use one left key; for a transition use its identified key.
    if (allKeys || onlyKey == left) {
      keys[left] = down && ((flags & leftMask) != 0 || (flags & (leftMask | rightMask)) == 0);
    }
    if (allKeys || onlyKey == right) {
      keys[right] = down && ((flags & rightMask) != 0 || (!allKeys && (flags & (leftMask | rightMask)) == 0));
    }
  };
  pair(kVK_Shift, kVK_RightShift, kCGEventFlagMaskShift, NX_DEVICELSHIFTKEYMASK, NX_DEVICERSHIFTKEYMASK);
  pair(kVK_Control, kVK_RightControl, kCGEventFlagMaskControl, NX_DEVICELCTLKEYMASK, NX_DEVICERCTLKEYMASK);
  pair(kVK_Option, kVK_RightOption, kCGEventFlagMaskAlternate, NX_DEVICELALTKEYMASK, NX_DEVICERALTKEYMASK);
  pair(kVK_Command, kVK_RightCommand, kCGEventFlagMaskCommand, NX_DEVICELCMDKEYMASK, NX_DEVICERCMDKEYMASK);
  if (allKeys || onlyKey == kVK_Function) {
    keys[kVK_Function] = (flags & kCGEventFlagMaskSecondaryFn) != 0;
  }
  keys[kVK_CapsLock] = false; // preserve the toggle without treating it as held
}
