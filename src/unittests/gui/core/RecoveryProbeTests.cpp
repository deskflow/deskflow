// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/CredentialStore.h"
#include "common/IsolatedSession.h"
#include "common/LabExecutionPolicy.h"
#include "gui/core/DeviceRecovery.h"
#include "gui/core/NearbyDiscovery.h"
#include "gui/core/RecoveryProbe.h"
#include "net/ProbeInterfaceBinding.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
using namespace deskflow::gui;
using namespace deskflow::auth;
class ResolverFixture : public ScopedAddressResolver
{
public:
  Context context;
  DiscoveryScope scope;
  QString host;
  int calls = 0, cancels = 0;
  void start(const Context &c, const DiscoveryScope &s, const QString &h, int) override
  {
    context = c;
    scope = s;
    host = h;
    ++calls;
  }
  void cancel() override
  {
    ++cancels;
  }
  void reply(QStringList addresses, int ttl = 10000)
  {
    Q_EMIT this->addresses(context, scope, host, addresses, ttl);
  }
};
class ExecutorFixture : public TlsProbeExecutor
{
public:
  Context context;
  QString numeric;
  int calls = 0, cancels = 0;
  void start(const Context &c, const DiscoveryScope &, const QString &n, int) override
  {
    context = c;
    numeric = n;
    ++calls;
  }
  void cancel() override
  {
    ++cancels;
  }
  bool selectPreparedSession(const QString &) override
  {
    return false;
  }
  void reply(bool good = true)
  {
    Q_EMIT completed(
        context, numeric,
        {context, 1, good ? Outcome::Verified : Outcome::WrongIdentity, good ? context.expected : QString(64, 'b')}
    );
  }
};
struct Chain
{
  const QString fp = QString(64, 'a');
  const DiscoveryScope scope{7, "Fixture", QHostAddress("192.168.50.10"), 24};
  QVector<DiscoveryScope> scopes{scope};
  IdentityBindings bindings;
  NearbyDiscovery discovery;
  ResolverFixture resolver;
  ExecutorFixture executor;
  qint64 now = 0;
  DeviceRecovery recovery;
  RecoveryProbeBroker broker;
  Chain()
      : recovery(bindings, discovery, nullptr, [this] { return now; }),
        broker(recovery, resolver, executor, [this] { return scopes; })
  {
    const auto g = bindings.begin({"old.local", 34800});
    bindings.observe(g, fp);
    bindings.confirm(g, "Saved", true);
    bindings.authenticated(g, fp, true, true);
  }
  void start()
  {
    if (!recovery.begin(fp, scope))
      qFatal("fixture could not begin recovery");
    Q_EMIT discovery.found("one", "Untrusted name", "new.local", 34800);
  }
  QString saved()
  {
    return bindings.devices()[fp].endpoint.host;
  }
};
class RecoveryProbeTests : public QObject
{
  Q_OBJECT
  QByteArray line(const Report &r)
  {
    return QJsonDocument(
               QJsonObject{{"event", "peerAuthentication"}, {"payload", r.encode()}}
           ).toJson(QJsonDocument::Compact) +
           '\n';
  }
  Context context()
  {
    return {QString(32, 'c'), QString(64, 'a'), 7};
  }
private Q_SLOTS:
  void interfaceBindingNeverFallsBack()
  {
    int calls = 0;
    const auto set = [&](uint32_t) {
      ++calls;
      return true;
    };
    const auto get = [](uint32_t &n) {
      n = 7;
      return true;
    };
    QVERIFY(!deskflow::net::requireProbeInterface(0, set, get));
    QCOMPARE(calls, 0);
    QVERIFY(!deskflow::net::requireProbeInterface(0x1000000, set, get));
    QCOMPARE(calls, 0);
    QVERIFY(deskflow::net::requireProbeInterface(7, set, get));
    QVERIFY(!deskflow::net::requireProbeInterface(8, set, get));
    QVERIFY(!deskflow::net::requireProbeInterface(7, [](uint32_t) { return false; }, get));
    QVERIFY(!deskflow::net::requireProbeInterface(7, set, [](uint32_t &) { return false; }));
  }
  void completeNumericRecoveryUsesPinnedIdentity()
  {
    Chain c;
    c.start();
    QCOMPARE(c.resolver.calls, 1);
    QCOMPARE(c.saved(), QString("old.local"));
    c.resolver.reply({"192.168.50.20"});
    QCOMPARE(c.executor.calls, 1);
    QVERIFY(c.executor.context != c.recovery.attempt());
    QCOMPARE(c.saved(), QString("old.local"));
    QSignalSpy ready(&c.recovery, &DeviceRecovery::recovered);
    c.executor.reply();
    QCOMPARE(c.saved(), QString("192.168.50.20"));
    QCOMPARE(ready.count(), 1);
    QCOMPARE(ready.at(0).at(1).toString(), QString("192.168.50.20"));
    QCOMPARE(c.bindings.devices().first().sha256, c.fp);
  }
  void multipleAddressesUseDistinctChildrenAndRejectOldResult()
  {
    Chain c;
    c.start();
    c.resolver.reply({"192.168.50.20", "192.168.50.20", "192.168.50.30"});
    const auto old = c.executor.context;
    const auto ip = c.executor.numeric;
    c.executor.reply(false);
    QCOMPARE(c.executor.calls, 2);
    QVERIFY(c.executor.context != old);
    Q_EMIT c.executor.completed(old, ip, {old, 1, Outcome::Verified, c.fp});
    QCOMPARE(c.saved(), QString("old.local"));
    Q_EMIT c.executor.failed(old, true);
    QCOMPARE(c.executor.calls, 2);
    c.executor.reply();
    QCOMPARE(c.saved(), QString("192.168.50.30"));
  }
  void unsafeNumericAddresses_data()
  {
    QTest::addColumn<QString>("address");
    for (const auto *a :
         {"8.8.8.8", "10.0.0.2", "127.0.0.1", "192.168.50.0", "192.168.50.255", "192.168.50.10", "::1",
          "192.168.50.020", "other.local", "224.0.0.251"})
      QTest::newRow(a) << QString(a);
  }
  void unsafeNumericAddresses()
  {
    QFETCH(QString, address);
    Chain c;
    c.start();
    c.resolver.reply({address});
    QCOMPARE(c.executor.calls, 0);
    QCOMPARE(c.saved(), QString("old.local"));
  }
  void crossInterfaceAndForgedResolverContext()
  {
    Chain c;
    c.start();
    auto wrong = c.resolver.scope;
    wrong.index = 8;
    Q_EMIT c.resolver.addresses(c.resolver.context, wrong, c.resolver.host, {"192.168.50.20"}, 10000);
    auto foreign = c.resolver.context;
    foreign.session = QString(32, 'd');
    Q_EMIT c.resolver.addresses(foreign, c.resolver.scope, c.resolver.host, {"192.168.50.20"}, 10000);
    QCOMPARE(c.executor.calls, 0);
    c.scopes.clear();
    c.resolver.reply({"192.168.50.20"});
    QCOMPARE(c.executor.calls, 0);
    c.broker.expire();
    QCOMPARE(c.saved(), QString("old.local"));
  }
  void cancellationAndWithdrawalStopBothDependencies()
  {
    Chain c;
    c.start();
    c.resolver.reply({"192.168.50.20"});
    const auto old = c.executor.context;
    c.recovery.cancel();
    c.executor.reply();
    QCOMPARE(c.saved(), QString("old.local"));
    QVERIFY(c.resolver.cancels > 0);
    QVERIFY(c.executor.cancels > 0);
    c.start();
    c.resolver.reply({"192.168.50.30"});
    Q_EMIT c.discovery.removed("one");
    c.executor.reply();
    QCOMPARE(c.saved(), QString("old.local"));
    Q_EMIT c.executor.failed(old, false);
    QCOMPARE(c.saved(), QString("old.local"));
  }
  void candidateChangeInvalidatesPendingChild()
  {
    Chain c;
    c.start();
    c.resolver.reply({"192.168.50.20"});
    const auto child = c.executor.context;
    Q_EMIT c.discovery.found("one", "Changed", "changed.local", 34800);
    Q_EMIT c.executor.completed(child, "192.168.50.20", {child, 1, Outcome::Verified, c.fp});
    QCOMPARE(c.saved(), QString("old.local"));
    QCOMPARE(c.recovery.state(), DeviceRecovery::State::Backoff);
  }
  void expiredLeaseAndTimeoutCannotCommit()
  {
    Chain c;
    c.start();
    c.resolver.reply({"192.168.50.20"}, 1);
    QTest::qWait(3);
    c.executor.reply();
    QCOMPARE(c.saved(), QString("old.local"));
    Chain d;
    d.start();
    d.resolver.reply({"192.168.50.20"});
    d.broker.expire();
    d.executor.reply();
    QCOMPARE(d.saved(), QString("old.local"));
  }
  void untrustedIdentityNeverCreatesTrustOrAddress()
  {
    Chain c;
    c.start();
    c.resolver.reply({"192.168.50.20"});
    const auto child = c.executor.context;
    Q_EMIT c.executor.completed(child, c.executor.numeric, {child, 1, Outcome::Untrusted, c.fp});
    QCOMPARE(c.saved(), QString("old.local"));
    QCOMPARE(c.recovery.state(), DeviceRecovery::State::NeedsAction);
  }
  void workerProtocolFragmentationAndExitGate()
  {
    const auto c = context();
    const auto bytes = line({c, 1, Outcome::Started, {}}) + line({c, 1, Outcome::Verified, c.expected});
    for (int exit : {0, 3, 4}) {
      ProbeOutput p(c);
      for (char ch : bytes)
        QVERIFY(p.feed(QByteArray(1, ch)));
      QCOMPARE(bool(p.finish(exit, true)), exit == 0);
    }
    ProbeOutput crash(c);
    QVERIFY(crash.feed(bytes));
    QVERIFY(!crash.finish(0, false));
  }
  void workerProtocolRejectsGarbageReplayForeignAndOverflow()
  {
    const auto c = context();
    auto other = c;
    other.session = QString(32, 'e');
    for (const auto &bad :
         {QByteArray("secret text\n"), line({c, 1, Outcome::Verified, c.expected}),
          line({other, 1, Outcome::Started, {}}),
          line({c, 1, Outcome::Started, {}}) + line({c, 2, Outcome::Verified, c.expected}),
          line({c, 1, Outcome::Started, {}}) + line({c, 1, Outcome::Started, {}}), QByteArray(8193, 'x')}) {
      ProbeOutput p(c);
      p.feed(bad);
      QVERIFY(!p.finish(0, true));
    }
  }
  void realExecutorAndResolverRemainHardDisabled()
  {
    Chain c;
    ProcessTlsProbe executor(nullptr, [&] { return c.scopes; });
    QSignalSpy failed(&executor, &TlsProbeExecutor::failed);
    QVERIFY(!deskflow::lab::kProbeNetworkExecutionEnabled);
    QVERIFY(!executor.selectPreparedSession("/Applications/Deskflow.app"));
    executor.start(context(), c.scope, "192.168.50.20", 34800);
    QCOMPARE(failed.count(), 1);
    QVERIFY(executor.findChildren<QProcess *>().isEmpty());
    std::unique_ptr<ScopedAddressResolver> resolver(createScopedAddressResolver(nullptr, [&] { return c.scopes; }, {}));
    QSignalSpy noDns(resolver.get(), &ScopedAddressResolver::failed);
    resolver->start(context(), c.scope, "new.local", 34800);
    QCOMPARE(noDns.count(), 1);
  }
  void sessionCreationRefusesReuseAndWorksWithSyntheticMaterial()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/probe-session-XXXXXX");
    const auto id = QUuid::createUuid().toString(QUuid::Id128), root = temp.path() + '/' + id;
    const QJsonObject manifest{
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
    using deskflow::lab::CredentialStore;
    QCOMPARE(CredentialStore::createSession(root, temp.path(), manifest), QString());
    QVERIFY(deskflow::lab::IsolatedSession::inspect(root, temp.path()).valid());
    QCOMPARE(
        CredentialStore::provision(root, temp.path(), {"synthetic-no-key", "synthetic-trust", "synthetic-trust"}),
        QString()
    );
    QVERIFY(!CredentialStore::createSession(root, temp.path(), manifest).isEmpty());
    QString error;
    auto lease = CredentialStore::acquire(root, temp.path(), &error);
    QVERIFY2(lease, qPrintable(error));
    lease.reset();
    QCOMPARE(CredentialStore::recover(root, temp.path()), QString());
  }
};
QTEST_GUILESS_MAIN(RecoveryProbeTests)
#include "RecoveryProbeTests.moc"
