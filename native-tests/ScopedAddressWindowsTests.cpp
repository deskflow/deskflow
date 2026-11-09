// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Actual Windows resolver source with DNS-SD API substitutions, not Windows OS.
#include "DnsApiTestShim.h"
#include "gui/core/RecoveryProbe.h"
#include <QSignalSpy>
#include <QTest>
#include <vector>
using namespace deskflow::gui;
using namespace deskflow::auth;
namespace fixture {
std::vector<DNS_SERVICE_RESOLVE_REQUEST> requests;
int cancelled = 0, freed = 0;
} // namespace fixture
DWORD DnsServiceResolve(DNS_SERVICE_RESOLVE_REQUEST *r, DNS_SERVICE_CANCEL *)
{
  fixture::requests.push_back(*r);
  return DNS_REQUEST_PENDING;
}
DWORD DnsServiceResolveCancel(DNS_SERVICE_CANCEL *)
{
  ++fixture::cancelled;
  return 0;
}
void DnsServiceFreeInstance(PDNS_SERVICE_INSTANCE)
{
  ++fixture::freed;
}
class ScopedAddressWindowsTests : public QObject
{
  Q_OBJECT
  const DiscoveryScope scope{7, "Fixture", QHostAddress("192.168.50.10"), 24};
  Context context()
  {
    return {QString(32, 'c'), QString(64, 'a'), 1};
  }
  void reply(
      const DNS_SERVICE_RESOLVE_REQUEST &q, int iface = 7, const QString &host = "pc.local", int port = 34800,
      const char *address = "192.168.50.20"
  )
  {
    auto wide = host.toStdWString();
    IP4_ADDRESS ip = 0;
    inet_pton(AF_INET, address, &ip);
    DNS_SERVICE_INSTANCE i{};
    i.dwInterfaceIndex = iface;
    i.pszHostName = wide.data();
    i.wPort = port;
    i.ip4Address = &ip;
    q.pResolveCompletionCallback(0, q.pQueryContext, &i);
  }
private Q_SLOTS:
  void explicitServiceAndInterfaceOnly()
  {
    std::unique_ptr<ScopedAddressResolver> r(createScopedAddressResolver(
        nullptr, [&] { return QVector<DiscoveryScope>{scope}; },
        [](const QString &, int) { return QStringList{"deskflow-test._deskflow._tcp.local"}; }
    ));
    QSignalSpy found(r.get(), &ScopedAddressResolver::addresses);
    r->start(context(), scope, "pc.local", 34800);
    const auto q = fixture::requests.back();
    QCOMPARE(q.InterfaceIndex, 7u);
    QCOMPARE(QString::fromWCharArray(q.QueryName), QString("deskflow-test._deskflow._tcp.local"));
    reply(q);
    QTRY_COMPARE(found.count(), 1);
    QCOMPARE(found.at(0).at(3).toStringList(), QStringList{"192.168.50.20"});
  }
  void wrongReply_data()
  {
    QTest::addColumn<QString>("kind");
    for (const auto *s : {"interface", "host", "port", "public", "off-link"})
      QTest::newRow(s) << QString(s);
  }
  void wrongReply()
  {
    QFETCH(QString, kind);
    std::unique_ptr<ScopedAddressResolver> r(createScopedAddressResolver(
        nullptr, [&] { return QVector<DiscoveryScope>{scope}; },
        [](const QString &, int) { return QStringList{"fixture._deskflow._tcp.local"}; }
    ));
    QSignalSpy found(r.get(), &ScopedAddressResolver::addresses), failed(r.get(), &ScopedAddressResolver::failed);
    r->start(context(), scope, "pc.local", 34800);
    reply(
        fixture::requests.back(), kind == "interface" ? 8 : 7, kind == "host" ? "evil.local" : "pc.local",
        kind == "port" ? 24800 : 34800,
        kind == "public"     ? "8.8.8.8"
        : kind == "off-link" ? "10.0.0.2"
                             : "192.168.50.20"
    );
    QTRY_COMPARE(failed.count(), 1);
    QCOMPARE(found.count(), 0);
  }
  void noServiceDoesNotFallbackToDns()
  {
    const auto before = fixture::requests.size();
    std::unique_ptr<ScopedAddressResolver> r(
        createScopedAddressResolver(nullptr, [&] { return QVector<DiscoveryScope>{scope}; }, {})
    );
    QSignalSpy failed(r.get(), &ScopedAddressResolver::failed);
    r->start(context(), scope, "pc.local", 34800);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(fixture::requests.size(), before);
  }
  void cancelledCallbackCannotEnterNewSession()
  {
    std::unique_ptr<ScopedAddressResolver> r(createScopedAddressResolver(
        nullptr, [&] { return QVector<DiscoveryScope>{scope}; },
        [](const QString &, int) { return QStringList{"fixture._deskflow._tcp.local"}; }
    ));
    QSignalSpy found(r.get(), &ScopedAddressResolver::addresses);
    r->start(context(), scope, "pc.local", 34800);
    const auto old = fixture::requests.back();
    r->cancel();
    auto c = context();
    c.generation = 2;
    r->start(c, scope, "pc.local", 34800);
    reply(old);
    QTest::qWait(110);
    QCOMPARE(found.count(), 0);
    reply(fixture::requests.back());
    QTRY_COMPARE(found.count(), 1);
    QVERIFY(fixture::cancelled > 0);
  }
};
QTEST_GUILESS_MAIN(ScopedAddressWindowsTests)
#include "ScopedAddressWindowsTests.moc"
