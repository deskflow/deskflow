// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/CredentialStore.h"
#include "common/IsolatedSession.h"
#include "common/Settings.h"
#include "gui/core/DeviceRecovery.h"
#include "gui/core/DiscoveryTransport.h"
#include "gui/core/NearbyDiscovery.h"
#include "gui/dialogs/NearbyDevicesDialog.h"
#include "net/SecureSocket.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <openssl/ssl.h>
#include <unistd.h>
#ifdef Q_OS_MACOS
#include <membership.h>
#include <sys/acl.h>
#endif
using namespace deskflow::lab;
using namespace deskflow::gui;
using namespace deskflow::auth;
class FixtureTransport : public DiscoveryTransport
{
public:
  FixtureTransport() : DiscoveryTransport(nullptr)
  {
  }
  void browse(const DiscoveryScope &) override
  {
    Q_EMIT activeChanged(true);
  }
  void advertise(const DiscoveryAdvertisement &) override
  {
  }
  void stop() override
  {
    Q_EMIT activeChanged(false);
  }
};
class Stage12IntegrationTests : public QObject
{
  Q_OBJECT
  const QString pin = QString(64, 'a'), other = QString(64, 'b');
  const DiscoveryScope scope{7, "Lab-LAN", QHostAddress("192.168.50.10"), 24};
  CredentialMaterial payload() const
  {
    return {
        "synthetic-invalid-identity-no-key", QByteArray("v2:sha256:") + QByteArray(64, 'a'),
        QByteArray("v2:sha256:") + QByteArray(64, 'b')
    };
  }
  QString root(const QString &parent)
  {
    const auto id = QUuid::createUuid().toString(QUuid::Id128);
    const auto p = parent + '/' + id;
    for (const auto *d : {"config/DeskflowLab/tls", "state", "runtime", "logs"})
      QDir().mkpath(p + '/' + d);
    QFile::setPermissions(p, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    QFile f(p + "/session.json");
    if (!f.open(QIODevice::WriteOnly))
      return {};
    f.write(QJsonDocument(
                QJsonObject{
                    {"schema", 1},
                    {"purpose", "deskflow-lab-network"},
                    {"session", id},
                    {"mode", "client"},
                    {"bindAddress", "127.0.0.1"},
                    {"peerAddress", "127.0.0.1"},
                    {"port", 34800},
                    {"tls", true},
                    {"peerChecks", true}
                }
    ).toJson());
    return p;
  }
  void seed(IdentityBindings &b)
  {
    auto g = b.begin({"old.local", 34800});
    b.observe(g, pin);
    QVERIFY(b.confirm(g, "Known device", true));
    QVERIFY(b.authenticated(g, pin, true, true));
  }
  void deliver()
  {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
  }
  void candidate(FixtureTransport &t, const QString &key, const QString &host, int lease = 120000)
  {
    Q_EMIT t.resolved(key, "Untrusted name", host, 34800, lease);
    deliver();
  }
  void prove(DeviceRecovery &r, const QString &digest)
  {
    const auto c = r.attempt();
    r.receive({c, 1, Outcome::Started, {}});
    r.receive(SecureSocket::authenticatePeerDigest({c, 1, Outcome::Started, {}}, digest, true, true, true));
  }
private Q_SLOTS:
  void atomicProvisionReadLeaseAndCleanup()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    const auto p = root(temp.path());
    const auto m = payload();
    QCOMPARE(CredentialStore::provision(p, temp.path(), m), QString());
    QVERIFY(!QFileInfo::exists(p + "/credentials.next"));
    QString error;
    auto lease = CredentialStore::acquire(p, temp.path(), &error);
    QVERIFY2(lease, qPrintable(error));
    QCOMPARE(lease->material().identity, m.identity);
    QCOMPARE(CredentialStore::recover(p, temp.path()), QString("credentials-busy"));
    lease.reset();
    QCOMPARE(CredentialStore::recover(p, temp.path()), QString());
    QVERIFY(!QFileInfo::exists(p + "/credentials.bundle"));
  }
  void activationRequiresMatchingIsolatedBootstrap()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    const auto p = root(temp.path());
    QCOMPARE(CredentialStore::provision(p, temp.path(), payload()), QString());
    QString error;
    auto lease = CredentialStore::acquire(p, temp.path(), &error);
    QVERIFY2(lease, qPrintable(error));
    QVERIFY(!CredentialStore::activate(*lease));
    QVERIFY(!CredentialStore::active());
  }
  void publicationFailureDoesNotOverwriteAndCleansStaging()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    const auto p = root(temp.path());
    const auto m = payload();
    QCOMPARE(CredentialStore::provision(p, temp.path(), m), QString());
    QFile before(p + "/credentials.bundle");
    QVERIFY(before.open(QIODevice::ReadOnly));
    const auto bytes = before.readAll();
    before.close();
    QCOMPARE(
        CredentialStore::provision(p, temp.path(), {"different", m.servers, m.clients}),
        QString("atomic-publication-failed")
    );
    QVERIFY(!QFileInfo::exists(p + "/credentials.next"));
    QVERIFY(before.open(QIODevice::ReadOnly));
    QCOMPARE(before.readAll(), bytes);
  }
  void rejectProductionPathsAndRedirectedRoot()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    QVERIFY(!CredentialStore::provision("/Applications/Deskflow.app", temp.path(), payload()).isEmpty());
    const auto p = root(temp.path());
    const auto alias = temp.path() + '/' + QUuid::createUuid().toString(QUuid::Id128);
    QVERIFY(QFile::link(p, alias));
    QVERIFY(!CredentialStore::provision(alias, temp.path(), payload()).isEmpty());
    QVERIFY(!QFileInfo::exists(p + "/credentials.bundle"));
  }
  void refusedSymlinkAndHardlinkLeavesExternalFileUnchanged()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    const auto p = root(temp.path());
    QFile sentinel(temp.path() + "/sentinel");
    QVERIFY(sentinel.open(QIODevice::WriteOnly));
    sentinel.write("unchanged");
    sentinel.close();
    QVERIFY(QFile::link(sentinel.fileName(), p + "/credentials.next"));
    QVERIFY(!CredentialStore::provision(p, temp.path(), payload()).isEmpty());
    QVERIFY(!CredentialStore::recover(p, temp.path()).isEmpty());
    QVERIFY(QFile::remove(p + "/credentials.next"));
    QCOMPARE(::link(QFile::encodeName(sentinel.fileName()), QFile::encodeName(p + "/credentials.bundle")), 0);
    QString error;
    QVERIFY(!CredentialStore::acquire(p, temp.path(), &error));
    QVERIFY(!CredentialStore::recover(p, temp.path()).isEmpty());
    QVERIFY(sentinel.open(QIODevice::ReadOnly));
    QCOMPARE(sentinel.readAll(), QByteArray("unchanged"));
  }
  void rejectBroadModesCorruptionAndOversize()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    const auto p = root(temp.path());
    QCOMPARE(CredentialStore::provision(p, temp.path(), payload()), QString());
    QFile::setPermissions(
        p + "/credentials.bundle", QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadOther
    );
    QString error;
    QVERIFY(!CredentialStore::acquire(p, temp.path(), &error));
    QCOMPARE(error, QString("unsafe-credential-permissions"));
    QFile::setPermissions(p + "/credentials.bundle", QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QFile corrupt(p + "/credentials.bundle");
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write("SYNTHETIC_PRIVATE_SENTINEL");
    corrupt.close();
    QVERIFY(!CredentialStore::acquire(p, temp.path(), &error));
    QCOMPARE(error, QString("invalid-credential-bundle"));
    QVERIFY(!error.contains("SENTINEL"));
    auto m = payload();
    m.identity = QByteArray(65537, 'x');
    QCOMPARE(CredentialStore::provision(p, temp.path(), m), QString("invalid-material"));
  }
