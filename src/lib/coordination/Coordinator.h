/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/CoordinationMesh.h"
#include "coordination/ElectionState.h"
#include "coordination/LocalInputMonitor.h"
#include "coordination/Peer.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace deskflow::coordination {

//! Coordinator configuration (from Settings; see design.md).
struct CoordinatorConfig
{
  std::string selfName;
  int meshPort = 24851;
  int deskflowPort = 24800;
  std::string token;
  PeerList peers;
  ElectionTuning tuning;
};

//! What the epoch loop should run next.
struct RoleDecision
{
  Role role = Role::Init;
  std::string serverAddress; // when role == Client
  bool quit = false;
};

//! Orchestrates election, mesh, input detection, and the reconciler.
/*!
Owns its threads and outlives the per-role App epochs. The epoch loop in
deskflow-core blocks in awaitRoleDecision(); when the election decides a
flip, the coordinator records the new decision, wakes the epoch loop, and
interrupts the currently running app via the registered callback.
*/
class Coordinator
{
public:
  explicit Coordinator(CoordinatorConfig config);
  Coordinator(const Coordinator &) = delete;
  Coordinator &operator=(const Coordinator &) = delete;
  ~Coordinator();

  bool start();
  void stop();

  //! Block until a (new) role decision or quit is available.
  RoleDecision awaitRoleDecision();

  //! Register how to interrupt the currently running app epoch.
  /*!
  Called (from coordinator threads) whenever a decision is made while an
  epoch runs; typically posts EventTypes::Quit to the app event queue.
  */
  void setInterruptCallback(std::function<void()> interrupt);

  //! The running client observed the shared cursor entering/leaving us.
  void notifyCursorHere(bool here);

  //! The running epoch ended on its own (app error); re-arm the same role.
  void notifyEpochEnded();

  //! Request a graceful process shutdown.
  void requestQuit();

  //! True when a decision (or quit) is waiting to be consumed.
  /*!
  The epoch loop re-checks this right after starting an app so a decision
  made in the gap between awaitRoleDecision() and the app's event loop
  cannot be missed.
  */
  bool hasPendingDecision();

private:
  void onMessage(const Message &message, const std::function<void(const std::string &)> &reply);
  void onGenuineInput();
  void promoteSelf(const char *reason);
  void followSender(const Message &claim);
  void decide(Role role, const std::string &serverAddress);
  void broadcastClaim();
  void workerLoop();
  void discoverOnce();

  CoordinatorConfig m_config;
  std::unique_ptr<CoordinationMesh> m_mesh;
  std::unique_ptr<ILocalInputMonitor> m_inputMonitor;

  std::mutex m_mutex; // guards election state + decision + interrupt
  ElectionState m_election;
  std::function<void()> m_interrupt;
  RoleDecision m_decision;
  bool m_hasDecision = false;
  bool m_quit = false;
  std::condition_variable m_decisionReady;

  std::thread m_worker;
  std::condition_variable m_workerWake;
  bool m_workerStop = false;
  bool m_broadcastPending = false;
  double m_startedAt = 0.0;
  int m_wedgeStrikes = 0;
};

} // namespace deskflow::coordination
