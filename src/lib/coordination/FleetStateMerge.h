/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/FleetState.h"

namespace deskflow::coordination {

struct FleetMergeResult
{
  bool changed = false;
  bool topologyBecameReady = false;
};

//! Apply a server-authoritative topology/cursor fragment into \p state.
FleetMergeResult applyServerFragment(FleetState &state, const FleetFragment &fragment);

} // namespace deskflow::coordination
