// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "MacStartupExecutor.h"
#include "common/CandidatePolicy.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QUuid>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace deskflow::gui::startup {
namespace {
const auto privateFile = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
const auto privateDirectory = privateFile | QFileDevice::ExeOwner;
QByteArray sha(const QByteArray &b)
{
  return QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex();
}
bool plain(const QString &path)
{
  if (!QDir::isAbsolutePath(path) || QDir::cleanPath(path) != path || path.contains(QChar(0)))
    return false;
  for (auto p = path;;) {
    const QFileInfo f(p);
    if (f.isSymLink())
      return false;
    const auto parent = f.absolutePath();
    if (parent == p)
      break;
    p = parent;
  }
  return true;
}
bool owned(const QFileInfo &f)
{
#ifdef Q_OS_UNIX
  return !f.exists() || f.ownerId() == geteuid();
#else
  return false;
#endif
}
bool safeFile(const QString &path)
{
  const QFileInfo f(path);
  return plain(path) && owned(f) && (!f.exists() || (f.isFile() && f.size() <= 65536));
}
bool bytes(const QString &path, QByteArray &out)
{
  if (!safeFile(path))
    return false;
  if (!QFileInfo::exists(path)) {
    out.clear();
    return true;
  }
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return false;
  out = f.readAll();
  return f.error() == QFileDevice::NoError;
}
bool atomic(const QString &path, const QByteArray &data, QFileDevice::Permissions permissions)
{
  if (!safeFile(path))
    return false;
  QSaveFile f(path);
  f.setDirectWriteFallback(false);
  return f.open(QIODevice::WriteOnly) && f.setPermissions(permissions) && f.write(data) == data.size() && f.commit();
}
bool privateDir(const QString &path)
{
  if (!plain(path))
    return false;
  QFileInfo before(path);
  if (before.exists() && (!before.isDir() || !owned(before)))
    return false;
  if (!before.exists() && (!QDir().mkpath(path) || !QFile::setPermissions(path, privateDirectory)))
    return false;
  QFileInfo after(path);
  return plain(path) && owned(after) && after.isDir() &&
         !(after.permissions() &
           (QFileDevice::WriteGroup | QFileDevice::WriteOther | QFileDevice::ReadGroup | QFileDevice::ReadOther));
}
bool existingPrivateDir(const QString &path)
{
  const QFileInfo f(path);
  return plain(path) && f.isDir() && owned(f) &&
         !(f.permissions() &
           (QFileDevice::WriteGroup | QFileDevice::WriteOther | QFileDevice::ReadGroup | QFileDevice::ReadOther));
}
bool validHash(const QByteArray &s)
{
  if (s.size() != 64)
    return false;
  for (char c : s)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
      return false;
  return true;
}
void track(Snapshot &s, const Operation &op)
{
  auto &job = s.services[op.label];
  switch (op.action) {
  case Action::Disable:
    job.disabled = true;
    break;
  case Action::Enable:
    job.disabled = false;
    break;
  case Action::Unload:
    job.loaded = false;
    job.running = false;
    break;
  case Action::Load:
    job.loaded = true;
    job.running = true;
    break;
  case Action::Install:
    job.definition = op.definition;
    break;
  default:
    break;
  }
}
class MacCommands final : public CommandRunner
{
public:
  CommandResult run(const QString &program, const QStringList &args) override
  {
#if defined(Q_OS_MACOS) && defined(DESKFLOW_CANDIDATE_BUILD) && defined(DESKFLOW_STARTUP_MUTATIONS) &&                 \
    !defined(DESKFLOW_LAB_BUILD)
    if (program != "/bin/launchctl" || args.isEmpty() ||
        !QStringList{"disable", "enable", "bootout", "bootstrap"}.contains(args.front()))
      return {};
    QProcess process;
    process.start(program, args, QIODevice::ReadOnly);
    if (!process.waitForStarted(1000))
      return {process.error() == QProcess::FailedToStart ? CommandResult::NotStarted : CommandResult::Unknown, -1};
    if (!process.waitForFinished(5000)) {
      // Stopping our command process does NOT imply that launchd undid it.
      process.kill();
      process.waitForFinished(200);
      return {CommandResult::Unknown, -1};
    }
    if (process.exitStatus() != QProcess::NormalExit)
      return {CommandResult::Unknown, -1};
    return {CommandResult::Finished, process.exitCode()};
#else
    Q_UNUSED(program);
    Q_UNUSED(args);
    return {};
#endif
  }
};
} // namespace
RecoveryRecord inspectRecoveryBackup(const Snapshot &current, const QString &directory)
{
  RecoveryRecord record;
  record.history.directory = directory;
  auto reject = [&](const QString &reason) {
    record.history.restorable = false;
    record.history.error = reason;
    return record;
  };
  const auto root = current.home + "/Library/Application Support/DeskflowCandidate/startup-backups";
  if (!current.supported || !current.error.isEmpty() || QFileInfo(directory).absolutePath() != root ||
      QUuid(QFileInfo(directory).fileName()).isNull() || !existingPrivateDir(root) || !existingPrivateDir(directory))
    return reject("invalid-backup-directory");
  QByteArray data;
  if (!bytes(directory + "/journal.json", data))
    return reject("journal-unreadable");
  const auto journal = QJsonDocument::fromJson(data).object();
  record.history.status = journal["status"].toString();
  if (record.history.status != "applied") {
    if (!QStringList{"restored", "backup-failed"}.contains(record.history.status))
      record.history.error = "operation-needs-review";
    return record;
  }
  record.receipt = journal["receipt"].toString().toLatin1();
  QByteArray manifest;
  if (!validHash(record.receipt) || !bytes(directory + "/manifest.json", manifest) || sha(manifest) != record.receipt)
    return reject("backup-manifest-mismatch");
  const auto saved = QJsonDocument::fromJson(manifest).object();
  if (saved["schema"] != 2 || saved["home"].toString() != current.home ||
      saved["domain"].toString() != current.domain || saved["executable"].toString() != current.executable ||
      saved["version"].toString() != current.version ||
      saved["executableHash"].toString().toLatin1() != current.executableHash ||
      saved["bundleInfoHash"].toString().toLatin1() != current.bundleInfoHash)
    return reject("backup-build-or-user-mismatch");
  Snapshot before = current;
  const auto files = saved["files"].toArray();
  const QStringList labels{candidate, original, watchdog};
  if (files.size() != 4)
    return reject("backup-file-list-mismatch");
  for (int i = 0; i < 4; ++i) {
    const auto item = files[i].toObject();
    RecoveryFile f;
    f.name = i < 3 ? labels[i] + ".plist" : "watchdog.zsh";
    f.path = i < 3 ? current.home + "/Library/LaunchAgents/" + f.name
                   : current.home + "/Library/Application Support/Deskflow/watchdog.zsh";
    if (item["name"].toString() != f.name || item["path"].toString() != f.path || !item["present"].isBool() ||
        !item["permissions"].isDouble())
      return reject("backup-file-path-mismatch");
    f.existed = item["present"].toBool();
    f.permissions = QFileDevice::Permissions(item["permissions"].toInt());
    if (!bytes(directory + '/' + f.name, f.bytes) || sha(f.bytes) != item["sha256"].toString().toLatin1() ||
        (!f.existed && !f.bytes.isEmpty()))
      return reject("backup-file-hash-mismatch");
    if (i < 3)
      before.services[labels[i]].definition = f.bytes;
    else
      before.watchdogHash = sha(f.bytes);
    record.files.append(f);
  }
  const auto candidateBytes = record.files[0].bytes;
  if (!candidateBytes.isEmpty() && candidateBytes != deskflow::candidate::launchAgent(current.executable, true) &&
      candidateBytes != deskflow::candidate::launchAgent(current.executable, false))
    return reject("unrecognized-candidate-backup");
  const auto states = saved["states"].toArray();
  if (states.size() != 3)
    return reject("backup-state-list-mismatch");
  for (int i = 0; i < 3; ++i) {
    const auto state = states[i].toObject();
    if (state["label"].toString() != labels[i] || !state["loaded"].isBool() || !state["disabled"].isBool() ||
        !state["running"].isBool())
      return reject("backup-state-mismatch");
    auto &job = before.services[labels[i]];
    job.loaded = state["loaded"].toBool();
    job.disabled = state["disabled"].toBool();
    job.running = state["running"].toBool();
    job.failed = false;
  }
  if (!saved["login"].isBool() || !saved["recovery"].isBool())
    return reject("backup-options-mismatch");
  record.plan = prepare(before, {saved["login"].toBool(), saved["recovery"].toBool()});
  if (!record.plan.valid() || before.revision() != saved["before"].toString().toLatin1() ||
      record.plan.approvalDigest != saved["plan"].toString().toLatin1() ||
      journal["plan"].toString().toLatin1() != record.plan.approvalDigest ||
      record.plan.after.revision() != current.revision() ||
      journal["expected"].toString().toLatin1() != current.revision())
    return reject("backup-state-changed");
  for (int i = 1; i < 4; ++i) {
    QByteArray actual;
    if (!bytes(record.files[i].path, actual) || actual != record.files[i].bytes ||
        QFileInfo::exists(record.files[i].path) != record.files[i].existed)
      return reject("original-files-changed");
  }
  record.history.restoreDigest =
      sha("restore\n" + record.receipt + '\n' + current.revision() + '\n' + record.plan.approvalDigest);
  record.history.restorable = true;
  return record;
}
OperationHistory latestStartupOperation(const Snapshot &current)
{
  const auto root = current.home + "/Library/Application Support/DeskflowCandidate/startup-backups";
  if (!QFileInfo::exists(root))
    return {};
  if (!existingPrivateDir(root))
    return {{}, {}, "history-unavailable", {}, false};
  const auto entries = QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
  if (entries.size() > 256)
    return {{}, {}, "history-limit-reached", {}, false};
  const auto handoff = root + "/handoff-result.json";
  if (QFileInfo::exists(handoff) &&
      (entries.isEmpty() || QFileInfo(handoff).lastModified() >
                                QFileInfo(entries.front().absoluteFilePath() + "/journal.json").lastModified())) {
    QByteArray data;
    if (!bytes(handoff, data))
      return {{}, {}, "history-unavailable", {}, false};
    const auto record = QJsonDocument::fromJson(data).object();
    if (record["status"] != "handoff-failed" || record["version"].toString() != current.version)
      return {{}, {}, "history-unavailable", {}, false};
    return {handoff, "handoff-failed", record["reason"].toString(), {}, false};
  }
  if (entries.isEmpty())
    return {};
  return inspectRecoveryBackup(current, entries.front().absoluteFilePath()).history;
}
bool recordStartupHandoffFailure(const Snapshot &s, const QString &reason)
{
  if (!s.supported || !plain(s.home) || s.version.isEmpty())
    return false;
  const auto root = s.home + "/Library/Application Support/DeskflowCandidate/startup-backups";
  if (!privateDir(root))
    return false;
  return atomic(
      root + "/handoff-result.json",
      QJsonDocument(
          QJsonObject{{"schema", 1}, {"status", "handoff-failed"}, {"reason", reason}, {"version", s.version}}
      ).toJson(),
      privateFile
  );
}
bool nativeStartupChangesAvailable()
{
#if defined(Q_OS_MACOS) && defined(DESKFLOW_CANDIDATE_BUILD) && defined(DESKFLOW_STARTUP_MUTATIONS) &&                 \
    !defined(DESKFLOW_LAB_BUILD)
  return true;
#else
  return false;
#endif
}
std::shared_ptr<CommandRunner> makeMacCommandRunner()
{
  return std::make_shared<MacCommands>();
}
MacStartupExecutor::MacStartupExecutor(std::shared_ptr<Reader> r, std::shared_ptr<CommandRunner> c, HandoffCheck h)
    : m_reader(std::move(r)),
      m_runner(std::move(c)),
      m_handoff(std::move(h))
{
}
Snapshot MacStartupExecutor::read()
{
  return m_reader->read();
}
bool MacStartupExecutor::mayModify() const
{
  return bool(m_reader) && bool(m_runner) && bool(m_handoff) && m_handoff() && !m_stopped;
}
bool MacStartupExecutor::same(const Snapshot &s) const
{
  if (!s.supported || !s.error.isEmpty() || s.revision() != m_expected.revision())
    return false;
  for (const auto &label : {candidate, original, watchdog})
    if (!s.services[label].known || !s.services[label].error.isEmpty())
      return false;
  return true;
}
bool MacStartupExecutor::authorize(const Plan &p, const QByteArray &digest, bool ready)
{
  if (m_authorized || !ready || !mayModify() || !p.valid() || digest != p.approvalDigest)
    return false;
  const auto canonical = prepare(p.before, p.options);
  if (canonical.operations != p.operations || canonical.approvalDigest != digest ||
      canonical.after.revision() != p.after.revision() || !validHash(p.before.executableHash) ||
      !validHash(p.before.bundleInfoHash) || p.before.version.isEmpty())
    return false;
  m_expected = p.before;
  if (!same(read()))
    return false;
  const auto previous = p.before.services[candidate].definition;
  if (!previous.isEmpty() && previous != deskflow::candidate::launchAgent(p.before.executable, true) &&
      previous != deskflow::candidate::launchAgent(p.before.executable, false)) {
    m_error = "unrecognized-candidate-backup";
    return false;
  }
  m_plan = p;
  m_authorized = true;
  return true;
}
QByteArray MacStartupExecutor::backupReceipt() const
{
  return m_receipt;
}
bool MacStartupExecutor::saveJournal(const QString &status, const QString &step)
{
  const QJsonObject journal{
      {"schema", 1},
      {"status", status},
      {"step", step},
      {"plan", QString::fromLatin1(m_plan.approvalDigest)},
      {"receipt", QString::fromLatin1(m_receipt)},
      {"expected", QString::fromLatin1(m_expected.revision())},
      {"version", m_plan.before.version},
      {"executable", m_plan.before.executable},
      {"executableHash", QString::fromLatin1(m_plan.before.executableHash)}
  };
  return atomic(m_directory + "/journal.json", QJsonDocument(journal).toJson(), privateFile);
}
Effect MacStartupExecutor::uncertain(const QString &error)
{
  m_error = error;
  m_stopped = true;
  if (!m_directory.isEmpty())
    saveJournal("uncertain", error);
  return Effect::Uncertain;
}
Effect MacStartupExecutor::backup()
{
  const auto &s = m_plan.before;
  const auto root = s.home + "/Library/Application Support/DeskflowCandidate/startup-backups";
  if (!plain(s.home) || !privateDir(root))
    return Effect::FailedUnchanged;
  m_lock = std::make_unique<QLockFile>(root + "/operation.lock");
  m_lock->setStaleLockTime(0);
  if (!m_lock->tryLock(0)) {
    m_error = "operation-locked";
    return Effect::FailedUnchanged;
  }
  // An interrupted or ambiguous earlier operation is not silently retried.
  for (const auto &name : QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QByteArray data;
    if (!bytes(root + '/' + name + "/journal.json", data))
      return Effect::FailedUnchanged;
    const auto state = QJsonDocument::fromJson(data).object().value("status").toString();
    if (!QStringList{"applied", "restored", "backup-failed"}.contains(state)) {
      m_error = "unresolved-previous-operation";
      return Effect::FailedUnchanged;
    }
  }
  m_directory = root + '/' + QUuid::createUuid().toString(QUuid::WithoutBraces);
  if (!privateDir(m_directory) || !saveJournal("preparing", "backup"))
    return Effect::FailedUnchanged;
  for (const auto &label : {candidate, original, watchdog}) {
    RecoveryFile f;
    f.name = label + ".plist";
    f.path = s.services[label].path;
    f.existed = QFileInfo::exists(f.path);
    f.permissions = QFileInfo(f.path).permissions();
    if (!bytes(f.path, f.bytes) || f.bytes != s.services[label].definition) {
      saveJournal("backup-failed", "file-changed");
      return Effect::FailedUnchanged;
    }
    m_files.append(f);
  }
  RecoveryFile script;
  script.name = "watchdog.zsh";
  script.path = s.home + "/Library/Application Support/Deskflow/watchdog.zsh";
  script.existed = QFileInfo::exists(script.path);
  script.permissions = QFileInfo(script.path).permissions();
  if (!bytes(script.path, script.bytes) || sha(script.bytes) != s.watchdogHash) {
    saveJournal("backup-failed", "script-changed");
    return Effect::FailedUnchanged;
  }
  m_files.append(script);
  QJsonArray files;
  for (const auto &f : m_files) {
    if (!atomic(m_directory + '/' + f.name, f.bytes, privateFile)) {
      saveJournal("backup-failed", "write-failed");
      return Effect::FailedUnchanged;
    }
    files.append(
        QJsonObject{
            {"path", f.path},
            {"name", f.name},
            {"present", f.existed},
            {"permissions", int(f.permissions)},
            {"sha256", QString::fromLatin1(sha(f.bytes))}
        }
    );
  }
  QJsonArray states;
  for (const auto &label : {candidate, original, watchdog}) {
    const auto job = s.services[label];
    states.append(
        QJsonObject{{"label", label}, {"loaded", job.loaded}, {"disabled", job.disabled}, {"running", job.running}}
    );
  }
  const auto manifest = QJsonDocument(
                            QJsonObject{
                                {"schema", 2},
                                {"plan", QString::fromLatin1(m_plan.approvalDigest)},
                                {"home", s.home},
                                {"domain", s.domain},
                                {"executable", s.executable},
                                {"version", s.version},
                                {"executableHash", QString::fromLatin1(s.executableHash)},
                                {"bundleInfoHash", QString::fromLatin1(s.bundleInfoHash)},
                                {"login", m_plan.options.login},
                                {"recovery", m_plan.options.crashRecovery},
                                {"before", QString::fromLatin1(s.revision())},
                                {"files", files},
                                {"states", states}
                            }
  ).toJson();
  if (!atomic(m_directory + "/manifest.json", manifest, privateFile)) {
    saveJournal("backup-failed", "manifest-failed");
    return Effect::FailedUnchanged;
  }
  m_receipt = sha(manifest);
  if (!verifyBackup() || !same(read()) || !saveJournal("ready", "backup-verified"))
    return uncertain("backup-verification-failed");
  return Effect::Applied;
}
bool MacStartupExecutor::verifyBackup() const
{
  QByteArray manifest;
  if (m_receipt.isEmpty() || !bytes(m_directory + "/manifest.json", manifest) || sha(manifest) != m_receipt)
    return false;
  for (const auto &f : m_files) {
    QByteArray b;
    if (!bytes(m_directory + '/' + f.name, b) || b != f.bytes)
      return false;
  }
  return true;
}
Effect MacStartupExecutor::change(const Operation &op)
{
  if (!same(read()) || !verifyBackup())
    return uncertain("precondition-changed");
  const auto before = m_expected;
  auto after = before;
  track(after, op);
  if (!saveJournal("inflight", QString::number(int(op.action)) + ':' + op.label))
    return uncertain("journal-write-failed");
  CommandResult result;
  if (op.action == Action::Install) {
    const auto path = m_expected.services[candidate].path;
    QByteArray current;
    if (op.label != candidate || !bytes(path, current) || current != m_expected.services[candidate].definition)
      return uncertain("candidate-file-changed");
    const auto directory = QFileInfo(path).absolutePath();
    if (!plain(directory) || (!QFileInfo::exists(directory) && !privateDir(directory)))
      return uncertain("candidate-directory-unavailable");
    const bool wrote = atomic(path, op.definition, privateFile);
    result = {CommandResult::Finished, wrote ? 0 : 1};
  } else {
    QStringList args;
    const auto target = before.domain + '/' + op.label;
    switch (op.action) {
    case Action::Disable:
      args = {"disable", target};
      break;
    case Action::Enable:
      args = {"enable", target};
      break;
    case Action::Unload:
      args = {"bootout", target};
      break;
    case Action::Load:
      args = {"bootstrap", before.domain, before.services[op.label].path};
      break;
    default:
      return uncertain("invalid-action");
    }
    result = m_runner->run("/bin/launchctl", args);
  }
  const auto actual = read();
  if (result.state == CommandResult::Unknown)
    return uncertain("command-outcome-unknown");
  if (result.state == CommandResult::Finished && result.exitCode == 0 && actual.revision() == after.revision()) {
    m_expected = after;
    if (!same(actual) || !saveJournal("ready", "verified"))
      return uncertain("readback-failed");
    return Effect::Applied;
  }
  if (same(actual)) {
    if (!saveJournal("ready", "failed-unchanged"))
      return uncertain("journal-write-failed");
    m_error = "command-failed-unchanged";
    return Effect::FailedUnchanged;
  }
  return uncertain("unexpected-postcondition");
}
Effect MacStartupExecutor::perform(const Operation &op)
{
  if (!m_authorized || !mayModify() || m_next >= m_plan.operations.size() || op != m_plan.operations[m_next])
    return Effect::FailedUnchanged;
  if (!same(read()))
    return uncertain("state-changed");
  const auto effect = op.action == Action::Backup ? backup() : change(op);
  if (effect == Effect::Applied) {
    ++m_next;
    if (m_next == m_plan.operations.size() && !saveJournal("ready", "awaiting-final-verification"))
      return uncertain("final-journal-failed");
  }
  return effect;
}
bool MacStartupExecutor::complete()
{
  if (!m_authorized || !mayModify() || m_next != m_plan.operations.size() || !verifyBackup())
    return false;
  const auto actual = read();
  if (!same(actual))
    return false;
  for (const auto &label : {original, candidate})
    if (m_plan.after.services[label].running && (!actual.services[label].running || actual.services[label].failed))
      return false;
  return saveJournal("applied", "complete");
}
Result MacStartupExecutor::restorePersisted(const QString &directory, const QByteArray &confirmed, bool ready)
{
  if (m_authorized || !ready || !mayModify())
    return {};
  const auto record = inspectRecoveryBackup(read(), directory);
  if (!record.history.restorable)
    return {Outcome::Stale, record.history.error, {}};
  if (confirmed != record.history.restoreDigest)
    return {};
  const auto root = QFileInfo(directory).absolutePath();
  m_lock = std::make_unique<QLockFile>(root + "/operation.lock");
  m_lock->setStaleLockTime(0);
  if (!m_lock->tryLock(0))
    return {Outcome::Stale, "operation-locked", {}};
  // Re-read under the same lock used by new changes. A receipt never grants
  // permission to replay a different build, path, plan or an uncertain journal.
  const auto locked = inspectRecoveryBackup(read(), directory);
  if (!locked.history.restorable || locked.history.restoreDigest != confirmed)
    return {Outcome::Stale, "backup-changed", {}};
  m_plan = locked.plan;
  m_expected = m_plan.after;
  m_files = locked.files;
  m_receipt = locked.receipt;
  m_directory = directory;
  m_authorized = true;
  m_next = m_plan.operations.size();
  const auto effect = restore(m_plan.before, m_receipt);
  const auto actual = read();
  bool running = true;
  for (const auto &label : {original, candidate})
    if (m_plan.before.services[label].running && (!actual.services[label].running || actual.services[label].failed))
      running = false;
  if (effect == Effect::Applied && same(actual) && running)
    return {Outcome::RolledBack, {}, m_receipt};
  uncertain("persisted-rollback-not-verified");
  return {Outcome::RecoveryRequired, m_error, m_receipt};
}
Effect MacStartupExecutor::restore(const Snapshot &before, const QByteArray &receipt)
{
  if (!m_authorized || !mayModify() || m_restored || receipt != m_receipt ||
      before.revision() != m_plan.before.revision() || !verifyBackup() || !same(read()))
    return Effect::FailedUnchanged;
  // No state/credential/config directories are copied. Only a known Candidate
  // plist is rewritten; original plists and watchdog script must remain exact.
  for (const auto &f : m_files) {
    if (f.path == before.services[candidate].path)
      continue;
    QByteArray current;
    if (!bytes(f.path, current) || current != f.bytes || QFileInfo::exists(f.path) != f.existed)
      return uncertain("original-files-changed-before-restore");
  }
  auto run = [&](Action action, const QString &label) { return change({action, label, {}}) == Effect::Applied; };
  // Stop the replacement before any old input service can be restarted.
  for (const auto &label : {candidate, watchdog, original}) {
    const auto current = m_expected.services[label], wanted = before.services[label];
    if (current.loaded && (!wanted.loaded || label == candidate)) {
      if (!current.disabled && !run(Action::Disable, label))
        return uncertain("restore-disable-failed");
      if (!run(Action::Unload, label))
        return uncertain("restore-unload-failed");
    }
  }
  const auto candidateCopy = m_files.front();
  const auto path = candidateCopy.path;
  if (m_expected.services[candidate].definition != candidateCopy.bytes ||
      QFileInfo::exists(path) != candidateCopy.existed) {
    if (!saveJournal("inflight", "restore-candidate-file"))
      return uncertain("restore-journal-failed");
    QByteArray current;
    if (!bytes(path, current) || current != m_expected.services[candidate].definition)
      return uncertain("restore-file-changed");
    const bool ok =
        candidateCopy.existed ? atomic(path, candidateCopy.bytes, candidateCopy.permissions) : QFile::remove(path);
    if (!ok)
      return uncertain("restore-file-failed");
    m_expected.services[candidate].definition = candidateCopy.bytes;
    if (!same(read()) || !saveJournal("ready", "restore-file-verified"))
      return uncertain("restore-file-readback-failed");
  }
  // GUI first, watchdog last. A previously loaded-but-disabled job may need a
  // temporary enable to bootstrap, followed by restoration of its disabled bit.
  for (const auto &label : {original, candidate, watchdog}) {
    const auto wanted = before.services[label];
    if (wanted.loaded && !m_expected.services[label].loaded) {
      if (m_expected.services[label].disabled && !run(Action::Enable, label))
        return uncertain("restore-enable-failed");
      if (!run(Action::Load, label))
        return uncertain("restore-load-failed");
    }
    if (m_expected.services[label].disabled != wanted.disabled &&
        !run(wanted.disabled ? Action::Disable : Action::Enable, label))
      return uncertain("restore-disabled-state-failed");
  }
  if (!same(read()) || m_expected.revision() != before.revision() || !saveJournal("restored", "complete"))
    return uncertain("restore-not-verified");
  m_restored = true;
  return Effect::Applied;
}
} // namespace deskflow::gui::startup
