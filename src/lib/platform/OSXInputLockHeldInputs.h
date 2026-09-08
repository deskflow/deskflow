/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXInputLockModifiers.h"
#include <set>

// Track the input stream before filtering it. Quartz source-state tables are
// updated after filtering, so a swallowed down/up cannot be reconciled with
// those tables. Keep different event sources' observed holds independent.
struct OSXInputLockHeldInputs
{
  // Only at initialization, serialized with the capture run loop before any
  // callback is delivered. This does not repair history after capture loss.
  template <class KeyDown, class ButtonDown> void seedHid(KeyDown keyDown, ButtonDown buttonDown)
  {
    for (unsigned key = 0; key < hidKeys.size(); ++key) {
      hidKeys[key] = key != kVK_CapsLock && keyDown(key);
    }
    for (unsigned button = 0; button < hidButtons.size(); ++button) {
      hidButtons[button] = buttonDown(button);
    }
  }

  void key(unsigned code, CGEventType type, CGEventFlags flags, bool hid)
  {
    auto &keys = hid ? hidKeys : otherKeys;
    if (code >= keys.size() || code == kVK_CapsLock) {
      return;
    }
    if (type == kCGEventFlagsChanged) {
      // Flags may include modifiers held by a different source. Only the
      // identified modifier transition belongs to this source's history.
      updateInputLockModifiers(keys, flags, code);
    } else {
      keys[code] = type == kCGEventKeyDown;
    }
  }

  void button(unsigned code, bool down, bool hid)
  {
    auto &buttons = hid ? hidButtons : otherButtons;
    if (code < buttons.size()) {
      buttons[code] = down;
    }
  }

  void mediaKey(uint16_t code, bool down)
  {
    if (down)
      media.insert(code);
    else
      media.erase(code);
  }

  bool keyHeld(unsigned code) const
  {
    return code < hidKeys.size() && (hidKeys[code] || otherKeys[code]);
  }

  bool hasNonHidHold() const
  {
    return otherKeys.any() || otherButtons.any() || !media.empty();
  }
  bool empty() const
  {
    return hidKeys.none() && hidButtons.none() && !hasNonHidHold();
  }

  void invalidateHistory()
  {
    m_historyLost = true;
  }

  bool historyLost() const
  {
    return m_historyLost;
  }

  bool readyToResume() const
  {
    return !m_historyLost && empty();
  }

  std::bitset<128> hidKeys, otherKeys;
  std::bitset<32> hidButtons, otherButtons;
  std::set<uint16_t> media;

private:
  bool m_historyLost = false;
};
