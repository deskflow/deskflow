// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "StartupRecovery.h"
#include <QFileDevice>
#include <QLockFile>
#include <functional>

namespace deskflow::gui::startup {
struct CommandResult
{
  enum State
  {
    Finished,
    NotStarted,
    Unknown
  } state = NotStarted;
  int exitCode = -1;
};
class CommandRunner
{
public:
  virtual ~CommandRunner() = default;
  virtual CommandResult run(const QString &program, const QStringList &arguments) = 0;
};
// Real launchctl adapter. Tests substitute CommandRunner; they never invoke it.
std::shared_ptr<CommandRunner> makeMacCommandRunner();
// This capability is compile-time gated, independently of UI confirmation.
bool nativeStartupChangesAvailable();
struct RecoveryFile
{
  QString path, name;
  QByteArray bytes;
  QFileDevice::Permissions permissions{};
  bool existed = false;
};
struct RecoveryRecord
{
  Plan plan;
  OperationHistory history;
  QByteArray receipt;
  QVector<RecoveryFile> files;
};
RecoveryRecord inspectRecoveryBackup(const Snapshot &current, const QString &directory);
OperationHistory latestStartupOperation(const Snapshot &current);
bool recordStartupHandoffFailure(const Snapshot &, const QString &reason);

// Operates only on the three reviewed user jobs. This is not a general command
// or file-writing API. The calling short-lived transaction process must have
// completed its explicitly confirmed GUI handoff before authorize() can pass.
class MacStartupExecutor final : public Executor
{
public:
  using HandoffCheck = std::function<bool()>;
  MacStartupExecutor(std::shared_ptr<Reader>, std::shared_ptr<CommandRunner>, HandoffCheck);
  Snapshot read() override;
  bool mayModify() const override;
  bool authorize(const Plan &, const QByteArray &, bool) override;
  bool complete() override;
  Effect perform(const Operation &) override;
  Effect restore(const Snapshot &, const QByteArray &) override;
  Result restorePersisted(const QString &directory, const QByteArray &confirmedDigest, bool handoffReady);
  QByteArray backupReceipt() const override;
  QString error() const
  {
    return m_error;
  }
  QString backupDirectory() const
  {
    return m_directory;
  }

private:
  bool same(const Snapshot &) const;
  bool saveJournal(const QString &status, const QString &step);
  Effect backup();
  Effect change(const Operation &);
  Effect uncertain(const QString &);
  bool verifyBackup() const;
  std::shared_ptr<Reader> m_reader;
  std::shared_ptr<CommandRunner> m_runner;
  HandoffCheck m_handoff;
  Plan m_plan;
  Snapshot m_expected;
  QVector<RecoveryFile> m_files;
  std::unique_ptr<QLockFile> m_lock;
  QByteArray m_receipt;
  QString m_directory, m_error;
  int m_next = 0;
  bool m_authorized = false, m_stopped = false, m_restored = false;
};
} // namespace deskflow::gui::startup
