/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace deskflow::coordination {

//! Coordination role of this node.
enum class Role
{
  Init,
  Client,
  Server
};

const char *roleName(Role role);

//! Election timing constants (see docs/coordination/behavior-spec.md §3).
struct ElectionTuning
{
  //! Ignore local promotion triggers this long after any role change.
  double selfCooldownS = 2.5;
  //! While server, ignore inbound claims this long after the flip.
  double claimCooldownS = 1.5;
  //! Claim heartbeat cadence while server.
  double heartbeatIntervalS = 3.0;
  //! Genuine-input burst to promote: count within window.
  int burstCount = 4;
  double burstWindowS = 0.40;
  //! Stricter burst while the shared cursor is on this screen, so
  //! forwarded motion echoes can never promote a passive client.
  int burstCountCursorHere = 12;
  double burstWindowCursorHereS = 0.80;
};

//! Pure last-touch-wins election logic.
/*!
Deterministic and clock-injected: owns no sockets, timers, or threads.
The \c Coordinator feeds it inputs (local hardware input ticks, inbound
claims, manual promotes) and applies its decisions (role transitions are
confirmed back via becameServer() / becameClient() once the transport
flip actually happened).
*/
class ElectionState
{
public:
  using Clock = std::function<double()>;

  //! What to do with an inbound claim.
  enum class ClaimAction
  {
    Ignore,
    FollowSender
  };

  ElectionState(std::string selfName, ElectionTuning tuning, Clock clock);

  //! @name inputs
  //@{

  //! Record one genuine local hardware input event.
  /*!
  Returns true when this event completes a promotion-worthy burst
  (the burst is then consumed). Always false while already server or
  within the self-cooldown of the last role change.
  */
  bool onLocalInput();

  //! Track whether the shared cursor is currently on this screen.
  void setCursorHere(bool here);

  //! Evaluate an inbound claim; merges the sender's sequence number.
  ClaimAction onClaim(const std::string &senderName, const std::string &ip, const std::string &lan, int64_t seq);

  //@}
  //! @name transitions (applied by the coordinator)
  //@{

  void becameServer();
  void becameClient(const std::string &serverAddress);

  //@}
  //! @name accessors
  //@{

  Role role() const
  {
    return m_role;
  }
  const std::string &serverAddress() const
  {
    return m_serverAddress;
  }
  double lastSwitchAt() const
  {
    return m_lastSwitchAt;
  }
  int64_t seq() const
  {
    return m_seq;
  }
  bool cursorHere() const
  {
    return m_cursorHere;
  }

  //! Increment and return the sequence number for an outbound claim.
  int64_t nextClaimSeq();

  //@}

private:
  std::string m_selfName;
  ElectionTuning m_tuning;
  Clock m_clock;
  Role m_role = Role::Init;
  std::string m_serverAddress;
  double m_lastSwitchAt = -1.0e9; // long ago: no cooldown at boot
  int64_t m_seq = 0;
  bool m_cursorHere = false;
  std::deque<double> m_inputBurst;
};

} // namespace deskflow::coordination
