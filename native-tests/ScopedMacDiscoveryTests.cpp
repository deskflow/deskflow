// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Real transport + Apple SDK declarations. Every DNS-SD function is replaced
// here with an in-process pipe fixture: no mDNS daemon / network is contacted.
#include "gui/core/DiscoveryTransport.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <arpa/inet.h>
#include <deque>
#include <dns_sd.h>
#include <memory>
#include <unistd.h>
using namespace deskflow::gui;
namespace {
struct Request
{
  enum Kind
  {
    Browse,
    Resolve,
    Register,
    Host
  } kind;
  quint32 iface;
  QString name, type, domain, host;
  int port = 0, txtLength = 0, rrtype = 0, rrclass = 0, ttl = 0;
  QByteArray data;
  DNSServiceFlags flags = 0;
};
struct Ref
{
  int pipefd[2]{-1, -1};
  bool alive = true;
  void *context = nullptr;
  DNSServiceBrowseReply browse = nullptr;
  DNSServiceResolveReply resolve = nullptr;
  DNSServiceRegisterReply registration = nullptr;
  DNSServiceRegisterRecordReply host = nullptr;
  std::deque<std::function<void()>> events;
  Ref()
  {
    if (::pipe(pipefd))
      qFatal("Unable to create fixture pipe");
  }
  ~Ref()
  {
    if (pipefd[0] >= 0)
      ::close(pipefd[0]);
    if (pipefd[1] >= 0)
      ::close(pipefd[1]);
  }
};
QVector<Request> requests;
std::vector<std::unique_ptr<Ref>> refs;
Ref *ref(DNSServiceRef r)
{
  return reinterpret_cast<Ref *>(r);
}
DNSServiceRef handle(Ref *r)
{
  return reinterpret_cast<DNSServiceRef>(r);
}
DNSServiceErrorType make(DNSServiceRef *out)
{
  refs.push_back(std::make_unique<Ref>());
  *out = handle(refs.back().get());
  return kDNSServiceErr_NoError;
}
void enqueue(Ref *r, std::function<void()> fn)
{
  QVERIFY(r->alive);
  r->events.push_back(std::move(fn));
  QCOMPARE(::write(r->pipefd[1], "x", 1), ssize_t(1));
}
Ref *lastBrowse()
{
  for (auto i = refs.rbegin(); i != refs.rend(); ++i)
    if ((*i)->alive && (*i)->browse)
      return i->get();
  return nullptr;
}
Ref *lastResolve()
{
  for (auto i = refs.rbegin(); i != refs.rend(); ++i)
    if ((*i)->alive && (*i)->resolve)
      return i->get();
  return nullptr;
}
DiscoveryScope scope(quint32 index = 7)
{
  return {index, "en7", QHostAddress("192.168.50.10"), 24};
}
} // namespace
extern "C"
{
  DNSServiceErrorType DNSSD_API DNSServiceCreateConnection(DNSServiceRef *out)
  {
    return make(out);
  }
  DNSServiceErrorType DNSSD_API DNSServiceBrowse(
      DNSServiceRef *out, DNSServiceFlags flags, uint32_t index, const char *type, const char *domain,
      DNSServiceBrowseReply cb, void *context
  )
  {
    make(out);
    ref(*out)->browse = cb;
    ref(*out)->context = context;
    requests.push_back({Request::Browse, index, {}, type, domain, {}});
    requests.last().flags = flags;
    return 0;
  }
  DNSServiceErrorType DNSSD_API DNSServiceResolve(
      DNSServiceRef *out, DNSServiceFlags flags, uint32_t index, const char *name, const char *type, const char *domain,
      DNSServiceResolveReply cb, void *context
  )
  {
    make(out);
    ref(*out)->resolve = cb;
    ref(*out)->context = context;
    requests.push_back({Request::Resolve, index, name, type, domain, {}});
    requests.last().flags = flags;
    return 0;
  }
  DNSServiceErrorType DNSSD_API DNSServiceRegister(
      DNSServiceRef *out, DNSServiceFlags flags, uint32_t index, const char *name, const char *type, const char *domain,
      const char *host, uint16_t port, uint16_t txtLength, const void *, DNSServiceRegisterReply cb, void *context
  )
  {
    make(out);
    ref(*out)->registration = cb;
    ref(*out)->context = context;
    requests.push_back({Request::Register, index, name, type, domain, host});
    auto &r = requests.last();
    r.port = ntohs(port);
    r.txtLength = txtLength;
    r.flags = flags;
    return 0;
  }
  DNSServiceErrorType DNSSD_API DNSServiceRegisterRecord(
      DNSServiceRef h, DNSRecordRef *record, DNSServiceFlags flags, uint32_t index, const char *host, uint16_t type,
      uint16_t cls, uint16_t size, const void *data, uint32_t ttl, DNSServiceRegisterRecordReply cb, void *context
  )
  {
    ref(h)->host = cb;
    ref(h)->context = context;
    *record = reinterpret_cast<DNSRecordRef>(h);
    requests.push_back({Request::Host, index, {}, {}, {}, host});
    auto &r = requests.last();
    r.flags = flags;
    r.rrtype = type;
    r.rrclass = cls;
    r.ttl = ttl;
    r.data = QByteArray(static_cast<const char *>(data), size);
    return 0;
  }
  int DNSSD_API DNSServiceRefSockFD(DNSServiceRef h)
  {
    return ref(h)->pipefd[0];
  }
  DNSServiceErrorType DNSSD_API DNSServiceProcessResult(DNSServiceRef h)
  {
    auto *r = ref(h);
    char c;
    if (::read(r->pipefd[0], &c, 1) != 1 || r->events.empty())
      return kDNSServiceErr_Unknown;
    auto event = std::move(r->events.front());
    r->events.pop_front();
    event();
    return 0;
  }
  void DNSSD_API DNSServiceRefDeallocate(DNSServiceRef h)
  {
    auto *r = ref(h);
    r->alive = false;
    r->events.clear();
    for (auto &fd : r->pipefd) {
      if (fd >= 0)
        ::close(fd);
      fd = -1;
    }
  }
}
class ScopedMacDiscoveryTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void init()
  {
    requests.clear();
    refs.clear();
  }
  void cleanup()
  {
    for (auto &r : refs)
      QVERIFY(!r->alive);
    refs.clear();
  }
  void unspecifiedUnavailableAndPublicInterfacesMakeNoRequests()
  {
    QVector<DiscoveryScope> available{scope()};
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [&] { return available; }));
    t->browse({});
    t->browse(scope(8));
    auto publicScope = scope();
    publicScope.address = QHostAddress("1.1.1.1");
    t->browse(publicScope);
    t->advertise({{}, DiscoveryAdvertisement::newToken(), 34800});
    QCOMPARE(requests.size(), 0);
  }
  void browseResolveAndCallbacksStayOnSelectedInterface()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    QSignalSpy found(t.get(), &DiscoveryTransport::resolved);
    t->browse(scope());
    QCOMPARE(requests.size(), 1);
    auto *b = lastBrowse();
    QVERIFY(b);
    QCOMPARE(requests[0].iface, quint32(7));
    QCOMPARE(requests[0].domain, QString("local."));
    QCOMPARE(requests[0].type, QString("_deskflow._tcp"));
    enqueue(b, [b] { b->browse(handle(b), kDNSServiceFlagsAdd, 8, 0, "PC", "_deskflow._tcp.", "local.", b->context); });
    QTest::qWait(10);
    QCOMPARE(requests.size(), 1);
    enqueue(b, [b] {
      b->browse(handle(b), kDNSServiceFlagsAdd, 7, 0, "PC", "_deskflow._tcp.", "example.com.", b->context);
    });
    QTest::qWait(10);
    QCOMPARE(requests.size(), 1);
    enqueue(b, [b] { b->browse(handle(b), kDNSServiceFlagsAdd, 7, 0, "PC", "_deskflow._tcp.", "local.", b->context); });
    QTRY_COMPARE(requests.size(), 2);
    QCOMPARE(requests[1].iface, quint32(7));
    auto *q = lastResolve();
    QVERIFY(q);
    enqueue(q, [q] { q->resolve(handle(q), 0, 8, 0, "PC", "pc.local", htons(34800), 0, nullptr, q->context); });
    QTest::qWait(10);
    QCOMPARE(found.count(), 0);
    t->stop();
  }
  void validResolvedEndpointAndGoodbyeAreDelivered()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    QSignalSpy found(t.get(), &DiscoveryTransport::resolved), removed(t.get(), &DiscoveryTransport::removed);
    t->browse(scope());
    auto *b = lastBrowse();
    enqueue(b, [b] { b->browse(handle(b), kDNSServiceFlagsAdd, 7, 0, "PC", "_deskflow._tcp.", "local.", b->context); });
    QTRY_VERIFY(lastResolve());
    auto *q = lastResolve();
    enqueue(q, [q] { q->resolve(handle(q), 0, 7, 0, "PC", "pc.local", htons(34800), 0, nullptr, q->context); });
    QTRY_COMPARE(found.count(), 1);
    QCOMPARE(found[0][2].toString(), QString("pc.local"));
    enqueue(b, [b] { b->browse(handle(b), 0, 7, 0, "PC", "_deskflow._tcp.", "local.", b->context); });
    QTRY_COMPARE(removed.count(), 1);
  }
  void advertisementContainsOnlyRandomAliasPrivateAddressAndPort()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    const DiscoveryAdvertisement ad{scope(), DiscoveryAdvertisement::newToken(), 34800};
    t->advertise(ad);
    QCOMPARE(requests.size(), 2);
    const auto host = requests[0], service = requests[1];
    QCOMPARE(host.kind, Request::Host);
    QCOMPARE(host.iface, quint32(7));
    QCOMPARE(host.host, ad.host());
    QCOMPARE(host.rrtype, int(kDNSServiceType_A));
    QCOMPARE(host.rrclass, int(kDNSServiceClass_IN));
    QCOMPARE(host.ttl, 120);
    const auto address = htonl(scope().address.toIPv4Address());
    QCOMPARE(host.data, QByteArray(reinterpret_cast<const char *>(&address), 4));
    QCOMPARE(service.kind, Request::Register);
    QCOMPARE(service.iface, quint32(7));
    QCOMPARE(service.name, ad.instance());
    QCOMPARE(service.host, ad.host());
    QCOMPARE(service.port, 34800);
    QCOMPARE(service.txtLength, 0);
    QCOMPARE(service.domain, QString("local."));
    QVERIFY(service.flags & kDNSServiceFlagsNoAutoRename);
  }
  void networkRemovalStopsAndNeverFallsBackToAllInterfaces()
  {
    QVector<DiscoveryScope> available{scope()};
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [&] { return available; }));
    QSignalSpy ended(t.get(), &DiscoveryTransport::sessionEnded);
    t->browse(scope());
    available.clear();
    auto *b = lastBrowse();
    enqueue(b, [b] { b->browse(handle(b), kDNSServiceFlagsAdd, 7, 0, "PC", "_deskflow._tcp.", "local.", b->context); });
    QTRY_COMPARE_WITH_TIMEOUT(ended.count(), 1, 2000);
    QCOMPARE(requests.size(), 1);
    QVERIFY(!b->alive);
  }
  void deadlineWithdrawsAndDoesNotRestart()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    t->advertise({scope(), DiscoveryAdvertisement::newToken(), 34800});
    auto *timer = t->findChild<QTimer *>("transportDeadline");
    QVERIFY(timer);
    QCOMPARE(timer->interval(), 300000);
    QVERIFY(timer->isSingleShot());
    QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    QTest::qWait(20);
    QCOMPARE(requests.size(), 2);
    for (auto &r : refs)
      QVERIFY(!r->alive);
  }
  void oldDeferredFailureCannotStopNewSession()
  {
    std::unique_ptr<DiscoveryTransport> t(createDiscoveryTransport(nullptr, [] {
      return QVector<DiscoveryScope>{scope()};
    }));
    t->browse(scope());
    auto *old = lastBrowse();
    old->browse(handle(old), 0, 7, kDNSServiceErr_Unknown, nullptr, nullptr, nullptr, old->context);
    t->browse(scope());
    auto *current = lastBrowse();
    QTest::qWait(10);
    QVERIFY(current->alive);
    QVERIFY(!old->alive);
  }
};
QTEST_GUILESS_MAIN(ScopedMacDiscoveryTests)
#include "ScopedMacDiscoveryTests.moc"
