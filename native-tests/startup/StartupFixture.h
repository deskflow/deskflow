// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "gui/core/StartupRecovery.h"
#include <QDir>
#include <atomic>
namespace startup_fixture {
using namespace deskflow::gui::startup;
inline Snapshot snapshot()
{
  Snapshot s;
  s.supported = true;
  s.home = QDir::cleanPath(QDir::rootPath() + "/Deskflow-Startup-Fixture");
  s.domain = "gui/501";
  s.executable = s.home + "/DeskflowCandidate.app/Contents/MacOS/DeskflowCandidate";
  s.watchdogHash = "fixture-script";
  for (const auto &label : {original, watchdog, keepawake, candidate}) {
    Service job;
    job.label = label;
    job.path = s.home + "/Library/LaunchAgents/" + label + ".plist";
    job.known = true;
    if (label != candidate) {
      job.definition = "fixture-" + label.toUtf8();
      job.loaded = true;
      job.login = true;
      job.running = label != watchdog;
    }
    s.services.insert(label, job);
  }
  s.services[watchdog].interval = 15;
  s.services[original].alwaysAlive = true;
  return s;
}
class FakeReader : public Reader
{
public:
  Snapshot state = snapshot();
  std::atomic<int> reads{0};
  Snapshot read() override
  {
    ++reads;
    return state;
  }
};
class FakeExecutor : public Executor
{
public:
  Snapshot state = snapshot();
  bool permitted = true, emptyReceipt = false, restoreFails = false, omitEffects = false, failToRun = false;
  int calls = 0, restores = 0, failAt = -1, uncertainAt = -1, driftAt = -1;
  QVector<Operation> actions;
  QByteArray receipt;
  Snapshot read() override
  {
    return state;
  }
  bool mayModify() const override
  {
    return permitted;
  }
  QByteArray backupReceipt() const override
  {
    return receipt;
  }
  Effect perform(const Operation &op) override
  {
    ++calls;
    actions.append(op);
    if (calls == driftAt)
      state.services[original].definition = "concurrent-edit";
    if (calls == failAt)
      return Effect::FailedUnchanged;
    if (op.action == Action::Backup) {
      if (!emptyReceipt)
        receipt = "verified-fixture-backup";
      return Effect::Applied;
    }
    if (!omitEffects) {
      auto &job = state.services[op.label];
      switch (op.action) {
      case Action::Disable:
        job.disabled = true;
        break;
      case Action::Unload:
        job.loaded = false;
        job.running = false;
        break;
      case Action::Install:
        job.definition = op.definition;
        break;
      case Action::Enable:
        job.disabled = false;
        break;
      case Action::Load:
        job.loaded = true;
        job.running = !failToRun;
        job.failed = failToRun;
        break;
      default:
        break;
      }
    }
    return calls == uncertainAt ? Effect::Uncertain : Effect::Applied;
  }
  Effect restore(const Snapshot &before, const QByteArray &token) override
  {
    ++restores;
    if (restoreFails || token != receipt)
      return Effect::FailedUnchanged;
    for (const auto &label : {original, watchdog, candidate})
      state.services[label] = before.services[label];
    state.watchdogHash = before.watchdogHash;
    return Effect::Applied;
  }
};
} // namespace startup_fixture
