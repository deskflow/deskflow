// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Actual scoped resolver + Apple SDK declarations; fake DNS API, no packets.
#include "gui/core/RecoveryProbe.h"
#include <QSignalSpy>
#include <QTest>
#include <arpa/inet.h>
#include <dns_sd.h>
#include <map>
#include <unistd.h>
using namespace deskflow::gui;
using namespace deskflow::auth;
namespace fixture {
struct Query
{
  DNSServiceGetAddrInfoReply callback;
  void *context;
  int fds[2];
};
std::map<DNSServiceRef, Query> queries;
quintptr sequence = 0;
quint32 iface = 0, flags = 0, protocol = 0;
QByteArray host;
DNSServiceRef latest = nullptr;
} // namespace fixture
extern "C" DNSServiceErrorType DNSServiceGetAddrInfo(
    DNSServiceRef *ref, DNSServiceFlags flags, uint32_t iface, DNSServiceProtocol protocol, const char *host,
    DNSServiceGetAddrInfoReply callback, void *context
)
{
  fixture::Query q{callback, context, {}};
  if (pipe(q.fds))
    return kDNSServiceErr_Unknown;
  *ref = reinterpret_cast<DNSServiceRef>(++fixture::sequence);
  fixture::queries[*ref] = q;
  fixture::latest = *ref;
  fixture::iface = iface;
  fixture::flags = flags;
  fixture::protocol = protocol;
  fixture::host = host;
  return kDNSServiceErr_NoError;
}
extern "C" int DNSServiceRefSockFD(DNSServiceRef ref)
{
  return fixture::queries[ref].fds[0];
}
extern "C" DNSServiceErrorType DNSServiceProcessResult(DNSServiceRef)
{
  return kDNSServiceErr_NoError;
}
extern "C" void DNSServiceRefDeallocate(DNSServiceRef ref)
{
  auto it = fixture::queries.find(ref);
  if (it != fixture::queries.end()) {
    close(it->second.fds[0]);
    close(it->second.fds[1]);
    fixture::queries.erase(it);
  }
}
class ScopedAddressMacTests : public QObject
{
  Q_OBJECT
  const DiscoveryScope scope{7, "Fixture", QHostAddress("192.168.50.10"), 24};
  Context context()
  {
    return {QString(32, 'c'), QString(64, 'a'), 1};
  }
  void reply(
      DNSServiceRef ref, const char *address, quint32 iface = 7, int ttl = 10,
      DNSServiceFlags flags = kDNSServiceFlagsAdd, const char *host = "pc.local."
  )
  {
    auto q = fixture::queries.at(ref);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    inet_pton(AF_INET, address, &a.sin_addr);
    q.callback(ref, flags, iface, 0, host, reinterpret_cast<sockaddr *>(&a), ttl, q.context);
  }
private Q_SLOTS:
  void explicitInterfaceProtocolAndPrivateNumericOnly()
  {
    std::unique_ptr<ScopedAddressResolver> r(
        createScopedAddressResolver(nullptr, [&] { return QVector<DiscoveryScope>{scope}; }, {})
    );
    QSignalSpy found(r.get(), &ScopedAddressResolver::addresses);
    r->start(context(), scope, "pc.local", 34800);
    QCOMPARE(fixture::iface, 7u);
    QCOMPARE(fixture::protocol, quint32(kDNSServiceProtocol_IPv4));
    QVERIFY(fixture::flags & kDNSServiceFlagsForceMulticast);
    QCOMPARE(fixture::host, QByteArray("pc.local."));
    reply(fixture::latest, "192.168.50.20");
    QTRY_COMPARE(found.count(), 1);
    QCOMPARE(found.at(0).at(3).toStringList(), QStringList{"192.168.50.20"});
    QVERIFY(fixture::queries.empty());
  }
  void rejectedReply_data()
  {
    QTest::addColumn<QString>("kind");
    for (const auto *s :
         {"public", "off-link", "own", "network", "broadcast", "other-interface", "wrong-host", "withdrawn", "expired",
          "mixed-invalid"})
      QTest::newRow(s) << QString(s);
  }
  void rejectedReply()
  {
    QFETCH(QString, kind);
    std::unique_ptr<ScopedAddressResolver> r(
        createScopedAddressResolver(nullptr, [&] { return QVector<DiscoveryScope>{scope}; }, {})
    );
    QSignalSpy found(r.get(), &ScopedAddressResolver::addresses), failed(r.get(), &ScopedAddressResolver::failed);
    r->start(context(), scope, "pc.local", 34800);
    const auto ref = fixture::latest;
    if (kind == "mixed-invalid")
      reply(ref, "192.168.50.20", 7, 10, kDNSServiceFlagsAdd | kDNSServiceFlagsMoreComing);
    const auto address = kind == "public" || kind == "mixed-invalid" ? "8.8.8.8"
                         : kind == "off-link"                        ? "10.0.0.2"
                         : kind == "own"                             ? "192.168.50.10"
                         : kind == "network"                         ? "192.168.50.0"
                         : kind == "broadcast"                       ? "192.168.50.255"
                                                                     : "192.168.50.20";
    reply(
        ref, address, kind == "other-interface" ? 8 : 7, kind == "expired" ? 0 : 10,
        kind == "withdrawn" ? 0 : kDNSServiceFlagsAdd, kind == "wrong-host" ? "attacker.local." : "pc.local."
    );
    QTRY_COMPARE(failed.count(), 1);
    QCOMPARE(found.count(), 0);
  }
  void cancellationAndInterfaceLossInvalidateCallbacks()
  {
    QVector<DiscoveryScope> available{scope};
    std::unique_ptr<ScopedAddressResolver> r(createScopedAddressResolver(nullptr, [&] { return available; }, {}));
    QSignalSpy found(r.get(), &ScopedAddressResolver::addresses), failed(r.get(), &ScopedAddressResolver::failed);
    r->start(context(), scope, "pc.local", 34800);
    const auto oldRef = fixture::latest;
    const auto old = fixture::queries.at(oldRef);
    r->cancel();
    auto next = context();
    next.generation = 2;
    r->start(next, scope, "pc.local", 34800);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.50.20", &address.sin_addr);
    old.callback(
        oldRef, kDNSServiceFlagsAdd, 7, 0, "pc.local.", reinterpret_cast<sockaddr *>(&address), 10, old.context
    );
    QCOMPARE(found.count(), 0);
    available.clear();
    reply(fixture::latest, "192.168.50.20");
    QTRY_COMPARE(failed.count(), 1);
    QCOMPARE(found.count(), 0);
  }
  void multipleAddressesDeduplicatedAndLeaseBounded()
  {
    std::unique_ptr<ScopedAddressResolver> r(
        createScopedAddressResolver(nullptr, [&] { return QVector<DiscoveryScope>{scope}; }, {})
    );
    QSignalSpy found(r.get(), &ScopedAddressResolver::addresses);
    r->start(context(), scope, "pc.local", 34800);
    const auto ref = fixture::latest;
    reply(ref, "192.168.50.20", 7, 999, kDNSServiceFlagsAdd | kDNSServiceFlagsMoreComing);
    reply(ref, "192.168.50.20", 7, 7, kDNSServiceFlagsAdd | kDNSServiceFlagsMoreComing);
    reply(ref, "192.168.50.30", 7, 3);
    QTRY_COMPARE(found.count(), 1);
    QCOMPARE(found.at(0).at(3).toStringList().size(), 2);
    QCOMPARE(found.at(0).at(4).toInt(), 3000);
  }
};
QTEST_GUILESS_MAIN(ScopedAddressMacTests)
#include "ScopedAddressMacTests.moc"