#ifdef Q_OS_MACOS
  void rejectExtendedAclOnSyntheticLeaf()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    const auto p = root(temp.path());
    QCOMPARE(CredentialStore::provision(p, temp.path(), payload()), QString());
    acl_t acl = acl_init(1);
    QVERIFY(acl);
    acl_entry_t entry = nullptr;
    QCOMPARE(acl_create_entry(&acl, &entry), 0);
    QCOMPARE(acl_set_tag_type(entry, ACL_EXTENDED_ALLOW), 0);
    uuid_t ownUuid;
    QCOMPARE(mbr_uid_to_uuid(geteuid(), ownUuid), 0);
    QCOMPARE(acl_set_qualifier(entry, ownUuid), 0);
    acl_permset_t perms;
    QCOMPARE(acl_get_permset(entry, &perms), 0);
    QCOMPARE(acl_add_perm(perms, ACL_READ_DATA), 0);
    // Only the existing owner receives a redundant read grant on synthetic
    // bytes. No user files, real credentials or system permissions are touched.
    const auto file = QFile::encodeName(p + "/credentials.bundle");
    QCOMPARE(acl_set_file(file, ACL_TYPE_EXTENDED, acl), 0);
    acl_free(acl);
    QString error;
    QVERIFY(!CredentialStore::acquire(p, temp.path(), &error));
    QCOMPARE(error, QString("unsafe-credential-permissions"));
    QCOMPARE(::link(file, QFile::encodeName(p + "/credentials.next")), 0);
    QCOMPARE(CredentialStore::recover(p, temp.path()), QString("unsafe-cleanup-target"));
    QVERIFY(QFileInfo::exists(p + "/credentials.next"));
    QVERIFY(QFileInfo::exists(p + "/credentials.bundle"));
  }
