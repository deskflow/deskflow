/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

namespace deskflow::coordination {

//! Whether a local key event should reach the local OS (true) or be relayed (false).
/*!
\param cursorOnSelf True when CoordinationScreenEntered has fired and the shared
       cursor is on this machine (ElectionState::cursorHere()).
\param cursorQueryAvailable False when the relay monitor has no cursor callback.
       When unavailable, keys always pass through to the local OS (safe default).
*/
inline bool passKeyToLocalOs(bool cursorOnSelf, bool cursorQueryAvailable = true)
{
  return !cursorQueryAvailable || cursorOnSelf;
}

} // namespace deskflow::coordination
