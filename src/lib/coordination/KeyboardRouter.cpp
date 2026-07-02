/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "common/FleetCursor.h"
#include "coordination/KeyboardRouter.h"

#include "coordination/KeyboardRelayDecision.h"

namespace deskflow::coordination {

using deskflow::common::cursorHostIsLocal;

KeyboardRouteDecision routeKeyboard(const KeyboardRouteInput &input)
{
  if (!input.cursorHostKnown) {
    if (passKeyToLocalOs(false, false, input.secondsSinceRelayStart)) {
      return {KeyboardRoute::Local, {}};
    }
    return {KeyboardRoute::Forward, {}};
  }

  if (input.cursorHost.empty() || cursorHostIsLocal(input.selfName, input.cursorHost)) {
    return {KeyboardRoute::Local, {}};
  }

  return {KeyboardRoute::Forward, input.cursorHost};
}

} // namespace deskflow::coordination
