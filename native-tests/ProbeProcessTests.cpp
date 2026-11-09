// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// The real QProcess executor with an inert compiled child; no socket/TLS/input.
#include "common/CredentialStore.h"
#include "gui/core/RecoveryProbe.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <QUuid>
using namespace deskflow::gui;
using namespace deskflow::auth;
using namespace deskflow::lab;
static QString fixtureParent;
namespace deskflow::gui {
QString probeProcessFixtureParent()
{
  return fixtureParent;
}
} // namespace deskflow::gui
static QJsonObject manifest(const QString &id)
{
  return {
      {"schema", 1},
      {"purpose", "deskflow-lab-network"},
      {"session", id},
      {"mode", "client"},
      {"bindAddress", "192.168.50.10"},
      {"peerAddress", "192.168.50.20"},
      {"port", 34800},
      {"tls", true},
      {"peerChecks", true}
  };
}
static int inertWorker(QStringList args)
{
  if (args.size() == 9 && args[7] == "--candidate-profile") {
    if (CredentialStore::candidateSessionsDirectory(args[8]) != QFileInfo(args[2]).absolutePath())
      return 3;
    args = args.mid(0, 7);
  }
  if (args.size() != 7 || args[1] != "--run" || args[3] != "--peer-auth-context" || args[5] != "--interface-index" ||
      args[6] != "7")
    return 3;
  const auto c = Context::decode(args[4]);
  if (!c.valid())
    return 3;
  const auto parent = QFileInfo(args[2]).absolutePath();
  QString error;
  auto lease = CredentialStore::acquire(args[2], parent, &error);
  if (!lease || lease->material().identity != "synthetic-invalid-no-key")
    return 3;
  const auto line = [](const Report &r) {
    return QJsonDocument(
               QJsonObject{{"event", "peerAuthentication"}, {"payload", r.encode()}}
           ).toJson(QJsonDocument::Compact) +
           '\n';
  };
  // Delays exercise cancellation and timeout of this exact child, never a service.
  if (c.generation == 3 || c.generation == 6)
    QTest::qWait(10000);
  if (c.generation == 8)
    QTest::qWait(500);
  QByteArray output = line({c, 1, Outcome::Started, {}});
  output += line(
      {c, 1, c.generation == 2 ? Outcome::WrongIdentity : Outcome::Verified,
       c.generation == 2 ? QString(64, 'b') : c.expected}
  );
  if (c.generation == 4)
    output = "invalid worker stdout\n";
  if (c.generation == 7)
    output += QByteArray(9000, 'x');
  lease.reset();
  if (!CredentialStore::recover(args[2], parent).isEmpty())
    return 3;
  QTextStream(stdout) << output;
  return c.generation == 2 || c.generation == 5 ? 4 : 0;
}
class ProbeProcessTests : public QObject
{
  Q_OBJECT
  QTemporaryDir parent;
  QString prepared;
  const DiscoveryScope scope{7, "Fixture", QHostAddress("192.168.50.10"), 24};
  Context context(quint64 mode = 1)
  {
    return {QUuid::createUuid().toString(QUuid::Id128), QString(64, 'a'), mode};
  }
  bool gone(const Context &c)
  {
    return !QFileInfo::exists(fixtureParent + '/' + c.session + "/credentials.bundle");
  }
private Q_SLOTS:
  void initTestCase()
  {
    QVERIFY(parent.isValid());
    fixtureParent = QFileInfo(parent.path()).canonicalFilePath();
    const auto id = QUuid::createUuid().toString(QUuid::Id128);
    prepared = fixtureParent + '/' + id;
    QCOMPARE(CredentialStore::createSession(prepared, fixtureParent, manifest(id)), QString());
    QCOMPARE(
        CredentialStore::provision(
            prepared, fixtureParent, {"synthetic-invalid-no-key", "synthetic-servers", "synthetic-clients"}
        ),
        QString()
    );
  }
  void childOutputAndCleanup_data()
  {
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("accepted");
    QTest::newRow("verified") << 1 << true;
    QTest::newRow("wrong-identity") << 2 << true;
    QTest::newRow("malformed-output") << 4 << false;
    QTest::newRow("wrong-exit") << 5 << false;
    QTest::newRow("overflow") << 7 << false;
  }
  void childOutputAndCleanup()
  {
    QFETCH(int, mode);
    QFETCH(bool, accepted);
    ProcessTlsProbe p(nullptr, [&] { return QVector<DiscoveryScope>{scope}; });
    QVERIFY(p.selectPreparedSession(prepared));
    QSignalSpy ready(&p, &TlsProbeExecutor::completed), failed(&p, &TlsProbeExecutor::failed);
    const auto c = context(mode);
    p.start(c, scope, "192.168.50.20", 34800);
    QTRY_COMPARE_WITH_TIMEOUT(ready.count() + failed.count(), 1, 5000);
    QCOMPARE(ready.count(), int(accepted));
    QTRY_VERIFY(gone(c));
  }
  void cancellationAndRestartCannotReturnOldResult()
  {
    ProcessTlsProbe p(nullptr, [&] { return QVector<DiscoveryScope>{scope}; });
    QVERIFY(p.selectPreparedSession(prepared));
    QSignalSpy ready(&p, &TlsProbeExecutor::completed), failed(&p, &TlsProbeExecutor::failed);
    const auto old = context(3);
    p.start(old, scope, "192.168.50.20", 34800);
    QTest::qWait(150);
    p.cancel();
    const auto current = context();
    p.start(current, scope, "192.168.50.20", 34800);
    QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 5000);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(qvariant_cast<Context>(ready.at(0).at(0)), current);
    QTRY_VERIFY(gone(old));
    QVERIFY(gone(current));
  }
  void changedInterfaceKillsChildWithoutCompletion()
  {
    QVector<DiscoveryScope> scopes{scope};
    ProcessTlsProbe p(nullptr, [&] { return scopes; });
    QVERIFY(p.selectPreparedSession(prepared));
    QSignalSpy ready(&p, &TlsProbeExecutor::completed), failed(&p, &TlsProbeExecutor::failed);
    const auto c = context(3);
    p.start(c, scope, "192.168.50.20", 34800);
    QTest::qWait(100);
    scopes.clear();
    QTRY_COMPARE(failed.count(), 1);
    QCOMPARE(ready.count(), 0);
    QTRY_VERIFY(gone(c));
  }
  void deadlineTerminatesAndCleansOnlyOwnedChild()
  {
    ProcessTlsProbe p(nullptr, [&] { return QVector<DiscoveryScope>{scope}; });
    QVERIFY(p.selectPreparedSession(prepared));
    QSignalSpy ready(&p, &TlsProbeExecutor::completed), failed(&p, &TlsProbeExecutor::failed);
    const auto c = context(6);
    p.start(c, scope, "192.168.50.20", 34800);
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 6500);
    QCOMPARE(ready.count(), 0);
    QTRY_VERIFY(gone(c));
    QVERIFY(QFileInfo::exists(prepared + "/credentials.bundle"));
  }
  void candidateProfileSurvivesExecutorRestartAndRevocationStopsCommit()
  {
    const auto profile = fixtureParent + "/DeskflowCandidate", settings = profile + "/DeskflowCandidate.conf",
               identity = profile + "/tls/deskflow-candidate.pem";
    QVERIFY(QDir().mkpath(profile));
    QVERIFY(CredentialStore::ensurePrivateCandidateDirectory(profile + "/tls"));
    QFile key(identity);
    QVERIFY(CredentialStore::openPrivateCandidateFile(key));
    key.write("synthetic-invalid-no-key");
    key.close();
    const auto fp = QString(64, 'a'), trust = profile + "/tls/trusted-servers";
    QVERIFY(CredentialStore::writeCandidateTrust(trust, ("v2:sha256:" + fp + "\n").toUtf8()));
    for (int iteration = 0; iteration < 2; ++iteration) {
      ProcessTlsProbe probe(nullptr, [&] { return QVector<DiscoveryScope>{scope}; });
      QVERIFY(probe.selectCandidateProfile(settings, identity, fp));
      QVERIFY(probe.ready());
      QSignalSpy accepted(&probe, &TlsProbeExecutor::completed), failed(&probe, &TlsProbeExecutor::failed);
      const auto c = context();
      probe.start(c, scope, "192.168.50.20", 34800);
      QTRY_COMPARE_WITH_TIMEOUT(accepted.count(), 1, 5000);
      QCOMPARE(failed.count(), 0);
      QVERIFY(!QFileInfo::exists(profile + "/recovery-sessions/" + c.session + "/credentials.bundle"));
    }
    ProcessTlsProbe probe(nullptr, [&] { return QVector<DiscoveryScope>{scope}; });
    QVERIFY(probe.selectCandidateProfile(settings, identity, fp));
    QSignalSpy accepted(&probe, &TlsProbeExecutor::completed), failed(&probe, &TlsProbeExecutor::failed);
    probe.start(context(8), scope, "192.168.50.20", 34800);
    QTest::qWait(150);
    QVERIFY(CredentialStore::writeCandidateTrust(trust, "\n"));
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
    QCOMPARE(accepted.count(), 0);
    QVERIFY(!probe.ready());
  }
  void cleanupTestCase()
  {
    QCOMPARE(CredentialStore::recover(prepared, fixtureParent), QString());
  }
};
int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  if (app.arguments().contains("--run"))
    return inertWorker(app.arguments());
  ProbeProcessTests test;
  return QTest::qExec(&test, argc, argv);
}
#include "ProbeProcessTests.moc"
