/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

namespace deskflow::coordination {

//! Grace period after client-epoch relay start while screen enter/leave is pending.
inline constexpr double kCursorRelayBootGraceS = 0.3;

//! Whether a local key event should reach the local OS (true) or be relayed (false).
/*!
\param cursorOnSelf True when CoordinationScreenEntered has fired and the shared
       cursor is on this machine (ElectionState::cursorHere()).
\param cursorScreenKnown False until the first enter/leave after a client epoch starts.
\param secondsSinceRelayStart Elapsed time since the relay monitor started this epoch.
\param bootGraceS Boot grace window; while unknown and within grace, pass keys locally.
*/
inline bool passKeyToLocalOs(
    bool cursorOnSelf,
    bool cursorScreenKnown,
    double secondsSinceRelayStart,
    double bootGraceS = kCursorRelayBootGraceS
)
{
  if (!cursorScreenKnown) {
    return secondsSinceRelayStart < bootGraceS;
  }
  return cursorOnSelf;
}

} // namespace deskflow::coordination
