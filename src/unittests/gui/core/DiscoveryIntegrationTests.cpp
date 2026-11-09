// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/Settings.h"
#include "gui/core/DiscoveryCatalog.h"
#include "gui/core/DiscoveryTransport.h"
#include "gui/core/NearbyDiscovery.h"
#include "gui/core/NearbyPublisher.h"
#include "gui/dialogs/NearbyDevicesDialog.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
using namespace deskflow::gui;
class FakeTransport final : public DiscoveryTransport
{
public:
  using DiscoveryTransport::DiscoveryTransport;
  int browses = 0, stops = 0, advertisements = 0;
  QString name;
  int port = 0;
  quint32 iface = 0;
  void browse(const DiscoveryScope &scope) override
  {
    ++browses;
    iface = scope.index;
    Q_EMIT activeChanged(true);
  }
  void stop() override
  {
    ++stops;
    Q_EMIT activeChanged(false);
  }
  void advertise(const DiscoveryAdvertisement &ad) override
  {
    ++advertisements;
    name = ad.instance();
    port = ad.port;
    iface = ad.scope.index;
  }
};
class DiscoveryIntegrationTests : public QObject
{
  Q_OBJECT
  using PS = deskflow::core::ProcessState;
  using CS = deskflow::core::ConnectionState;
  static DiscoveryScope scope(quint32 index = 7)
  {
    return {index, "en7", QHostAddress("192.168.50.10"), 24};
  }
  static void deliver()
  {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
  }
private Q_SLOTS:
  void currentScopePolicyReadOnlySnapshot()
  {
    QJsonArray array;
    for (const auto &s : DiscoveryScope::current()) {
      QVERIFY(s.valid());
      array.append(
          QJsonObject{
              {"interface", s.name},
              {"index", int(s.index)},
              {"private_ipv4", s.address.toString()},
              {"prefix", s.prefix}
          }
      );
    }
    const auto output = qEnvironmentVariable("DESKFLOW_SCOPE_REPORT");
    if (!output.isEmpty()) {
      QFile file(output);
      QVERIFY(file.open(QIODevice::WriteOnly));
      file.write(QJsonDocument(array).toJson());
    }
  }
  void rejectsUnsafeNames_data()
  {
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("valid");
    QTest::newRow("Chinese") << QString("工作电脑") << true;
    QTest::newRow("html-is-plain-text") << QString("<b>PC</b>") << true;
    QTest::newRow("empty") << QString() << false;
    QTest::newRow("newline") << QString("PC\nTrust me") << false;
    QTest::newRow("bidi") << (QString("PC") + QChar(0x202e)) << false;
    QTest::newRow("null") << (QString("PC") + QChar(0)) << false;
    QTest::newRow("long-utf8") << QString(32, QChar(0x7535)) << false;
    QTest::newRow("replacement") << (QString("PC") + QChar(0xfffd)) << false;
    QTest::newRow("whitespace") << QString(" PC ") << false;
  }
  void rejectsUnsafeNames()
  {
    QFETCH(QString, name);
    QFETCH(bool, valid);
    QCOMPARE(DiscoveryCatalog::validName(name), valid);
  }
  void deduplicatesInterfacesWithoutConflatingNames()
  {
    DiscoveryCatalog c;
    QSignalSpy found(&c, &DiscoveryCatalog::found), removed(&c, &DiscoveryCatalog::removed);
    QVERIFY(c.upsert("en0:PC", "PC", "HOST.local.", 24800, 0));
    QVERIFY(c.upsert("en1:PC", "PC", "host.local", 24800, 0));
    QCOMPARE(found.count(), 1);
    QVERIFY(c.upsert("en1:other", "PC", "different.local", 24800, 0));
    QCOMPARE(found.count(), 2);
    c.remove("en0:PC");
    QCOMPARE(removed.count(), 0);
    c.remove("en1:PC");
    QCOMPARE(removed.count(), 1);
  }
  void replacementWithdrawsOldEndpoint()
  {
    DiscoveryCatalog c;
    QSignalSpy found(&c, &DiscoveryCatalog::found), removed(&c, &DiscoveryCatalog::removed);
    c.upsert("instance", "PC", "old.local", 24800, 0);
    c.upsert("instance", "PC", "new.local", 34800, 10);
    QCOMPARE(removed.count(), 1);
    QCOMPARE(removed[0][0].toString(), QString("old.local:24800"));
    QCOMPARE(found.count(), 2);
    QCOMPARE(found[1][2].toString(), QString("new.local"));
  }
  void expiryRefreshAndGoodbye()
  {
    DiscoveryCatalog c;
    QSignalSpy found(&c, &DiscoveryCatalog::found), removed(&c, &DiscoveryCatalog::removed);
    c.upsert("one", "PC", "pc.local", 24800, 0, 1000);
    c.expire(999);
    QCOMPARE(removed.count(), 0);
    c.upsert("one", "PC", "pc.local", 24800, 900, 1000);
    c.expire(1000);
    QCOMPARE(removed.count(), 0);
    c.expire(1900);
    QCOMPARE(removed.count(), 1);
    c.upsert("one", "PC", "pc.local", 24800, 2000, INT_MAX);
    c.expire(121999);
    QCOMPARE(removed.count(), 1);
    c.expire(122000);
    QCOMPARE(removed.count(), 2);
    c.upsert("one", "PC", "pc.local", 24800, 123000);
    c.upsert("one", "PC", "pc.local", 24800, 123001, 0);
    QCOMPARE(removed.count(), 3);
    QCOMPARE(found.count(), 3);
  }
  void boundedCatalogAndMalformedUpdate()
  {
    DiscoveryCatalog c;
    QSignalSpy found(&c, &DiscoveryCatalog::found);
    for (int i = 0; i < 64; ++i)
      QVERIFY(c.upsert(QString::number(i), "PC", QString("pc%1.local").arg(i), 24800, 0));
    QVERIFY(!c.upsert("extra", "PC", "extra.local", 24800, 0));
    QVERIFY(!c.upsert("0", "PC", "evil.example", 443, 1));
    QVERIFY(!c.upsert("0", "PC", "ok.local", 24800, -1));
    QCOMPARE(c.sourceCount(), 64);
    QCOMPARE(found.count(), 64);
    c.expire(120000);
    QCOMPARE(c.sourceCount(), 0);
    QVERIFY(c.upsert("extra", "PC", "extra.local", 24800, 120001));
  }
  void queuedResultFromStoppedSessionCannotResurrect()
  {
    FakeTransport t;
    NearbyDiscovery d(nullptr, &t);
    QSignalSpy found(&d, &NearbyDiscovery::found);
    d.start(scope());
    Q_EMIT t.resolved("one", "PC", "old.local", 24800, 120000);
    d.stop();
    d.start(scope());
    deliver();
    QCOMPARE(found.count(), 0);
    Q_EMIT t.resolved("two", "PC", "new.local", 24800, 120000);
    deliver();
    QCOMPARE(found.count(), 1);
    d.stop();
    Q_EMIT t.resolved("late", "PC", "late.local", 24800, 120000);
    deliver();
    QCOMPARE(found.count(), 1);
  }
  void publisherRequiresOptInListeningAndSecurity()
  {
    FakeTransport t;
    NearbyPublisher p(nullptr, &t);
    p.configure(false, true, 24800, true, true, scope(7));
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    QCOMPARE(t.advertisements, 0);
    p.configure(true, true, 24800, false, true, scope(7));
    QCOMPARE(t.advertisements, 0);
    p.configure(true, true, 24800, true, false, scope(7));
    QCOMPARE(t.advertisements, 0);
    p.configure(true, false, 24800, true, true, scope(7));
    QCOMPARE(t.advertisements, 0);
    p.configure(true, true, 24800, true, true, scope(7));
    QCOMPARE(t.advertisements, 1);
    QCOMPARE(t.iface, quint32(7));
    p.connectionChanged(CS::Connected);
    QCOMPARE(t.advertisements, 1);
    p.processChanged(PS::Stopped);
    QCOMPARE(t.stops, 1);
    p.connectionChanged(CS::Listening);
    QCOMPARE(t.advertisements, 1);
  }
  void sustainedDiscoveryRenewsWithoutTrustAndStopsOnExplicitDisable()
  {
    FakeTransport t;
    QVector<DiscoveryScope> scopes{scope()};
    NearbyPublisher p(nullptr, &t, [&] { return scopes; });
    p.configure(true, true, 34800, true, true, scope(), true);
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    QCOMPARE(t.advertisements, 1);
    const auto first = t.name;
    p.renew();
    QCOMPARE(t.advertisements, 2);
    QVERIFY(t.name != first);
    p.processChanged(PS::Stopped);
    QVERIFY(p.enabled());
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    QCOMPARE(t.advertisements, 3);
    p.configure(false, true, 34800, true, true, scope(), true);
    p.renew();
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    QCOMPARE(t.advertisements, 3);
    QVERIFY(!p.enabled());
  }
  void sustainedDiscoveryOnlyFollowsSameApprovedPrivateLink()
  {
    FakeTransport t;
    QVector<DiscoveryScope> scopes{scope()};
    NearbyPublisher p(nullptr, &t, [&] { return scopes; });
    p.configure(true, true, 34800, true, true, scope(), true);
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    scopes[0].address = QHostAddress("192.168.50.11");
    p.networkChanged();
    QCOMPARE(t.advertisements, 2);
    QCOMPARE(p.scope(), scopes[0]);
    scopes.clear();
    p.networkChanged();
    QVERIFY(p.enabled());
    const auto before = t.advertisements;
    p.renew();
    QCOMPARE(t.advertisements, before);
    auto other = scope();
    other.name = "en8";
    other.index = 8;
    scopes = {other};
    p.networkChanged();
    QCOMPARE(t.advertisements, before);
    other = scope();
    other.address = QHostAddress("192.168.60.10");
    scopes = {other};
    p.networkChanged();
    QCOMPARE(t.advertisements, before);
    scopes = {scope()};
    p.networkChanged();
    QCOMPARE(t.advertisements, before + 1);
    Q_EMIT t.sessionEnded();
    QVERIFY(!p.enabled());
    p.renew();
    QCOMPARE(t.advertisements, before + 1);
  }
  void sustainedBackendAddressExpiryRevalidatesAndRolesCancel()
  {
    FakeTransport t;
    QVector<DiscoveryScope> scopes{scope()};
    NearbyPublisher p(nullptr, &t, [&] { return scopes; });
    p.configure(true, true, 34800, true, true, scope(), true);
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    scopes[0].address = QHostAddress("192.168.50.12");
    Q_EMIT t.sessionEnded();
    QVERIFY(p.enabled());
    QCOMPARE(t.advertisements, 2);
    p.configure(true, false, 34800, true, true, scopes[0], true);
    QVERIFY(!p.enabled());
    p.renew();
    QCOMPARE(t.advertisements, 2);
  }
  void publisherWithdrawsOnDisableAndRefreshesChangedAddressScope()
  {
    FakeTransport t;
    NearbyPublisher p(nullptr, &t);
    p.configure(true, true, 24800, true, true, scope(1));
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    const auto firstName = t.name;
    p.networkChanged();
    QCOMPARE(t.stops, 1);
    QCOMPARE(t.advertisements, 1);
    p.connectionChanged(CS::Listening);
    QCOMPARE(t.advertisements, 1);
    p.configure(true, true, 34800, true, true, scope(2));
    QCOMPARE(t.advertisements, 2);
    QCOMPARE(t.port, 34800);
    QCOMPARE(t.iface, quint32(2));
    QVERIFY(t.name != firstName);
    p.configure(false, true, 34800, true, true, scope(2));
    QCOMPARE(t.stops, 2);
    p.networkChanged();
    QCOMPARE(t.advertisements, 2);
  }
  void publisherExposureIsBoundedAndAliasIsAnonymous()
  {
    FakeTransport t;
    NearbyPublisher p(nullptr, &t);
    p.configure(true, true, 34800, true, true, scope());
    p.processChanged(PS::Started);
    p.connectionChanged(CS::Listening);
    QVERIFY(QRegularExpression("^Deskflow-[0-9a-f]{24}$").match(t.name).hasMatch());
    auto *deadline = p.findChild<QTimer *>("publisherDeadline");
    QVERIFY(deadline);
    QCOMPARE(deadline->interval(), 300000);
    QVERIFY(QMetaObject::invokeMethod(deadline, "timeout", Qt::DirectConnection));
    QCOMPARE(t.stops, 1);
    p.connectionChanged(CS::Listening);
    QCOMPARE(t.advertisements, 1);
  }
  void scopeRejectsZeroPublicAndChangedInterfaces()
  {
    FakeTransport t;
    NearbyDiscovery d(nullptr, &t);
    d.start({});
    QCOMPARE(t.browses, 0);
    auto bad = scope();
    bad.address = QHostAddress("8.8.8.8");
    d.start(bad);
    QCOMPARE(t.browses, 0);
    d.start(scope());
    QCOMPARE(t.iface, quint32(7));
    QSignalSpy found(&d, &NearbyDiscovery::found);
    Q_EMIT t.sessionEnded();
    Q_EMIT t.resolved("late", "PC", "late.local", 34800, 120000);
    deliver();
    QCOMPARE(found.count(), 0);
  }
  void rejectsNonPhysicalLinks_data()
  {
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("label");
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("flags");
    QTest::addColumn<bool>("allowed");
    const int flags = QNetworkInterface::IsUp | QNetworkInterface::IsRunning | QNetworkInterface::CanMulticast;
    QTest::newRow("wifi") << QString("en1") << QString("Wi-Fi") << int(QNetworkInterface::Wifi) << flags << true;
    QTest::newRow("ethernet") << QString("Ethernet") << QString("Ethernet") << int(QNetworkInterface::Ethernet) << flags
                              << true;
    for (const auto &name : QStringList{
             "utun3", "tun0", "tap0", "ppp0", "ipsec0", "wg0", "bridge0", "awdl0", "llw0", "vmnet1", "vbox0", "virbr0",
             "veth0", "docker0", "zt0", "tailscale0"
         })
      QTest::newRow(qPrintable(name)) << name << QString() << int(QNetworkInterface::Ethernet) << flags << false;
    QTest::newRow("vpn-label") << QString("{ABC}") << QString("NoLock VPN") << int(QNetworkInterface::Ethernet) << flags
                               << false;
    QTest::newRow("virtual") << QString("adapter") << QString() << int(QNetworkInterface::Virtual) << flags << false;
    QTest::newRow("unknown") << QString("adapter") << QString() << int(QNetworkInterface::Unknown) << flags << false;
    QTest::newRow("down") << QString("en1") << QString() << int(QNetworkInterface::Wifi) << 0 << false;
    QTest::newRow("point-to-point") << QString("en1") << QString() << int(QNetworkInterface::Ethernet)
                                    << (flags | QNetworkInterface::IsPointToPoint) << false;
  }
  void rejectsNonPhysicalLinks()
  {
    QFETCH(QString, name);
    QFETCH(QString, label);
    QFETCH(int, type);
    QFETCH(int, flags);
    QFETCH(bool, allowed);
    QCOMPARE(
        DiscoveryScope::eligibleLink(
            name, label, static_cast<QNetworkInterface::InterfaceType>(type), QNetworkInterface::InterfaceFlags(flags)
        ),
        allowed
    );
  }
  void rejectsPublicAndUnsafeAddresses_data()
  {
    QTest::addColumn<QString>("address");
    QTest::addColumn<int>("prefix");
    QTest::addColumn<bool>("allowed");
    for (const auto &ip : QStringList{"192.168.1.1", "10.1.1.1", "172.16.1.1", "169.254.1.1"})
      QTest::newRow(qPrintable(ip)) << ip << 24 << true;
    for (const auto &ip : QStringList{
             "8.8.8.8", "100.64.1.1", "127.0.0.1", "224.0.0.251", "0.0.0.0", "::1", "fe80::1", "fd00::1", "2001:db8::1",
             "192.168.1.0", "192.168.1.255"
         })
      QTest::newRow(qPrintable(ip)) << ip << 24 << false;
    QTest::newRow("crosses-public-subnet") << QString("192.168.1.1") << 8 << false;
    QTest::newRow("host-route") << QString("192.168.1.1") << 32 << false;
  }
  void rejectsPublicAndUnsafeAddresses()
  {
    QFETCH(QString, address);
    QFETCH(int, prefix);
    QFETCH(bool, allowed);
    auto s = scope();
    s.address = QHostAddress(address);
    s.prefix = prefix;
    QCOMPARE(s.valid(), allowed);
  }
  void dialogRequiresExplicitNetworkAndSearch()
  {
    NearbyDevicesDialog dialog(nullptr, [] { return QVector<DiscoveryScope>{scope()}; });
    auto *network = dialog.findChild<QComboBox *>("nearbyNetwork");
    auto *search = dialog.findChild<QPushButton *>("refreshNearby");
    QVERIFY(network && search);
    QCOMPARE(network->currentIndex(), 0);
    QVERIFY(!search->isEnabled());
    QSignalSpy active(dialog.findChild<NearbyDiscovery *>(), &NearbyDiscovery::searchingChanged);
    dialog.show();
    QTest::qWait(10);
    QCOMPARE(active.count(), 0);
    network->setCurrentIndex(1);
    QVERIFY(search->isEnabled());
    for (const auto &event : active)
      QVERIFY(!event[0].toBool());
    auto *reload = dialog.findChild<QPushButton *>("refreshNetworks");
    QVERIFY(reload);
    reload->click();
    QCOMPARE(network->currentIndex(), 0);
    QVERIFY(!search->isEnabled());
  }
  void changedSelectionRequiresFreshConsent()
  {
    NearbyDevicesDialog dialog;
    auto *provider = dialog.findChild<NearbyDiscovery *>();
    auto *list = dialog.findChild<QListWidget *>("nearbyDevices");
    auto *select = dialog.findChild<QPushButton *>("selectNearbyAddress");
    Q_EMIT provider->found("same", "PC", "old.local", 24800);
    list->setCurrentRow(0);
    QVERIFY(select->isEnabled());
    Q_EMIT provider->found("same", "PC", "new.local", 24800);
    QVERIFY(!select->isEnabled());
    QVERIFY(dialog.selectedHost().isEmpty());
    list->setCurrentRow(0);
    QVERIFY(select->isEnabled());
    QCOMPARE(dialog.selectedHost(), QString("new.local"));
    Q_EMIT provider->found("other", "Other", "different.local", 24800);
    Q_EMIT provider->removed("same");
    QVERIFY(!select->isEnabled());
    QCOMPARE(list->count(), 1);
  }
  void advertisingDefaultsOffAndLabTransportCannotActivate()
  {
    QCOMPARE(Settings::value(Settings::Server::DiscoveryEnabled).toBool(), false);
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    QSignalSpy active(t.get(), &DiscoveryTransport::activeChanged);
    t->browse(scope());
    t->advertise({scope(), DiscoveryAdvertisement::newToken(), 34800});
    for (const auto &event : active)
      QCOMPARE(event[0].toBool(), false);
  }
};
int main(int argc, char **argv)
{
  const auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (root.isEmpty() || !QDir(root).isAbsolute() || !QDir(root).exists())
    return 2;
  QTemporaryDir fixture(root + "/discovery-XXXXXX");
  if (!fixture.isValid())
    return 2;
  qputenv("XDG_CONFIG_HOME", (fixture.path() + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (fixture.path() + "/state").toUtf8());
  QApplication app(argc, argv);
  DiscoveryIntegrationTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "DiscoveryIntegrationTests.moc"
