// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DiscoveryCatalog.h"
#include "DiscoveryTransport.h"
#ifndef Q_OS_WIN
#include <QElapsedTimer>
#include <QSocketNotifier>
#include <QTimer>
#include <map>
#include <memory>
#ifdef Q_OS_MACOS
#include <arpa/inet.h>
#include <cstring>
#include <dns_sd.h>
#endif
namespace deskflow::gui {
#ifdef Q_OS_MACOS
namespace {
QString boundedUtf8(const char *value, size_t max)
{
  if (!value)
    return {};
  const auto size = strnlen(value, max + 1);
  if (size > max)
    return {};
  const QByteArray bytes(value, size);
  const auto text = QString::fromUtf8(bytes);
  return text.toUtf8() == bytes ? text : QString();
}
class MacDiscoveryTransport final : public DiscoveryTransport
{
  struct Query
  {
    MacDiscoveryTransport *owner = nullptr;
    DNSServiceRef ref = nullptr;
    QSocketNotifier *notifier = nullptr;
    QString key, name;
    bool done = false;
    QElapsedTimer age;
    ~Query()
    {
      delete notifier;
      if (ref)
        DNSServiceRefDeallocate(ref);
    }
  };
  struct Service
  {
    QByteArray name;
    quint32 iface = 0;
    qint64 lastAttempt = -30000, lastSuccess = 0;
  };
  std::unique_ptr<Query> m_browser, m_advertisement, m_hostRecord;
  DiscoveryScope m_scope;
  QElapsedTimer m_session;
  QTimer m_deadline;
  quint64 m_epoch = 0;
  std::map<QString, std::unique_ptr<Query>> m_queries;
  QMap<QString, Service> m_services;
  QTimer m_refresh;
  QElapsedTimer m_clock;
  void watch(Query *q, bool oneShot)
  {
    q->notifier = new QSocketNotifier(DNSServiceRefSockFD(q->ref), QSocketNotifier::Read, this);
    connect(q->notifier, &QSocketNotifier::activated, this, [this, q, oneShot] {
      const auto result = DNSServiceProcessResult(q->ref);
      if (result != kDNSServiceErr_NoError) {
        stop();
        Q_EMIT status(tr("Local discovery stopped. Check local network access and refresh."));
        Q_EMIT sessionEnded();
        return;
      }
      if (oneShot && q->done) {
        const auto key = q->key;
        m_queries.erase(key);
      }
    });
  }
  void resolve(const QString &key)
  {
    if (!scopeAvailable(m_scope))
      return;
    if (m_queries.size() >= 16 || m_queries.count(key) || !m_services.contains(key))
      return;
    auto &service = m_services[key];
    if (m_clock.elapsed() - service.lastAttempt < 30000)
      return;
    service.lastAttempt = m_clock.elapsed();
    auto q = std::make_unique<Query>();
    q->owner = this;
    q->key = key;
    q->name = QString::fromUtf8(service.name);
    q->age.start();
    const auto result = DNSServiceResolve(
        &q->ref, 0, service.iface, service.name.constData(), "_deskflow._tcp", "local.",
        &MacDiscoveryTransport::resolved, q.get()
    );
    if (result != kDNSServiceErr_NoError)
      return;
    watch(q.get(), true);
    m_queries.emplace(key, std::move(q));
  }
  static void resolved(
      DNSServiceRef, DNSServiceFlags, uint32_t iface, DNSServiceErrorType error, const char *, const char *host,
      uint16_t port, uint16_t, const unsigned char *, void *context
  )
  {
    auto *q = static_cast<Query *>(context);
    q->done = true;
    const auto address = boundedUtf8(host, 254);
    if (error || iface != q->owner->m_scope.index || !q->owner->scopeAvailable(q->owner->m_scope) ||
        !DiscoveryCatalog::validEndpoint(address, ntohs(port)))
      return;
    if (q->owner->m_services.contains(q->key))
      q->owner->m_services[q->key].lastSuccess = q->owner->m_clock.elapsed();
    Q_EMIT q->owner->DiscoveryTransport::resolved(q->key, q->name, address, ntohs(port), 120000);
    // TXT (including any identity/trust claims) deliberately never enters the model.
  }
  static void browsed(
      DNSServiceRef, DNSServiceFlags flags, uint32_t iface, DNSServiceErrorType error, const char *name,
      const char *type, const char *domain, void *context
  )
  {
    auto *self = static_cast<MacDiscoveryTransport *>(context);
    if (error) {
      self->endSoon(tr("Nearby discovery is unavailable."));
      return;
    }
    if (iface != self->m_scope.index || !self->scopeAvailable(self->m_scope))
      return;
    const auto display = boundedUtf8(name, 63);
    if (!DiscoveryCatalog::validName(display) || boundedUtf8(domain, 16) != "local." ||
        boundedUtf8(type, 32) != "_deskflow._tcp.")
      return;
    const auto key = QString::number(iface) + ":" + display;
    if (!(flags & kDNSServiceFlagsAdd)) {
      self->m_queries.erase(key);
      self->m_services.remove(key);
      Q_EMIT self->removed(key);
      return;
    }
    if (!self->m_services.contains(key) && self->m_services.size() >= 64)
      return;
    if (!self->m_services.contains(key))
      self->m_services[key] = {display.toUtf8(), iface, -30000, self->m_clock.elapsed()};
    self->resolve(key);
  }
  void endSoon(const QString &message)
  {
    // DNS callbacks must return before their references are deallocated.
    const auto epoch = m_epoch;
    QTimer::singleShot(0, this, [this, message, epoch] {
      if (epoch != m_epoch)
        return;
      stop();
      Q_EMIT status(message);
      Q_EMIT sessionEnded();
    });
  }
  static void hostRegistered(DNSServiceRef, DNSRecordRef, DNSServiceFlags, DNSServiceErrorType error, void *context)
  {
    if (error)
      static_cast<MacDiscoveryTransport *>(context)->endSoon(
          tr("Could not advertise this server. Check local network access.")
      );
  }
  static void registered(
      DNSServiceRef, DNSServiceFlags, DNSServiceErrorType error, const char *, const char *, const char *, void *context
  )
  {
    auto *self = static_cast<MacDiscoveryTransport *>(context);
    if (error || !self->scopeAvailable(self->m_scope)) {
      self->endSoon(tr("Could not advertise this server. Check local network access."));
      return;
    }
    Q_EMIT self->activeChanged(true);
    Q_EMIT self->status(tr("Discoverable on the selected network. Fingerprint confirmation is still required."));
  }

public:
  explicit MacDiscoveryTransport(QObject *parent, DiscoveryScopeSource source)
      : DiscoveryTransport(parent, std::move(source))
  {
    m_deadline.setParent(this);
    m_deadline.setObjectName("transportDeadline");
    m_deadline.setSingleShot(true);
    m_deadline.setTimerType(Qt::PreciseTimer);
    m_deadline.setInterval(kDiscoverySessionMs);
    connect(&m_deadline, &QTimer::timeout, this, [this] {
      stop();
      Q_EMIT status(tr("Discovery stopped after 5 minutes. Turn it on again to continue."));
      Q_EMIT sessionEnded();
    });
    m_clock.start();
    m_refresh.setInterval(1000);
    connect(&m_refresh, &QTimer::timeout, this, [this] {
      if (!scopeAvailable(m_scope) || m_session.elapsed() >= kDiscoverySessionMs) {
        stop();
        Q_EMIT status(tr("Discovery stopped. Select a local network and start again."));
        Q_EMIT sessionEnded();
        return;
      }
      for (auto it = m_queries.begin(); it != m_queries.end();) {
        if (it->second->age.elapsed() >= 10000)
          it = m_queries.erase(it);
        else
          ++it;
      }
      for (const auto &key : m_services.keys()) {
        if (m_clock.elapsed() - m_services[key].lastSuccess >= 150000) {
          m_queries.erase(key);
          m_services.remove(key);
          Q_EMIT removed(key);
        } else
          resolve(key);
      }
    });
  }
  ~MacDiscoveryTransport() override
  {
    stop();
  }
  void browse(const DiscoveryScope &scope) override
  {
    stop();
    if (!scopeAvailable(scope)) {
      Q_EMIT status(tr("Select an available private Wi-Fi or Ethernet network."));
      Q_EMIT sessionEnded();
      return;
    }
#ifdef DESKFLOW_LAB_BUILD
    Q_EMIT status(tr("LAN discovery is disabled in preview builds."));
    Q_EMIT sessionEnded();
    return;
#endif
    m_scope = scope;
    m_session.start();
    m_deadline.start();
    m_browser = std::make_unique<Query>();
    const auto result = DNSServiceBrowse(
        &m_browser->ref, 0, m_scope.index, "_deskflow._tcp", "local.", &MacDiscoveryTransport::browsed, this
    );
    if (result != kDNSServiceErr_NoError) {
      stop();
      Q_EMIT status(tr("Nearby discovery is unavailable."));
      Q_EMIT sessionEnded();
      return;
    }
    watch(m_browser.get(), false);
    m_refresh.start();
    Q_EMIT activeChanged(true);
    Q_EMIT status(tr("Searching the selected network for up to 5 minutes. Devices are not trusted automatically."));
  }
  void advertise(const DiscoveryAdvertisement &ad) override
  {
    stop();
    if (!ad.valid() || !scopeAvailable(ad.scope)) {
      Q_EMIT status(tr("Select an available private Wi-Fi or Ethernet network."));
      Q_EMIT sessionEnded();
      return;
    }
#ifdef DESKFLOW_LAB_BUILD
    Q_EMIT status(tr("LAN advertising is disabled in preview builds."));
    Q_EMIT sessionEnded();
    return;
#endif
    m_scope = ad.scope;
    m_session.start();
    m_deadline.start();
    m_hostRecord = std::make_unique<Query>();
    DNSRecordRef record = nullptr;
    const auto host = ad.host().toUtf8();
    const auto ipv4 = htonl(m_scope.address.toIPv4Address());
    auto result = DNSServiceCreateConnection(&m_hostRecord->ref);
    if (result == kDNSServiceErr_NoError)
      result = DNSServiceRegisterRecord(
          m_hostRecord->ref, &record, kDNSServiceFlagsUnique, m_scope.index, host.constData(), kDNSServiceType_A,
          kDNSServiceClass_IN, sizeof(ipv4), &ipv4, 120, &MacDiscoveryTransport::hostRegistered, this
      );
    if (result != kDNSServiceErr_NoError) {
      stop();
      Q_EMIT status(tr("Could not advertise this server. Check local network access."));
      Q_EMIT sessionEnded();
      return;
    }
    watch(m_hostRecord.get(), false);
    m_advertisement = std::make_unique<Query>();
    const auto name = ad.instance().toUtf8();
    // Never let the system substitute its machine/account-derived host name.
    result = DNSServiceRegister(
        &m_advertisement->ref, kDNSServiceFlagsNoAutoRename, m_scope.index, name.constData(), "_deskflow._tcp",
        "local.", host.constData(), htons(static_cast<uint16_t>(ad.port)), 0, nullptr,
        &MacDiscoveryTransport::registered, this
    );
    if (result != kDNSServiceErr_NoError) {
      stop();
      Q_EMIT status(tr("Could not advertise this server. Check local network access."));
      Q_EMIT sessionEnded();
      return;
    }
    watch(m_advertisement.get(), false);
    m_refresh.start();
  }
  void stop() override
  {
    ++m_epoch;
    m_deadline.stop();
    m_refresh.stop();
    m_queries.clear();
    m_services.clear();
    m_browser.reset();
    m_advertisement.reset();
    m_hostRecord.reset();
    m_scope = {};
    m_session.invalidate();
    Q_EMIT activeChanged(false);
  }
};
} // namespace
DiscoveryTransport *createDiscoveryTransport(QObject *parent, DiscoveryScopeSource source)
{
  return new MacDiscoveryTransport(parent, std::move(source));
}
#else
class UnsupportedDiscoveryTransport final : public DiscoveryTransport
{
public:
  using DiscoveryTransport::DiscoveryTransport;
  void browse(const DiscoveryScope &) override
  {
    Q_EMIT status(tr("Nearby discovery is not available on this platform."));
    Q_EMIT sessionEnded();
  }
  void advertise(const DiscoveryAdvertisement &) override
  {
    browse({});
  }
  void stop() override
  {
    Q_EMIT activeChanged(false);
  }
};
DiscoveryTransport *createDiscoveryTransport(QObject *parent, DiscoveryScopeSource source)
{
  return new UnsupportedDiscoveryTransport(parent, std::move(source));
}
#endif
} // namespace deskflow::gui
#endif
