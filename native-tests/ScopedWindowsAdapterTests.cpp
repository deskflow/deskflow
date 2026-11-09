// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Compiles the actual Windows adapter against a documented API-shape fixture on
// macOS. No Windows SDK/ABI, native DLL, network, firewall or Windows execution.
#include "DnsApiTestShim.h"
#include "gui/core/DiscoveryTransport.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <memory>
using namespace deskflow::gui;
namespace {
struct Call
{
  QString kind, name, host;
  quint32 iface = 0;
  int port = 0, properties = 0;
  bool unicast = false;
  quint32 ipv4 = 0;
};
QVector<Call> calls;
QVector<DNS_SERVICE_BROWSE_REQUEST> browses;
QVector<DNS_SERVICE_RESOLVE_REQUEST> resolves;
int instances = 0;
wchar_t *copy(PCWSTR s)
{
  if (!s)
    return nullptr;
  const auto len = std::wcslen(s);
  auto *p = new wchar_t[len + 1];
  std::copy_n(s, len + 1, p);
  return p;
}
DiscoveryScope scope(quint32 index = 7)
{
  return {index, "Ethernet", QHostAddress("192.168.50.10"), 24};
}
DNS_RECORD *ptr(PCWSTR name, DWORD ttl = 120)
{
  auto *r = new DNS_RECORD;
  r->wType = DNS_TYPE_PTR;
  r->dwTtl = ttl;
  r->Data.PTR.pNameHost = copy(name);
  return r;
}
void candidate(int n, PCWSTR name = L"PC._deskflow._tcp.local", DWORD ttl = 120)
{
  auto b = browses[n];
  b.pBrowseCallback(0, b.pQueryContext, ptr(name, ttl));
}
PDNS_SERVICE_INSTANCE result(quint32 iface = 7)
{
  auto *i = DnsServiceConstructInstance(
      L"PC._deskflow._tcp.local", L"pc.local", nullptr, nullptr, 34800, 0, 0, 0, nullptr, nullptr
  );
  i->dwInterfaceIndex = iface;
  return i;
}
} // namespace
PDNS_SERVICE_INSTANCE DnsServiceConstructInstance(
    PCWSTR name, PCWSTR host, IP4_ADDRESS *ip4, IP6_ADDRESS *, WORD port, WORD priority, WORD weight, DWORD properties,
    PCWSTR *, PCWSTR *
)
{
  auto *i = new DNS_SERVICE_INSTANCE;
  ++instances;
  i->pszInstanceName = copy(name);
  i->pszHostName = copy(host);
  if (ip4)
    i->ip4Address = new IP4_ADDRESS(*ip4);
  i->wPort = port;
  i->wPriority = priority;
  i->wWeight = weight;
  i->dwPropertyCount = properties;
  return i;
}
void DnsServiceFreeInstance(PDNS_SERVICE_INSTANCE i)
{
  if (!i)
    return;
  delete[] i->pszInstanceName;
  delete[] i->pszHostName;
  delete i->ip4Address;
  delete i;
  --instances;
}
void DnsRecordListFree(PDNS_RECORD r, int)
{
  while (r) {
    auto *n = r->pNext;
    delete[] r->Data.PTR.pNameHost;
    delete r;
    r = n;
  }
}
DWORD DnsServiceBrowse(DNS_SERVICE_BROWSE_REQUEST *r, DNS_SERVICE_CANCEL *cancel)
{
  browses.push_back(*r);
  cancel->reserved = r;
  calls.push_back({"browse", QString::fromWCharArray(r->QueryName), {}, r->InterfaceIndex});
  return DNS_REQUEST_PENDING;
}
DWORD DnsServiceBrowseCancel(DNS_SERVICE_CANCEL *cancel)
{
  const auto r = *static_cast<DNS_SERVICE_BROWSE_REQUEST *>(cancel->reserved);
  QTimer::singleShot(0, [r] { r.pBrowseCallback(ERROR_CANCELLED, r.pQueryContext, nullptr); });
  return ERROR_SUCCESS;
}
DWORD DnsServiceResolve(DNS_SERVICE_RESOLVE_REQUEST *r, DNS_SERVICE_CANCEL *cancel)
{
  resolves.push_back(*r);
  cancel->reserved = r;
  calls.push_back({"resolve", QString::fromWCharArray(r->QueryName), {}, r->InterfaceIndex});
  return DNS_REQUEST_PENDING;
}
DWORD DnsServiceResolveCancel(DNS_SERVICE_CANCEL *cancel)
{
  const auto r = *static_cast<DNS_SERVICE_RESOLVE_REQUEST *>(cancel->reserved);
  QTimer::singleShot(0, [r] { r.pResolveCompletionCallback(ERROR_CANCELLED, r.pQueryContext, nullptr); });
  return ERROR_SUCCESS;
}
DWORD DnsServiceRegister(DNS_SERVICE_REGISTER_REQUEST *r, DNS_SERVICE_CANCEL *)
{
  auto *i = r->pServiceInstance;
  calls.push_back(
      {"register", QString::fromWCharArray(i->pszInstanceName), QString::fromWCharArray(i->pszHostName),
       r->InterfaceIndex, i->wPort, int(i->dwPropertyCount), bool(r->unicastEnabled),
       i->ip4Address ? *i->ip4Address : 0}
  );
  Q_ASSERT(i->dwInterfaceIndex == r->InterfaceIndex);
  const auto request = *r;
  QTimer::singleShot(0, [request] {
    request.pRegisterCompletionCallback(0, request.pQueryContext, result(request.InterfaceIndex));
  });
  return DNS_REQUEST_PENDING;
}
DWORD DnsServiceDeRegister(DNS_SERVICE_REGISTER_REQUEST *r, DNS_SERVICE_CANCEL *)
{
  calls.push_back({"withdraw", {}, {}, r->InterfaceIndex});
  const auto request = *r;
  QTimer::singleShot(0, [request] { request.pRegisterCompletionCallback(0, request.pQueryContext, nullptr); });
  return DNS_REQUEST_PENDING;
}
class ScopedWindowsAdapterTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void init()
  {
    calls.clear();
    browses.clear();
    resolves.clear();
    QCOMPARE(instances, 0);
  }
  void cleanup()
  {
    QTest::qWait(20);
    QCOMPARE(instances, 0);
  }
  void noDefaultOrUnavailableInterfaceRequests()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    t->browse({});
    t->browse(scope(8));
    t->advertise({{}, DiscoveryAdvertisement::newToken(), 34800});
    QCOMPARE(calls.size(), 0);
  }
  void browseAndResolvePassExactSelectedInterface()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    t->browse(scope());
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].iface, quint32(7));
    QCOMPARE(calls[0].name, QString("_deskflow._tcp.local"));
    candidate(0);
    QTRY_COMPARE(resolves.size(), 1);
    QCOMPARE(calls[1].iface, quint32(7));
  }
  void wrongInterfaceResolvedResponseIsRejected()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    QSignalSpy found(t.get(), &DiscoveryTransport::resolved);
    t->browse(scope());
    candidate(0);
    QTRY_COMPARE(resolves.size(), 1);
    auto r = resolves[0];
    r.pResolveCompletionCallback(0, r.pQueryContext, result(8));
    QTest::qWait(300);
    QCOMPARE(found.count(), 0);
  }
  void validResponseAndGoodbyeAreDelivered()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    QSignalSpy found(t.get(), &DiscoveryTransport::resolved), removed(t.get(), &DiscoveryTransport::removed);
    t->browse(scope());
    candidate(0);
    QTRY_COMPARE(resolves.size(), 1);
    auto r = resolves[0];
    r.pResolveCompletionCallback(0, r.pQueryContext, result());
    QTRY_COMPARE(found.count(), 1);
    candidate(0, L"PC._deskflow._tcp.local", 0);
    QTRY_COMPARE(removed.count(), 1);
  }
  void malformedAndNonLocalRecordsNeverResolve()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    t->browse(scope());
    candidate(0, L"PC._deskflow._tcp.example.com");
    candidate(0, L"bad\nname._deskflow._tcp.local");
    QTest::qWait(300);
    QCOMPARE(resolves.size(), 0);
  }
  void randomAliasPrivateAddressEmptyTxtAndMulticastOnly()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    const DiscoveryAdvertisement ad{scope(), DiscoveryAdvertisement::newToken(), 34800};
    t->advertise(ad);
    QCOMPARE(calls.size(), 1);
    const auto c = calls[0];
    QCOMPARE(c.iface, quint32(7));
    QCOMPARE(c.host, ad.host());
    QCOMPARE(c.name, ad.instance() + "._deskflow._tcp.local");
    QCOMPARE(c.ipv4, quint32(htonl(scope().address.toIPv4Address())));
    QCOMPARE(c.port, 34800);
    QCOMPARE(c.properties, 0);
    QVERIFY(!c.unicast);
    QTest::qWait(10);
    t->stop();
    QTRY_COMPARE(calls.size(), 2);
    QCOMPARE(calls[1].kind, QString("withdraw"));
  }
  void stopBeforeRegisterCompletionWithdrawsLater()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    t->advertise({scope(), DiscoveryAdvertisement::newToken(), 34800});
    t->stop();
    QTRY_COMPARE(calls.size(), 2);
    QCOMPARE(calls[1].kind, QString("withdraw"));
  }
  void stoppedQueuedResultsCannotReachReplacementSession()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope(), scope(8)};
    }));
    QSignalSpy found(t.get(), &DiscoveryTransport::resolved);
    t->browse(scope());
    candidate(0);
    t->stop();
    t->browse(scope(8));
    QTest::qWait(300);
    QCOMPARE(resolves.size(), 0);
    QCOMPARE(found.count(), 0);
    QCOMPARE(calls.last().iface, quint32(8));
  }
  void scopeRemovalStopsWithoutFallback()
  {
    QVector<DiscoveryScope> available{scope()};
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [&] { return available; }));
    QSignalSpy ended(t.get(), &DiscoveryTransport::sessionEnded);
    t->browse(scope());
    available.clear();
    QTRY_COMPARE(ended.count(), 1);
    QCOMPARE(calls.size(), 1);
  }
  void deadlineStopsWithoutAutoRestart()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    t->browse(scope());
    auto *timer = t->findChild<QTimer *>("transportDeadline");
    QVERIFY(timer);
    QCOMPARE(timer->interval(), 300000);
    QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    QTest::qWait(300);
    QCOMPARE(calls.size(), 1);
  }
};
QTEST_GUILESS_MAIN(ScopedWindowsAdapterTests)
#include "ScopedWindowsAdapterTests.moc"
