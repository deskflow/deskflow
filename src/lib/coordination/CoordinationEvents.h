/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "coordination/CoordinationProtocol.h"
#include "deskflow/KeyTypes.h"

#include <string>

//! Event data for CoordinationKeyForward (server injects into m_active).
class CoordinationKeyForwardInfo : public EventData
{
public:
  deskflow::coordination::Message::KeyPhase phase = deskflow::coordination::Message::KeyPhase::Down;
  KeyID id = 0;
  KeyModifierMask mask = 0;
  KeyButton button = 0;
  std::string lang;
  std::string from;

  CoordinationKeyForwardInfo(
      deskflow::coordination::Message::KeyPhase phase, KeyID id, KeyModifierMask mask, KeyButton button,
      std::string lang, std::string from
  )
      : phase(phase),
        id(id),
        mask(mask),
        button(button),
        lang(std::move(lang)),
        from(std::move(from))
  {
  }

  ~CoordinationKeyForwardInfo() override = default;
};
