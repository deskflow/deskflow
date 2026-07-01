/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/KeyboardRelayMap.h"

#include "deskflow/KeyTypes.h"

#include <windows.h>

namespace deskflow::coordination {

namespace {

KeyModifierMask activeModifiers()
{
  KeyModifierMask mask = 0;
  if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
    mask |= KeyModifierShift;
  }
  if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
    mask |= KeyModifierControl;
  }
  if (GetAsyncKeyState(VK_MENU) & 0x8000) {
    mask |= KeyModifierAlt;
  }
  if (GetAsyncKeyState(VK_LWIN) & 0x8000 || GetAsyncKeyState(VK_RWIN) & 0x8000) {
    mask |= KeyModifierSuper;
  }
  if (GetKeyState(VK_CAPITAL) & 1) {
    mask |= KeyModifierCapsLock;
  }
  return mask;
}

KeyID mapVirtualKey(int vkCode)
{
  switch (vkCode) {
  case VK_RETURN:
    return kKeyReturn;
  case VK_TAB:
    return kKeyTab;
  case VK_SPACE:
    return static_cast<KeyID>(' ');
  case VK_BACK:
    return kKeyBackSpace;
  case VK_DELETE:
    return kKeyDelete;
  case VK_ESCAPE:
    return kKeyEscape;
  case VK_LEFT:
    return kKeyLeft;
  case VK_RIGHT:
    return kKeyRight;
  case VK_UP:
    return kKeyUp;
  case VK_DOWN:
    return kKeyDown;
  case VK_HOME:
    return kKeyHome;
  case VK_END:
    return kKeyEnd;
  case VK_PRIOR:
    return kKeyPageUp;
  case VK_NEXT:
    return kKeyPageDown;
  default:
    break;
  }

  const BYTE keyboardState[256] = {};
  WCHAR buffer[8] = {};
  const int rc = ToUnicodeEx(
      static_cast<UINT>(vkCode), 0, keyboardState, buffer, 8, 0, GetKeyboardLayout(0)
  );
  if (rc == 1 && buffer[0] >= 32) {
    return static_cast<KeyID>(buffer[0]);
  }
  return kKeyNone;
}

} // namespace

bool mapRelayKeyFromHook(
    int vkCode, int scanCode, bool isExtended, bool keyUp, bool isRepeat, KeyID &id, KeyModifierMask &mask,
    KeyButton &button, Message::KeyPhase &phase
)
{
  (void)isExtended;
  (void)scanCode;

  phase = keyUp ? Message::KeyPhase::Up : (isRepeat ? Message::KeyPhase::Repeat : Message::KeyPhase::Down);
  button = static_cast<KeyButton>(vkCode);
  mask = activeModifiers();
  id = mapVirtualKey(vkCode);
  if (keyUp) {
    id = kKeyNone;
    return true;
  }
  return id != kKeyNone || isRepeat;
}

} // namespace deskflow::coordination