#endif
  void restartCleansIncompleteAndPublishedArtifacts()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/deskflow-credentials-XXXXXX");
    const auto p = root(temp.path());
    QFile next(p + "/credentials.next");
    QVERIFY(next.open(QIODevice::WriteOnly));
    next.write("interrupted-write");
    next.close();
    QFile::setPermissions(next.fileName(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QCOMPARE(CredentialStore::recover(p, temp.path()), QString());
    QVERIFY(!QFileInfo::exists(next.fileName()));
    QCOMPARE(CredentialStore::provision(p, temp.path(), payload()), QString());
    QCOMPARE(::link(QFile::encodeName(p + "/credentials.bundle"), QFile::encodeName(next.fileName())), 0);
    QCOMPARE(CredentialStore::recover(p, temp.path()), QString());
    QVERIFY(!QFileInfo::exists(p + "/credentials.bundle"));
  }
  void memoryTlsParserRejectsSyntheticBytesWithoutFileOrPrompt()
  {
    auto *context = SSL_CTX_new(TLS_method());
    QVERIFY(context);
    QVERIFY(!SecureSocket::loadPreparedIdentity(context, payload().identity));
    SSL_CTX_free(context);
  }
  void discoveryProofCommitsOnlySameIdentityAndResumesOnce()
  {
    IdentityBindings b;
    seed(b);
    FixtureTransport t;
    NearbyDiscovery d(nullptr, &t);
    qint64 now = 0;
    DeviceRecovery r(b, d, nullptr, [&] { return now; });
    QSignalSpy requests(&r, &DeviceRecovery::verificationRequested), ready(&r, &DeviceRecovery::recovered);
    QVERIFY(r.begin(pin, scope));
    d.start(scope);
    deliver();
    candidate(t, "source", "new.local");
    QCOMPARE(requests.count(), 1);
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
    prove(r, pin);
    QCOMPARE(ready.count(), 1);
    QCOMPARE(b.devices()[pin].endpoint.host, QString("new.local"));
    d.stop();
    QCOMPARE(r.state(), DeviceRecovery::State::Recovered);
    QCOMPARE(ready.count(), 1);
    IdentityBindings disk;
    QVERIFY(disk.load(b.encode()));
    QCOMPARE(disk.devices(), b.devices());
  }
  void forgedDuplicateAndMultipleCandidatesUseBoundedBackoff()
  {
    IdentityBindings b;
    seed(b);
    FixtureTransport t;
    NearbyDiscovery d(nullptr, &t);
    qint64 now = 0;
    DeviceRecovery r(b, d, nullptr, [&] { return now; });
    QSignalSpy requests(&r, &DeviceRecovery::verificationRequested);
    QVERIFY(r.begin(pin, scope));
    d.start(scope);
    deliver();
    candidate(t, "one", "wrong.local");
    candidate(t, "alias", "wrong.local");
    candidate(t, "two", "right.local");
    QCOMPARE(requests.count(), 1);
    const auto old = r.attempt();
    prove(r, other);
    QCOMPARE(r.state(), DeviceRecovery::State::Backoff);
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
    now = 999;
    r.tick();
    QCOMPARE(requests.count(), 1);
    now = 1000;
    r.tick();
    QCOMPARE(requests.count(), 2);
    r.receive({old, 1, Outcome::Verified, pin});
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
    prove(r, pin);
    QCOMPARE(b.devices()[pin].endpoint.host, QString("right.local"));
  }
  void withdrawalInterfaceChangeAndCancellationRejectOldProof()
  {
    IdentityBindings b;
    seed(b);
    FixtureTransport t;
    NearbyDiscovery d(nullptr, &t);
    qint64 now = 0;
    DeviceRecovery r(b, d, nullptr, [&] { return now; });
    QVERIFY(r.begin(pin, scope));
    d.start(scope);
    deliver();
    candidate(t, "one", "new.local");
    const auto old = r.attempt();
    candidate(t, "one", "new.local", 0);
    r.receive({old, 1, Outcome::Started, {}});
    r.receive({old, 1, Outcome::Verified, pin});
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
    now = 1000;
    candidate(t, "two", "other.local");
    r.tick();
    const auto next = r.attempt();
    auto changed = scope;
    changed.index = 8;
    r.interfaceChanged(changed);
    r.receive({next, 1, Outcome::Started, {}});
    r.receive({next, 1, Outcome::Verified, pin});
    QCOMPARE(r.state(), DeviceRecovery::State::Cancelled);
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
    QVERIFY(r.begin(pin, scope));
    candidate(t, "three", "last.local");
    const auto cancelled = r.attempt();
    r.cancel();
    r.receive({cancelled, 1, Outcome::Verified, pin});
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
  }
  void timeoutSecurityFailureAndForeignContextDoNotCommit()
  {
    IdentityBindings b;
    seed(b);
    FixtureTransport t;
    NearbyDiscovery d(nullptr, &t);
    qint64 now = 0;
    DeviceRecovery r(b, d, nullptr, [&] { return now; });
    QVERIFY(r.begin(pin, scope));
    d.start(scope);
    deliver();
    candidate(t, "one", "one.local");
    auto old = r.attempt();
    auto foreign = old;
    foreign.session = QString(32, 'f');
    r.receive({foreign, 1, Outcome::Started, {}});
    r.receive({foreign, 1, Outcome::Verified, pin});
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
    now = 10000;
    r.tick();
    QCOMPARE(r.state(), DeviceRecovery::State::Backoff);
    now = 11000;
    candidate(t, "two", "two.local");
    r.tick();
    auto current = r.attempt();
    r.receive({current, 1, Outcome::Started, {}});
    r.receive({current, 1, Outcome::SecurityDisabled, pin});
    QCOMPARE(r.state(), DeviceRecovery::State::NeedsAction);
    QCOMPARE(b.devices()[pin].endpoint.host, QString("old.local"));
  }
};
int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  Stage12IntegrationTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "Stage12IntegrationTests.moc"
