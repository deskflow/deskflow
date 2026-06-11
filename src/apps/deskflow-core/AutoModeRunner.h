/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/ElectionState.h"

#include <QString>

#include <atomic>
#include <memory>

class EventQueue;
class QThread;

namespace deskflow::coordination {
class Coordinator;
}

//! Runs deskflow-core in "auto" mode: the coordination epoch loop.
/*!
Owns a Coordinator for the process lifetime and repeatedly constructs,
runs, and destroys a ServerApp or ClientApp per elected role (see
docs/coordination/design.md). One process, one TCC identity, roles flip
in place.
*/
class AutoModeRunner
{
public:
  AutoModeRunner(EventQueue &events, QString processName);
  AutoModeRunner(const AutoModeRunner &) = delete;
  AutoModeRunner &operator=(const AutoModeRunner &) = delete;
  ~AutoModeRunner();

  //! Start the epoch loop on \p coreThread (mirrors App::run()).
  void run(QThread &coreThread);

  //! Graceful shutdown (wired to the IPC stop request).
  void requestQuit();

  int exitCode() const
  {
    return m_exitCode;
  }

private:
  void epochLoop();
  int runEpoch(deskflow::coordination::Role role, const std::string &serverAddress);

  EventQueue &m_events;
  QString m_processName;
  std::unique_ptr<deskflow::coordination::Coordinator> m_coordinator;
  std::atomic<bool> m_appRunning{false};
  std::atomic<int> m_exitCode{0};
};
