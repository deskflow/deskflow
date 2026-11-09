// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QMap>
#include <QStringList>
#include <QVector>
#include <memory>

namespace deskflow::gui::startup {
inline const QString candidate = QStringLiteral("org.deskflow.candidate.gui");
inline const QString original = QStringLiteral("local.deskflow.gui");
inline const QString watchdog = QStringLiteral("local.deskflow.watchdog");
inline const QString keepawake = QStringLiteral("local.deskflow.keepawake");
struct Service
{
  QString label, path, error;
  QByteArray definition;
  bool known = false, loaded = false, running = false, disabled = false, failed = false;
  bool login = false, crashRecovery = false, alwaysAlive = false;
  int interval = 0;
  QStringList arguments;
};
struct OperationHistory
{
  QString directory, status, error;
  QByteArray restoreDigest;
  bool restorable = false;
};
struct Snapshot
{
  bool supported = false;
  QString home, domain, executable, error;
  QMap<QString, Service> services;
  QByteArray watchdogHash;
  // Bind a reviewed operation to this exact build, not just an app pathname.
  QByteArray executableHash, bundleInfoHash;
  QString version;
  OperationHistory history; // Display-only; never part of service revision.
  // Mutable run counts/PIDs and the untouched keepawake service are excluded.
  QByteArray revision() const;
};
struct Preferences
{
  bool paused = true, resume = false, preventSleep = false;
};
struct Options
{
  bool login = false, crashRecovery = false;
};
enum class Action
{
  Backup,
  Disable,
  Unload,
  Install,
  Enable,
  Load
};
struct Operation
{
  Action action;
  QString label;
  QByteArray definition;
  bool operator==(const Operation &) const = default;
};
struct Plan
{
  Snapshot before, after;
  Options options;
  QVector<Operation> operations;
  QString error;
  QByteArray approvalDigest;
  bool valid() const
  {
    return error.isEmpty() && !approvalDigest.isEmpty();
  }
  QStringList commands() const;
};
Plan prepare(const Snapshot &, Options);

class Reader
{
public:
  virtual ~Reader() = default;
  virtual Snapshot read() = 0;
};
// Production implementation only reads the four named user jobs. No mutation
// method or process enumeration is exposed, including in unsupported platforms.
std::shared_ptr<Reader> makeReadOnlyReader();

enum class Effect
{
  Applied,
  FailedUnchanged,
  Uncertain
};
class Executor : public Reader
{
public:
  virtual bool mayModify() const = 0;
  // Called only after canonical plan/state validation. Production adapters
  // additionally require the confirmed single-instance handoff to be complete.
  virtual bool authorize(const Plan &, const QByteArray &, bool)
  {
    return mayModify();
  }
  virtual bool complete()
  {
    return true;
  }
  virtual Effect perform(const Operation &) = 0;
  // Restore only candidate/original/watchdog files, loaded and disabled states.
  // The receipt must include exact file backups, including the old script.
  virtual Effect restore(const Snapshot &, const QByteArray &backupReceipt) = 0;
  virtual QByteArray backupReceipt() const = 0;
};
enum class Outcome
{
  NeedsConfirmation,
  Stale,
  Applied,
  FailedRolledBack,
  RecoveryRequired,
  RolledBack
};
struct Result
{
  Outcome outcome = Outcome::NeedsConfirmation;
  QString error;
  QByteArray receipt;
  QByteArray planDigest;
};
// A preview is not consent. Both controller and production adapter check the
// plan digest and current state; the adapter refuses unsequenced direct calls.
Result apply(const Plan &, const QByteArray &confirmedDigest, bool handoffReady, Executor &);
Result rollback(const Plan &, const Result &, const QByteArray &confirmedDigest, Executor &);
} // namespace deskflow::gui::startup
