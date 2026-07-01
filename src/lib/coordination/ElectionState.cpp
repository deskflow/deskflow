/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/ElectionState.h"

#include <algorithm>
#include <utility>

namespace deskflow::coordination {

const char *roleName(Role role)
{
  switch (role) {
  case Role::Server:
    return "server";
  case Role::Client:
    return "client";
  default:
    return "init";
  }
}

ElectionState::ElectionState(std::string selfName, ElectionTuning tuning, Clock clock)
    : m_selfName(std::move(selfName)),
      m_tuning(tuning),
      m_clock(std::move(clock))
{
  // do nothing
}

bool ElectionState::onLocalInput()
{
  const double now = m_clock();

  if (m_role == Role::Server) {
    return false; // already primary
  }
  if (now - m_lastSwitchAt < m_tuning.selfCooldownS) {
    return false; // anti-flap right after a role change
  }

  const double window = m_cursorHere ? m_tuning.burstWindowCursorHereS : m_tuning.burstWindowS;
  const int needed = m_cursorHere ? m_tuning.burstCountCursorHere : m_tuning.burstCount;

  while (!m_inputBurst.empty() && now - m_inputBurst.front() > window) {
    m_inputBurst.pop_front();
  }
  m_inputBurst.push_back(now);

  if (static_cast<int>(m_inputBurst.size()) >= needed) {
    m_inputBurst.clear();
    return true;
  }
  return false;
}

void ElectionState::setCursorHere(bool here)
{
  if (m_cursorHere != here) {
    m_cursorHere = here;
    // The thresholds changed; stale ticks must not bridge the two regimes.
    m_inputBurst.clear();
  }
}

ElectionState::ClaimAction
ElectionState::onClaim(const std::string &senderName, const std::string &ip, const std::string &lan, int64_t seq)
{
  m_seq = std::max(m_seq, seq);

  if (senderName == m_selfName) {
    return ClaimAction::Ignore;
  }

  const double now = m_clock();

  // Server stickiness: a fresh server ignores rival claims for a short window.
  if (m_role == Role::Server && now - m_lastSwitchAt < m_tuning.claimCooldownS) {
    return ClaimAction::Ignore;
  }

  // Client/Init anti-flap, symmetric with onLocalInput(): a node that just
  // switched must not immediately follow a DIFFERENT peer's claim. Without
  // this guard two machines ping-pong leadership and churn epochs many times a
  // second -- which in turn races the socket/multiplexer teardown. A freshly
  // booted node is exempt (m_lastSwitchAt starts at -1e9), so first-join
  // following is unaffected, as is following the current server's heartbeat
  // (handled as a strict no-op just below).
  if (m_role != Role::Server && now - m_lastSwitchAt < m_tuning.selfCooldownS) {
    return ClaimAction::Ignore;
  }

  if (m_role == Role::Client && (m_serverAddress == ip || m_serverAddress == lan)) {
    // Heartbeat from the host we already follow: must be a strict no-op
    // (historically, probing here caused connection-backlog storms).
    return ClaimAction::Ignore;
  }

  return ClaimAction::FollowSender;
}

void ElectionState::becameServer()
{
  m_role = Role::Server;
  m_serverAddress.clear();
  m_lastSwitchAt = m_clock();
  m_cursorHere = false;
  m_inputBurst.clear();
}

void ElectionState::becameClient(const std::string &serverAddress)
{
  m_role = Role::Client;
  m_serverAddress = serverAddress;
  m_lastSwitchAt = m_clock();
  // Assumes cursor is remote until Client::enter() posts CoordinationScreenEntered.
  // Keyboard relay forwards while false; server should send enter() promptly when local.
  m_cursorHere = false;
  m_inputBurst.clear();
}

int64_t ElectionState::nextClaimSeq()
{
  return ++m_seq;
}

} // namespace deskflow::coordination
