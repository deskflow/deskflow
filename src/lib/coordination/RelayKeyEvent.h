/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyTypes.h"

#include <string>

namespace deskflow::coordination {

//! Neutral keyboard relay phase (no mesh protocol dependency).
enum class RelayKeyPhase
{
  Down,
  Up,
  Repeat
};

//! Neutral keyboard relay payload for server/client injection.
struct RelayKeyEvent
{
  RelayKeyPhase phase = RelayKeyPhase::Down;
  KeyID id = 0;
  KeyModifierMask mask = 0;
  KeyButton button = 0;
  std::string lang;
  std::string from;
};

} // namespace deskflow::coordination
