/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

namespace deskflow {

/**
 * @brief What a client should do when it receives a power "resume" signal.
 */
enum class ResumeAction
{
  None,               //!< Do nothing.
  Reconnect,          //!< Establish a fresh connection (we are disconnected).
  DropStaleConnection //!< Tear down an apparently-connected-but-stale link so
                      //!< the normal auto-reconnect re-establishes it.
};

/**
 * @brief Decide how a client should react to a power resume signal.
 *
 * Pure policy, extracted so it can be unit-tested without the client's Qt event
 * machinery.
 *
 * Background: some platforms (Windows 10/11) deliver a resume signal (a
 * repurposed WM_TIMECHANGE) with no preceding suspend signal. The historical
 * logic only reconnected when a suspend had been recorded, so on those
 * platforms a wake left the now-dead connection in place until keep-alive death
 * detection noticed it (~9 s of dead input).
 *
 * @param wasSuspended     A matching suspend signal was recorded.
 * @param wantedConnection There was a connection we intend to restore (only
 *                         meaningful on the suspend path).
 * @param appearsConnected We currently believe we are connected. After a real
 *                         wake this is stale-true until the dead socket is
 *                         noticed.
 */
constexpr ResumeAction decideResumeAction(bool wasSuspended, bool wantedConnection, bool appearsConnected)
{
  if (wasSuspended) {
    // Normal suspend->resume: we disconnected on suspend, so reconnect iff we
    // had a live connection beforehand.
    return wantedConnection ? ResumeAction::Reconnect : ResumeAction::None;
  }

  // Resume with no recorded suspend (missed-suspend / Windows wake). If we still
  // look connected, that link is stale after a wake: drop it so auto-reconnect
  // re-establishes promptly. If we already look disconnected, a reconnect is
  // already in flight, so there is nothing to do.
  return appearsConnected ? ResumeAction::DropStaleConnection : ResumeAction::None;
}

} // namespace deskflow
