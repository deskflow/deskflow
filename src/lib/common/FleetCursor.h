/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

namespace deskflow::common {

//! True when \p cursorHost names this machine's screen/host.
//!
//! Coordinated fleets should keep screen names aligned with computer names
//! (or map through fleet screens before comparing).
inline bool cursorHostIsLocal(const std::string &selfName, const std::string &cursorHost)
{
  return !cursorHost.empty() && selfName == cursorHost;
}

} // namespace deskflow::common
