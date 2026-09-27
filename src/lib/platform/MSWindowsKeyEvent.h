/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyTypes.h"

#include <Windows.h>

namespace deskflow::windows {

// IME mode commands injected by remappers need not have scan codes. Give
// them distinct buttons in the unused E0 break-code range, below the
// key state's 0x200 button limit. Never confuse them with Shift or Alt.
inline KeyButton imeButton(UINT virtualKey)
{
  switch (virtualKey) {
  case VK_IME_ON:
    return 0x1f0;
  case VK_IME_OFF:
    return 0x1f1;
  default:
    return 0;
  }
}

inline KeyID imeKeyID(UINT virtualKey)
{
  switch (virtualKey) {
  case VK_IME_ON:
    return kKeyHenkan; // macOS JIS Kana
  case VK_IME_OFF:
    return kKeyZenkaku; // macOS JIS Eisu (existing protocol mapping)
  default:
    return kKeyNone;
  }
}

inline LPARAM normalizeKeyEvent(UINT virtualKey, LPARAM info, KeyButton fallback)
{
  KeyButton button = imeButton(virtualKey);
  if (button == 0) {
    button = static_cast<KeyButton>((info >> 16) & 0x1ffu);
    if (button == 0) {
      button = fallback;
    }
  }
  return (info & ~static_cast<LPARAM>(0x01ff0000u)) | (static_cast<LPARAM>(button) << 16);
}

} // namespace deskflow::windows
