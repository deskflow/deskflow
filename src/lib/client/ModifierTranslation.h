/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyTypes.h"
#include "deskflow/OptionTypes.h"

#include <array>

// Maintains the explicit per-computer mapping separately from the automatic
// Windows/macOS mapping, so updates and resets do not compound the swap.
class ModifierTranslation
{
public:
  explicit ModifierTranslation(uint32_t clientPlatform = kCurrentKeyboardPlatform);

  void reset();
  void setOptions(const OptionsList &options);
  KeyID translateKey(KeyID id) const;
  KeyModifierMask translateMask(KeyModifierMask mask) const;
  bool isAutoSwitchActive() const
  {
    return m_autoSwitchActive;
  }

private:
  void updateActiveMapping();

  uint32_t m_clientPlatform;
  uint32_t m_serverPlatform = kKeyboardPlatformUnknown;
  bool m_autoSwitch = true;
  bool m_autoSwitchActive = false;
  std::array<KeyModifierID, kKeyModifierIDLast> m_configured{};
  std::array<KeyModifierID, kKeyModifierIDLast> m_active{};
};
