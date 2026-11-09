// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "StartupRecovery.h"
#include "common/CandidatePolicy.h"
#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace deskflow::gui::startup {
namespace {
QByteArray hash(const QByteArray &value)
{
  return QCryptographicHash::hash(value, QCryptographicHash::Sha256).toHex();
}
QStringList managed()
{
  return {watchdog, original, candidate};
}
bool observed(const Snapshot &s)
{
  for (const auto &label : managed())
    if (!s.services.contains(label) || !s.services[label].known || !s.services[label].error.isEmpty())
      return false;
  return true;
}
bool matches(const Snapshot &actual, const Snapshot &expected, bool verifyRunning = false)
{
  if (!actual.supported || !actual.error.isEmpty() || !observed(actual) || actual.revision() != expected.revision())
    return false;
  for (const auto &label : {original, candidate}) {
    const auto &want = expected.services[label], &got = actual.services[label];
    if (verifyRunning && want.running && (!got.running || got.failed))
      return false;
  }
  return true;
}
void recordEffect(Snapshot &s, const Operation &op)
{
  if (op.action == Action::Backup)
    return;
  auto &job = s.services[op.label];
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
    job.running = true;
    break;
  default:
    break;
  }
}
} // namespace
QByteArray Snapshot::revision() const
{
  QJsonArray jobs;
  for (const auto &label : managed()) {
    const auto s = services.value(label);
    jobs.append(
        QJsonObject{
            {"label", label},
            {"path", s.path},
            {"known", s.known},
            {"loaded", s.loaded},
            {"disabled", s.disabled},
            {"file", QString::fromLatin1(hash(s.definition))},
            {"error", s.error}
        }
    );
  }
  return hash(QJsonDocument(
                  QJsonObject{
                      {"home", home},
                      {"domain", domain},
                      {"executable", executable},
                      {"watchdog", QString::fromLatin1(watchdogHash)},
                      {"jobs", jobs},
                      {"executableHash", QString::fromLatin1(executableHash)},
                      {"bundleInfoHash", QString::fromLatin1(bundleInfoHash)},
                      {"version", version}
                  }
  ).toJson(QJsonDocument::Compact));
}
Plan prepare(const Snapshot &snapshot, Options options)
{
  Plan p;
  p.before = snapshot;
  p.after = snapshot;
  p.options = options;
  auto reject = [&](const QString &message) {
    p.error = message;
    return p;
  };
  if (!snapshot.supported)
    return reject(QStringLiteral("unsupported-platform"));
  if (!snapshot.error.isEmpty() || !observed(snapshot))
    return reject(QStringLiteral("status-unavailable"));
  bool ok = false;
  const auto uid = snapshot.domain.mid(4).toUInt(&ok);
  if (!snapshot.domain.startsWith("gui/") || !ok || snapshot.domain != QString("gui/%1").arg(uid) ||
      !QDir::isAbsolutePath(snapshot.home) || snapshot.home != QDir::cleanPath(snapshot.home))
    return reject(QStringLiteral("invalid-user-domain"));
  if (options.crashRecovery && !options.login)
    return reject(QStringLiteral("recovery-requires-login"));
  for (const auto &label : managed())
    if (snapshot.services[label].path != snapshot.home + "/Library/LaunchAgents/" + label + ".plist")
      return reject(QStringLiteral("unexpected-service-path"));
  const auto definition = deskflow::candidate::launchAgent(snapshot.executable, options.crashRecovery);
  if (definition.isEmpty())
    return reject(QStringLiteral("unexpected-candidate-path"));
  // Back up before retiring either original job; keepawake is never managed.
  p.operations.append({Action::Backup, {}, {}});
  auto retire = [&](const QString &label) {
    auto &s = p.after.services[label];
    if (!s.definition.isEmpty() || s.loaded) {
      if (!s.disabled)
        p.operations.append({Action::Disable, label, {}});
      if (s.loaded)
        p.operations.append({Action::Unload, label, {}});
      s.disabled = true;
      s.loaded = false;
      s.running = false;
    }
  };
  // Disabling Candidate alone must not also retire an unrelated original job.
  if (options.login) {
    retire(watchdog);
    retire(original);
  }
  retire(candidate);
  auto &c = p.after.services[candidate];
  if (options.login) {
    p.operations.append({Action::Install, candidate, definition});
    p.operations.append({Action::Enable, candidate, {}});
    p.operations.append({Action::Load, candidate, {}});
    c.definition = definition;
    c.disabled = false;
    c.loaded = true;
    c.running = true;
    c.failed = false;
    c.error.clear();
    c.login = true;
    c.crashRecovery = options.crashRecovery;
    c.alwaysAlive = false;
  }
  if (p.after.revision() == p.before.revision())
    return reject(QStringLiteral("no-change"));
  p.approvalDigest = hash(p.before.revision() + p.after.revision());
  return p;
}
QStringList Plan::commands() const
{
  QStringList out;
  for (const auto &step : operations) {
    const auto target = before.domain + '/' + step.label;
    switch (step.action) {
    case Action::Backup:
      out << QStringLiteral(
          "Backup exact original/Candidate plists and watchdog script; retain loaded/disabled states."
      );
      break;
    case Action::Disable:
      out << "/bin/launchctl disable " + target;
      break;
    case Action::Unload:
      out << "/bin/launchctl bootout " + target;
      break;
    case Action::Install:
      out << "Atomic write: " + before.services[candidate].path;
      break;
    case Action::Enable:
      out << "/bin/launchctl enable " + target;
      break;
    case Action::Load:
      out << "/bin/launchctl bootstrap " + before.domain + ' ' + before.services[candidate].path;
      break;
    }
  }
  return out;
}
Result apply(const Plan &plan, const QByteArray &confirmed, bool ready, Executor &executor)
{
  if (!plan.valid() || confirmed != plan.approvalDigest || !ready || !executor.mayModify())
    return {};
  const auto canonical = prepare(plan.before, plan.options);
  if (!canonical.valid() || canonical.approvalDigest != plan.approvalDigest ||
      canonical.operations != plan.operations || canonical.after.revision() != plan.after.revision())
    return {Outcome::Stale, QStringLiteral("plan-changed"), {}};
  if (!matches(executor.read(), plan.before))
    return {Outcome::Stale, QStringLiteral("state-changed"), {}};
  if (!executor.authorize(plan, confirmed, ready))
    return {};
  QByteArray receipt;
  Snapshot progress = plan.before;
  for (const auto &step : plan.operations) {
    if (!matches(executor.read(), progress))
      return {Outcome::RecoveryRequired, QStringLiteral("state-changed-during-operation"), receipt};
    const auto effect = executor.perform(step);
    if (step.action == Action::Backup && effect == Effect::Applied) {
      receipt = executor.backupReceipt();
      if (receipt.isEmpty())
        return {Outcome::RecoveryRequired, QStringLiteral("backup-not-verified"), {}};
    }
    if (effect == Effect::Uncertain)
      return {Outcome::RecoveryRequired, QStringLiteral("uncertain-operation"), receipt};
    if (effect == Effect::FailedUnchanged) {
      if (receipt.isEmpty())
        return {Outcome::FailedRolledBack, QStringLiteral("backup-failed-no-change"), {}};
      if (!matches(executor.read(), progress))
        return {Outcome::RecoveryRequired, QStringLiteral("state-changed-before-rollback"), receipt};
      const auto restored = executor.restore(plan.before, receipt);
      return restored == Effect::Applied && matches(executor.read(), plan.before, true)
                 ? Result{Outcome::FailedRolledBack, QStringLiteral("operation-failed-restored"), receipt}
                 : Result{Outcome::RecoveryRequired, QStringLiteral("rollback-not-verified"), receipt};
    }
    recordEffect(progress, step);
  }
  const auto actual = executor.read();
  if (!matches(actual, plan.after, true)) {
    // A known failed launch can restore the original. Unknown state or an
    // intervening edit must not be overwritten by an automatic rollback.
    if (matches(actual, progress)) {
      const auto restored = executor.restore(plan.before, receipt);
      if (restored == Effect::Applied && matches(executor.read(), plan.before, true))
        return {Outcome::FailedRolledBack, QStringLiteral("launch-failed-restored"), receipt};
    }
    return {Outcome::RecoveryRequired, QStringLiteral("postcondition-not-verified"), receipt};
  }
  if (!executor.complete())
    return {Outcome::RecoveryRequired, QStringLiteral("completion-not-recorded"), receipt};
  return {Outcome::Applied, {}, receipt, plan.approvalDigest};
}
Result rollback(const Plan &plan, const Result &applied, const QByteArray &confirmed, Executor &executor)
{
  if (!plan.valid() || applied.outcome != Outcome::Applied || applied.receipt.isEmpty() ||
      applied.planDigest != plan.approvalDigest || confirmed != plan.approvalDigest || !executor.mayModify())
    return {};
  const auto canonical = prepare(plan.before, plan.options);
  if (!canonical.valid() || canonical.approvalDigest != plan.approvalDigest ||
      canonical.operations != plan.operations || canonical.after.revision() != plan.after.revision())
    return {Outcome::Stale, QStringLiteral("plan-changed"), applied.receipt};
  if (!matches(executor.read(), plan.after))
    return {Outcome::Stale, QStringLiteral("state-changed"), applied.receipt};
  const auto effect = executor.restore(plan.before, applied.receipt);
  return effect == Effect::Applied && matches(executor.read(), plan.before, true)
             ? Result{Outcome::RolledBack, {}, applied.receipt}
             : Result{Outcome::RecoveryRequired, QStringLiteral("rollback-not-verified"), applied.receipt};
}
} // namespace deskflow::gui::startup
