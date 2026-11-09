// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "StartupFixture.h"
#include "gui/core/MacStartupExecutor.h"
#include "gui/core/StartupHandoff.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>
#include <QtTest>
using namespace deskflow::gui::startup;
namespace {
QByteArray hash(const QByteArray &b)
{
  return QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex();
}
bool write(const QString &path, const QByteArray &data)
{
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  return f.open(QIODevice::WriteOnly) && f.write(data) == data.size();
}
QByteArray readBytes(const QString &path)
{
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return {};
  return f.readAll();
}
class DiskReader : public Reader
{
public:
  Snapshot state;
  QString shared;
  void saveState()
  {
    if (shared.isEmpty())
      return;
    QJsonArray jobs;
    for (const auto &label : {candidate, original, watchdog, keepawake}) {
      const auto s = state.services[label];
      jobs.append(
          QJsonObject{
              {"label", label},
              {"loaded", s.loaded},
              {"running", s.running},
              {"disabled", s.disabled},
              {"failed", s.failed}
          }
      );
    }
    if (!write(shared, QJsonDocument(QJsonObject{{"jobs", jobs}, {"version", state.version}}).toJson()))
      qFatal("Fixture state write failed");
  }
  Snapshot read() override
  {
    if (!shared.isEmpty()) {
      const auto value = QJsonDocument::fromJson(readBytes(shared)).object();
      state.version = value["version"].toString();
      for (const auto &entry : value["jobs"].toArray()) {
        const auto o = entry.toObject();
        auto &j = state.services[o["label"].toString()];
        j.loaded = o["loaded"].toBool();
        j.running = o["running"].toBool();
        j.disabled = o["disabled"].toBool();
        j.failed = o["failed"].toBool();
      }
    }
    auto s = state;
    for (auto i = s.services.begin(); i != s.services.end(); ++i)
      i->definition = readBytes(i->path);
    s.watchdogHash = hash(readBytes(s.home + "/Library/Application Support/Deskflow/watchdog.zsh"));
    s.executableHash = hash(readBytes(s.executable));
    s.bundleInfoHash = hash(readBytes(QFileInfo(s.executable).absolutePath() + "/../Info.plist"));
    return s;
  }
};
class FakeCommands : public CommandRunner
{
public:
  std::shared_ptr<DiskReader> reader;
  QStringList calls;
  int fail = -1, unknown = -1, failLaunch = -1;
  bool changeThenFail = false, externalEdit = false;
  CommandResult run(const QString &program, const QStringList &args) override
  {
    if (program != "/bin/launchctl")
      qFatal("Unexpected command");
    calls << args.join(' ');
    const int n = calls.size();
    if (!reader->shared.isEmpty()) {
      QFile log(reader->state.home + "/fixture-commands.log");
      if (!log.open(QIODevice::Append))
        qFatal("Fixture log failed");
      log.write(args.join(' ').toUtf8() + '\n');
    }
    if (n == fail && !changeThenFail)
      return {CommandResult::Finished, 1};
    QString label;
    if (args[0] == "bootstrap")
      label = QFileInfo(args[2]).completeBaseName();
    else
      label = args[1].section('/', 2);
    if (!QStringList{candidate, original, watchdog}.contains(label))
      qFatal("Unexpected service");
    auto &s = reader->state.services[label];
    if (args[0] == "disable")
      s.disabled = true;
    else if (args[0] == "enable")
      s.disabled = false;
    else if (args[0] == "bootout") {
      s.loaded = false;
      s.running = false;
    } else if (args[0] == "bootstrap") {
      s.loaded = true;
      s.running = n != failLaunch;
      s.failed = n == failLaunch;
    } else
      qFatal("Unexpected action");
    if (externalEdit)
      write(reader->state.services[original].path, "external edit");
    reader->saveState();
    if (n == unknown)
      return {CommandResult::Unknown, -1};
    return {CommandResult::Finished, n == fail ? 1 : 0};
  }
};
struct Fixture
{
  QTemporaryDir temp;
  std::shared_ptr<DiskReader> reader = std::make_shared<DiskReader>();
  std::shared_ptr<FakeCommands> commands = std::make_shared<FakeCommands>();
  bool handoff = true;
  Fixture() : temp(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/executor-XXXXXX")
  {
    if (qEnvironmentVariable("DESKFLOW_TEST_ROOT").isEmpty() || !temp.isValid())
      qFatal("Explicit test root required");
    auto &s = reader->state;
    s = startup_fixture::snapshot();
    s.home = QDir(temp.path()).canonicalPath();
    s.version = "fixture-1.27.0.12";
    s.executable = s.home + "/DeskflowCandidate.app/Contents/MacOS/DeskflowCandidate";
    write(s.executable, "fixture executable, never run");
    write(QFileInfo(s.executable).absolutePath() + "/../Info.plist", "fixture version");
    for (auto i = s.services.begin(); i != s.services.end(); ++i) {
      i->path = s.home + "/Library/LaunchAgents/" + i.key() + ".plist";
      if (!i->definition.isEmpty())
        write(i->path, i->definition);
    }
    write(s.home + "/Library/Application Support/Deskflow/watchdog.zsh", "fixture script, never run\n");
    commands->reader = reader;
  }
  std::unique_ptr<MacStartupExecutor> executor()
  {
    return std::make_unique<MacStartupExecutor>(reader, commands, [this] { return handoff; });
  }
  Plan plan()
  {
    return prepare(reader->read(), {true, true});
  }
  void share()
  {
    reader->shared = reader->state.home + "/fixture-state.json";
    reader->saveState();
  }
};
std::shared_ptr<DiskReader> sharedReader(const QString &root)
{
  const auto allowed = QDir(qEnvironmentVariable("DESKFLOW_TEST_ROOT")).canonicalPath();
  if (allowed.isEmpty() || !root.startsWith(allowed + "/executor-") || QDir(root).canonicalPath() != root)
    qFatal("Invalid isolated root");
  auto r = std::make_shared<DiskReader>();
  r->state = startup_fixture::snapshot();
  r->state.home = root;
  r->state.executable = root + "/DeskflowCandidate.app/Contents/MacOS/DeskflowCandidate";
  for (auto i = r->state.services.begin(); i != r->state.services.end(); ++i)
    i->path = root + "/Library/LaunchAgents/" + i.key() + ".plist";
  r->shared = root + "/fixture-state.json";
  r->read();
  return r;
}
int fixtureProcess(const QStringList &args)
{
  if (args.size() < 3)
    return 3;
  auto reader = sharedReader(args[2]);
  auto commands = std::make_shared<FakeCommands>();
  commands->reader = reader;
  if (args[1] == "--fixture-worker" && args.size() == 8) {
    commands->fail = args[6].toInt();
    commands->unknown = args[7].toInt();
    return runStartupWorkerSession(
        {args[0], "--candidate-startup-transaction", args[3], args[4]}, reader, commands,
        [&] { return args[5] == "gone"; }, 1000
    );
  }
  MacStartupExecutor executor(reader, commands, [] { return true; });
  Result result;
  if (args[1] == "--fixture-apply") {
    const auto p = prepare(reader->read(), {true, true});
    result = apply(p, p.approvalDigest, true, executor);
  } else if (args[1] == "--fixture-restore" && args.size() == 7) {
    commands->fail = args[5].toInt();
    commands->unknown = args[6].toInt();
    result = executor.restorePersisted(args[3], args[4].toLatin1(), true);
  } else
    return 3;
  QTextStream(stdout) << QJsonDocument(
                             QJsonObject{{"directory", executor.backupDirectory()}, {"outcome", int(result.outcome)}}
                         ).toJson(QJsonDocument::Compact)
                      << Qt::endl;
  return result.outcome == Outcome::Applied || result.outcome == Outcome::RolledBack ? 0 : 6;
}
QJsonObject childApply(Fixture &f)
{
  f.share();
  QProcess child;
  child.start(QCoreApplication::applicationFilePath(), {"--fixture-apply", f.reader->state.home});
  if (!child.waitForFinished(5000) || child.exitStatus() != QProcess::NormalExit || child.exitCode() != 0)
    return {};
  return QJsonDocument::fromJson(child.readAllStandardOutput()).object();
}
QByteArray receive(QLocalSocket &socket)
{
  if (!socket.canReadLine())
    socket.waitForReadyRead(1500);
  return socket.canReadLine() ? socket.readLine().trimmed() : QByteArray{};
}
} // namespace
class MacStartupExecutorTests : public QObject
{
  Q_OBJECT
private slots:
  void directCallsAndMissingConsentAreInert()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    QVERIFY(p.valid());
    QCOMPARE(e->perform(p.operations.front()), Effect::FailedUnchanged);
    QCOMPARE(apply(p, "wrong", true, *e).outcome, Outcome::NeedsConfirmation);
    QCOMPARE(apply(p, p.approvalDigest, false, *e).outcome, Outcome::NeedsConfirmation);
    f.handoff = false;
    QCOMPARE(apply(p, p.approvalDigest, true, *e).outcome, Outcome::NeedsConfirmation);
    QVERIFY(f.commands->calls.isEmpty());
    QVERIFY(e->backupDirectory().isEmpty());
  }
  void exactPlanBackupAndRestore()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    const auto awake = f.reader->state.services[keepawake];
    const auto r = apply(p, p.approvalDigest, true, *e);
    QCOMPARE(r.outcome, Outcome::Applied);
    QVERIFY(!r.receipt.isEmpty());
    QCOMPARE(f.reader->read().revision(), p.after.revision());
    for (const auto &call : f.commands->calls)
      QVERIFY(!call.contains(keepawake));
    QVERIFY(!readBytes(p.after.services[candidate].path).contains("caffeinate"));
    QCOMPARE(readBytes(e->backupDirectory() + "/" + original + ".plist"), p.before.services[original].definition);
    QCOMPARE(hash(readBytes(e->backupDirectory() + "/manifest.json")), r.receipt);
    QCOMPARE(
        QFileInfo(e->backupDirectory()).permissions() & (QFileDevice::ReadGroup | QFileDevice::ReadOther),
        QFileDevice::Permissions{}
    );
    QCOMPARE(rollback(p, r, p.approvalDigest, *e).outcome, Outcome::RolledBack);
    QCOMPARE(f.reader->read().revision(), p.before.revision());
    QVERIFY(!QFileInfo::exists(p.before.services[candidate].path));
    QCOMPARE(f.reader->state.services[keepawake].running, awake.running);
    const int calls = f.commands->calls.size();
    QCOMPARE(e->restore(p.before, r.receipt), Effect::FailedUnchanged);
    QCOMPARE(f.commands->calls.size(), calls);
  }
  void eachCommandFailureRestores_data()
  {
    QTest::addColumn<int>("position");
    for (int i = 1; i <= 6; ++i)
      QTest::newRow(qPrintable(QString::number(i))) << i;
  }
  void eachCommandFailureRestores()
  {
    QFETCH(int, position);
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    f.commands->fail = position;
    const auto r = apply(p, p.approvalDigest, true, *e);
    QCOMPARE(r.outcome, Outcome::FailedRolledBack);
    QCOMPARE(f.reader->read().revision(), p.before.revision());
  }
  void ambiguousCommandStopsWithoutRollback()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    f.commands->unknown = 2;
    const auto r = apply(p, p.approvalDigest, true, *e);
    QCOMPARE(r.outcome, Outcome::RecoveryRequired);
    QCOMPARE(f.commands->calls.size(), 2);
    QCOMPARE(e->restore(p.before, r.receipt), Effect::FailedUnchanged);
    const auto journal = QJsonDocument::fromJson(readBytes(e->backupDirectory() + "/journal.json")).object();
    QCOMPARE(journal["status"].toString(), QString("uncertain"));
    e.reset();
    auto retry = f.executor();
    const auto next = f.plan();
    const auto again = apply(next, next.approvalDigest, true, *retry);
    QVERIFY(again.outcome != Outcome::Applied);
    QCOMPARE(f.commands->calls.size(), 2);
    QCOMPARE(retry->error(), QString("unresolved-previous-operation"));
  }
  void nonzeroAfterMutationIsUncertain()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    f.commands->fail = 1;
    f.commands->changeThenFail = true;
    QCOMPARE(apply(p, p.approvalDigest, true, *e).outcome, Outcome::RecoveryRequired);
    QCOMPARE(f.commands->calls.size(), 1);
  }
  void executableOrVersionChangeInvalidatesConsent_data()
  {
    QTest::addColumn<int>("kind");
    for (int k = 0; k < 3; ++k)
      QTest::newRow(qPrintable(QString::number(k))) << k;
  }
  void executableOrVersionChangeInvalidatesConsent()
  {
    QFETCH(int, kind);
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    if (kind == 0)
      write(p.before.executable, "replacement");
    if (kind == 1)
      write(QFileInfo(p.before.executable).absolutePath() + "/../Info.plist", "different version");
    if (kind == 2)
      f.reader->state.version = "changed";
    QCOMPARE(apply(p, p.approvalDigest, true, *e).outcome, Outcome::Stale);
    QVERIFY(f.commands->calls.isEmpty());
  }
  void externalEditStopsAndIsNotOverwritten()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    f.commands->externalEdit = true;
    QCOMPARE(apply(p, p.approvalDigest, true, *e).outcome, Outcome::RecoveryRequired);
    QCOMPARE(f.commands->calls.size(), 1);
    QCOMPARE(readBytes(p.before.services[original].path), QByteArray("external edit"));
  }
  void backupTamperingBlocksRollback()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    const auto r = apply(p, p.approvalDigest, true, *e);
    QCOMPARE(r.outcome, Outcome::Applied);
    write(e->backupDirectory() + "/" + original + ".plist", "tampered");
    const int n = f.commands->calls.size();
    QCOMPARE(rollback(p, r, p.approvalDigest, *e).outcome, Outcome::RecoveryRequired);
    QCOMPARE(f.commands->calls.size(), n);
  }
  void symbolicLinkIsNotFollowed()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    const auto path = p.before.services[candidate].path;
    const auto other = f.reader->state.home + "/unrelated";
    QVERIFY(write(other, "do not touch"));
    QVERIFY(QFile::link(other, path));
    QVERIFY(apply(p, p.approvalDigest, true, *e).outcome != Outcome::Applied);
    QCOMPARE(readBytes(other), QByteArray("do not touch"));
    QVERIFY(f.commands->calls.isEmpty());
  }
  void failedCandidateLaunchRestoresOriginal()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    f.commands->failLaunch = 6;
    QCOMPARE(apply(p, p.approvalDigest, true, *e).outcome, Outcome::FailedRolledBack);
    QCOMPARE(f.reader->read().revision(), p.before.revision());
    QVERIFY(f.reader->state.services[original].running);
  }
  void missingHandoffAfterBackupStops()
  {
    Fixture f;
    auto e = f.executor();
    const auto p = f.plan();
    QVERIFY(e->authorize(p, p.approvalDigest, true));
    QCOMPARE(e->perform(p.operations.front()), Effect::Applied);
    f.handoff = false;
    QCOMPARE(e->perform(p.operations[1]), Effect::FailedUnchanged);
    QVERIFY(f.commands->calls.isEmpty());
  }
  void nativeGateIsClosedInTests()
  {
    QVERIFY(!nativeStartupChangesAvailable());
  }
  void handoffRejectsChangedPlanAndExtraFields()
  {
    Fixture f;
    const auto p = f.plan();
    auto request = handoffRequest(p, p.approvalDigest);
    Plan decoded;
    QVERIFY(checkHandoffRequest(request, f.reader->read(), decoded));
    QCOMPARE(decoded.approvalDigest, p.approvalDigest);
    QVERIFY(handoffRequest(p, "not-confirmed").isEmpty());
    request["extra"] = "not permitted";
    QVERIFY(!checkHandoffRequest(request, f.reader->read(), decoded));
    request.remove("extra");
    request["login"] = false;
    QVERIFY(!checkHandoffRequest(request, f.reader->read(), decoded));
    request = handoffRequest(p, p.approvalDigest);
    write(p.before.executable, "changed");
    QVERIFY(!checkHandoffRequest(request, f.reader->read(), decoded));
  }
  void closedNativeGateCannotLaunchWorker()
  {
    Fixture f;
    const auto p = f.plan();
    StartupHandoff handoff("fixture-key");
    QVERIFY(!handoff.begin(p, p.approvalDigest));
    QCOMPARE(
        runStartupWorker(
            {"fixture", "--candidate-startup-transaction", "fixture-socket", QString(64, 'a')}, "fixture-key"
        ),
        3
    );
    QVERIFY(f.commands->calls.isEmpty());
  }
  void profileAndSleepStateAreUntouched()
  {
    Fixture f;
    const auto profile = f.reader->state.home + "/profile/DeskflowCandidate.conf";
    const QByteArray data = "paused=true\ncertificatePath=old-directory/example.pem\nunknown-layout-key=retained\n";
    QVERIFY(write(profile, data));
    auto e = f.executor();
    const auto p = f.plan();
    const auto r = apply(p, p.approvalDigest, true, *e);
    QCOMPARE(r.outcome, Outcome::Applied);
    QCOMPARE(readBytes(profile), data);
    QCOMPARE(rollback(p, r, p.approvalDigest, *e).outcome, Outcome::RolledBack);
    QCOMPARE(readBytes(profile), data);
  }
  void separateProcessRestoreUsesOnlySavedReceipt_data()
  {
    QTest::addColumn<int>("failure");
    QTest::newRow("success") << -1;
    for (int i = 1; i <= 7; ++i)
      QTest::newRow(qPrintable(QString("failure-%1").arg(i))) << i;
  }
  void separateProcessRestoreUsesOnlySavedReceipt()
  {
    QFETCH(int, failure);
    Fixture f;
    const auto before = f.reader->read();
    const auto report = childApply(f);
    QVERIFY(!report.isEmpty());
    const auto directory = report["directory"].toString();
    const auto record = inspectRecoveryBackup(f.reader->read(), directory);
    QVERIFY2(record.history.restorable, qPrintable(record.history.error));
    QVERIFY(record.history.restoreDigest != record.plan.approvalDigest);
    QProcess child;
    child.start(
        QCoreApplication::applicationFilePath(),
        {"--fixture-restore", f.reader->state.home, directory, QString::fromLatin1(record.history.restoreDigest),
         QString::number(failure), "-1"}
    );
    QVERIFY(child.waitForFinished(5000));
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    const auto journal = QJsonDocument::fromJson(readBytes(directory + "/journal.json")).object();
    if (failure < 0) {
      QCOMPARE(child.exitCode(), 0);
      QCOMPARE(f.reader->read().revision(), before.revision());
      QCOMPARE(journal["status"].toString(), QString("restored"));
      QCOMPARE(latestStartupOperation(f.reader->read()).status, QString("restored"));
    } else {
      QCOMPARE(child.exitCode(), 6);
      QCOMPARE(journal["status"].toString(), QString("uncertain"));
      QVERIFY(!inspectRecoveryBackup(f.reader->read(), directory).history.restorable);
    }
  }
  void persistedRestoreRejectsMissingConsentCorruptionAndUpgrade_data()
  {
    QTest::addColumn<int>("kind");
    for (int k = 0; k < 5; ++k)
      QTest::newRow(qPrintable(QString::number(k))) << k;
  }
  void persistedRestoreRejectsMissingConsentCorruptionAndUpgrade()
  {
    QFETCH(int, kind);
    Fixture f;
    const auto report = childApply(f);
    const auto directory = report["directory"].toString();
    const auto record = inspectRecoveryBackup(f.reader->read(), directory);
    QVERIFY(record.history.restorable);
    const auto beforeLog = readBytes(f.reader->state.home + "/fixture-commands.log");
    QByteArray consent = record.history.restoreDigest;
    if (kind == 0)
      consent = "not-confirmed";
    if (kind == 1)
      write(directory + "/manifest.json", "corrupt");
    if (kind == 2)
      write(directory + "/" + candidate + ".plist", "modified backup");
    if (kind == 3)
      write(f.reader->state.executable, "replacement build");
    if (kind == 4)
      write(f.reader->state.services[original].path, "unrelated user edit");
    QProcess child;
    child.start(
        QCoreApplication::applicationFilePath(),
        {"--fixture-restore", f.reader->state.home, directory, QString::fromLatin1(consent), "-1", "-1"}
    );
    QVERIFY(child.waitForFinished(5000));
    QVERIFY(child.exitCode() != 0);
    QCOMPARE(readBytes(f.reader->state.home + "/fixture-commands.log"), beforeLog);
  }
  void crossProcessHandoff_data()
  {
    QTest::addColumn<QString>("scenario");
    for (const auto &s :
         {"apply", "restore", "no-commit", "gui-present", "state-change", "bad-request", "uncertain-command"})
      QTest::newRow(s) << QString(s);
  }
  void crossProcessHandoff()
  {
    QFETCH(QString, scenario);
    Fixture f;
    const auto before = f.reader->read();
    f.share();
    QJsonObject request;
    if (scenario == "restore") {
      const auto report = childApply(f);
      const auto record = inspectRecoveryBackup(f.reader->read(), report["directory"].toString());
      QVERIFY(record.history.restorable);
      request = restoreHandoffRequest(record, record.history.restoreDigest);
    } else {
      const auto plan = f.plan();
      request = handoffRequest(plan, plan.approvalDigest);
    }
    const auto priorCommands = readBytes(f.reader->state.home + "/fixture-commands.log");
    if (scenario == "bad-request")
      request["digest"] = "wrong";
    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    const auto name = "deskflow-fixture-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVERIFY2(server.listen(name), qPrintable(server.errorString() + " | " + QDir::tempPath() + "/" + name));
    const QString nonce(64, 'a');
    QProcess child;
    child.start(
        QCoreApplication::applicationFilePath(),
        {"--fixture-worker", f.reader->state.home, name, nonce, scenario == "gui-present" ? "present" : "gone", "-1",
         scenario == "uncertain-command" ? "2" : "-1"}
    );
    QVERIFY(server.waitForNewConnection(2000));
    auto *peer = server.nextPendingConnection();
    QVERIFY(peer);
    QCOMPARE(receive(*peer), nonce.toLatin1());
    peer->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
    peer->flush();
    if (scenario != "bad-request") {
      QCOMPARE(receive(*peer), QByteArray("ready"));
      if (scenario == "state-change")
        write(f.reader->state.executable, "changed after ready");
      if (scenario != "no-commit") {
        peer->write("commit\n");
        peer->flush();
      }
      peer->disconnectFromServer();
    }
    QVERIFY(child.waitForFinished(5000));
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    const auto history = latestStartupOperation(f.reader->read());
    if (scenario == "apply") {
      QCOMPARE(child.exitCode(), 0);
      QCOMPARE(history.status, QString("applied"));
      QVERIFY(history.restorable);
    } else if (scenario == "restore") {
      QCOMPARE(child.exitCode(), 0);
      QCOMPARE(history.status, QString("restored"));
      QCOMPARE(f.reader->read().revision(), before.revision());
    } else if (scenario == "uncertain-command") {
      QCOMPARE(child.exitCode(), 6);
      QCOMPARE(history.status, QString("uncertain"));
      QVERIFY(!history.restorable);
    } else {
      QCOMPARE(child.exitCode(), 4);
      QCOMPARE(history.status, QString("handoff-failed"));
      QCOMPARE(readBytes(f.reader->state.home + "/fixture-commands.log"), priorCommands);
    }
  }
};
int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  const auto args = app.arguments();
  if (args.size() > 1 && args[1].startsWith("--fixture-"))
    return fixtureProcess(args);
  MacStartupExecutorTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "MacStartupExecutorTests.moc"
