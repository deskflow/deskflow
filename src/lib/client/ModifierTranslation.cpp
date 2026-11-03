/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ModifierTranslation.h"

#include <algorithm>
#include <cstddef>

ModifierTranslation::ModifierTranslation(uint32_t clientPlatform) : m_clientPlatform(clientPlatform)
{
  reset();
}

void ModifierTranslation::reset()
{
  m_serverPlatform = kKeyboardPlatformUnknown;
  m_autoSwitch = true;
  for (KeyModifierID id = 0; id < kKeyModifierIDLast; ++id) {
    m_configured[id] = id;
  }
  updateActiveMapping();
}

void ModifierTranslation::setOptions(const OptionsList &options)
{
  if (options.size() % 2 != 0) {
    return;
  }

  for (std::size_t i = 0; i < options.size(); i += 2) {
    KeyModifierID id = kKeyModifierIDNull;
    if (options[i] == kOptionModifierMapForShift) {
      id = kKeyModifierIDShift;
    } else if (options[i] == kOptionModifierMapForControl) {
      id = kKeyModifierIDControl;
    } else if (options[i] == kOptionModifierMapForAlt) {
      id = kKeyModifierIDAlt;
    } else if (options[i] == kOptionModifierMapForAltGr) {
      id = kKeyModifierIDAltGr;
    } else if (options[i] == kOptionModifierMapForMeta) {
      id = kKeyModifierIDMeta;
    } else if (options[i] == kOptionModifierMapForSuper) {
      id = kKeyModifierIDSuper;
    } else if (options[i] == kOptionServerKeyboardPlatform) {
      m_serverPlatform = options[i + 1];
    } else if (options[i] == kOptionAutoModifierSwitch) {
      m_autoSwitch = options[i + 1] != 0;
    }

    if (id != kKeyModifierIDNull) {
      m_configured[id] = std::clamp<KeyModifierID>(options[i + 1], 0, kKeyModifierIDLast - 1);
    }
  }
  updateActiveMapping();
}

void ModifierTranslation::updateActiveMapping()
{
  m_active = m_configured;
  m_autoSwitchActive = false;

  const bool isWindowsMacPair =
      (m_serverPlatform == kKeyboardPlatformWindows && m_clientPlatform == kKeyboardPlatformMacOS) ||
      (m_serverPlatform == kKeyboardPlatformMacOS && m_clientPlatform == kKeyboardPlatformWindows);
  if (!m_autoSwitch || !isWindowsMacPair) {
    return;
  }

  // A non-default mapping is an explicit user preference. Keep the complete
  // configured table instead of combining it with an implicit swap.
  for (KeyModifierID id = 1; id < kKeyModifierIDLast; ++id) {
    if (m_configured[id] != id) {
      return;
    }
  }

  m_active[kKeyModifierIDAlt] = kKeyModifierIDSuper;
  m_active[kKeyModifierIDSuper] = kKeyModifierIDAlt;
  m_autoSwitchActive = true;
}

KeyID ModifierTranslation::translateKey(KeyID id) const
{
  static const KeyID keys[kKeyModifierIDLast][2] = {
      {kKeyNone, kKeyNone},     {kKeyShift_L, kKeyShift_R}, {kKeyControl_L, kKeyControl_R}, {kKeyAlt_L, kKeyAlt_R},
      {kKeyMeta_L, kKeyMeta_R}, {kKeySuper_L, kKeySuper_R}, {kKeyAltGr, kKeyAltGr}
  };

  // AltGr is a right-side key even though the table contains it twice.
  if (id == kKeyAltGr) {
    return keys[m_active[kKeyModifierIDAltGr]][1];
  }
  for (KeyModifierID modifier = 1; modifier < kKeyModifierIDAltGr; ++modifier) {
    for (std::size_t side = 0; side < 2; ++side) {
      if (keys[modifier][side] == id) {
        return keys[m_active[modifier]][side];
      }
    }
  }
  return id;
}

KeyModifierMask ModifierTranslation::translateMask(KeyModifierMask mask) const
{
  static const KeyModifierMask masks[kKeyModifierIDLast] = {
      0, KeyModifierShift, KeyModifierControl, KeyModifierAlt, KeyModifierMeta, KeyModifierSuper, KeyModifierAltGr
  };

  KeyModifierMask result = mask & ~(KeyModifierShift | KeyModifierControl | KeyModifierAlt | KeyModifierMeta |
                                    KeyModifierSuper | KeyModifierAltGr);
  for (KeyModifierID id = 1; id < kKeyModifierIDLast; ++id) {
    if ((mask & masks[id]) != 0) {
      result |= masks[m_active[id]];
    }
  }
  return result;
}
