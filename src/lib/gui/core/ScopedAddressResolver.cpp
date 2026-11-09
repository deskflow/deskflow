// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DiscoveryCatalog.h"
#include "RecoveryProbe.h"
#include "common/LabExecutionPolicy.h"
#include <QPointer>
#include <QSocketNotifier>
#include <QTimer>
#include <cstring>
#include <memory>
#include <mutex>
#if defined(Q_OS_MACOS) && !defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
#include <arpa/inet.h>
#include <dns_sd.h>
#endif
#if defined(Q_OS_WIN) || defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
#ifdef DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST
#include "DnsApiTestShim.h"
#else
#include <windns.h>
#include <windows.h>
#include <winsock2.h>
#endif
#endif
namespace deskflow::gui {
namespace {
class NativeScopedResolver final : public ScopedAddressResolver
{
  DiscoveryScopeSource m_scopes;
  ServiceNameSource m_services;
  deskflow::auth::Context m_context;
  DiscoveryScope m_scope;
  QString m_host;
  int m_port = 0;
  QTimer m_deadline, m_drain;
  quint64 m_epoch = 0;
  QStringList m_addresses;
  int m_ttl = 10000;
  bool m_invalid = false;
#if defined(Q_OS_MACOS) && !defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
  DNSServiceRef m_ref = nullptr;
  QSocketNotifier *m_notifier = nullptr;
  static void reply(
      DNSServiceRef ref, DNSServiceFlags flags, uint32_t iface, DNSServiceErrorType error, const char *host,
      const sockaddr *address, uint32_t ttl, void *raw
  )
  {
    auto *self = static_cast<NativeScopedResolver *>(raw);
    if (ref != self->m_ref)
      return;
    if (error || !host || strnlen(host, 255) > 254 || !address || address->sa_family != AF_INET || !ttl ||
        iface != self->m_scope.index || DiscoveryCatalog::canonicalHost(QString::fromUtf8(host)) != self->m_host ||
        !(flags & kDNSServiceFlagsAdd)) {
      self->finishSoon(false);
      return;
    }
    const QHostAddress numeric(ntohl(reinterpret_cast<const sockaddr_in *>(address)->sin_addr.s_addr));
    if (!self->available() || !validProbeAddress(self->m_scope, numeric)) {
      self->finishSoon(false);
      return;
    }
    if (!self->m_addresses.contains(numeric.toString()))
      self->m_addresses << numeric.toString();
    self->m_ttl = qMin(self->m_ttl, static_cast<int>(qMin<uint32_t>(ttl, 10) * 1000));
    if (self->m_addresses.size() > 8) {
      self->finishSoon(false);
      return;
    }
    if (!(flags & kDNSServiceFlagsMoreComing))
      self->finishSoon(true);
  }
#endif
#if defined(Q_OS_WIN) || defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
  struct Query : std::enable_shared_from_this<Query>
  {
    std::shared_ptr<Query> lifetime;
    DNS_SERVICE_RESOLVE_REQUEST request{};
    DNS_SERVICE_CANCEL cancel{};
    std::wstring name;
    QString expected;
    int port = 0;
    quint32 iface = 0;
    std::mutex mutex;
    bool done = false, ok = false;
    QString address;
    static VOID WINAPI reply(DWORD error, PVOID raw, PDNS_SERVICE_INSTANCE instance)
    {
      auto self = static_cast<Query *>(raw)->shared_from_this();
      QString address;
      bool ok = false;
      if (!error && instance && instance->ip4Address && instance->pszHostName &&
          instance->dwInterfaceIndex == self->iface && instance->wPort == self->port) {
        size_t n = 0;
        while (n <= 254 && instance->pszHostName[n])
          ++n;
        if (n <= 254 &&
            DiscoveryCatalog::canonicalHost(QString::fromWCharArray(instance->pszHostName, n)) == self->expected) {
          address = QHostAddress(ntohl(*instance->ip4Address)).toString();
          ok = true;
        }
      }
      if (instance)
        DnsServiceFreeInstance(instance);
      {
        std::lock_guard lock(self->mutex);
        self->address = address;
        self->ok = ok;
        self->done = true;
      }
      self->lifetime.reset();
    }
  };
  std::shared_ptr<Query> m_query;
#endif
  bool available() const
  {
    return m_context.valid() && m_scopes().contains(m_scope);
  }
  void finishSoon(bool success)
  {
    if (!success)
      m_invalid = true;
    const auto epoch = m_epoch;
    QTimer::singleShot(0, this, [this, epoch, success] {
      if (epoch == m_epoch)
        finish(success);
    });
  }
  void finish(bool success)
  {
    const auto context = m_context;
    const auto scope = m_scope;
    const auto host = m_host;
    const auto addresses = m_addresses;
    const auto ttl = m_ttl;
    const bool ok = success && !m_invalid && available() && !addresses.isEmpty();
    cancel();
    if (context.valid()) {
      if (ok)
        Q_EMIT this->addresses(context, scope, host, addresses, ttl);
      else
        Q_EMIT failed(context, false);
    }
  }

public:
  NativeScopedResolver(QObject *parent, DiscoveryScopeSource scopes, ServiceNameSource services)
      : ScopedAddressResolver(parent),
        m_scopes(std::move(scopes)),
        m_services(std::move(services))
  {
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(3000);
    connect(&m_deadline, &QTimer::timeout, this, [this] { finish(false); });
    m_drain.setInterval(100);
    connect(&m_drain, &QTimer::timeout, this, [this] {
      if (!available()) {
        finish(false);
        return;
      }
#if defined(Q_OS_WIN) || defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
      if (m_query) {
        QString address;
        bool done = false, ok = false;
        {
          std::lock_guard lock(m_query->mutex);
          done = m_query->done;
          ok = m_query->ok;
          address = m_query->address;
        }
        if (done) {
          if (ok && validProbeAddress(m_scope, QHostAddress(address)))
            m_addresses << address;
          finish(ok);
        }
      }
#endif
    });
  }
  ~NativeScopedResolver() override
  {
    cancel();
  }
  void cancel() override
  {
    ++m_epoch;
    m_context = {};
    m_deadline.stop();
    m_drain.stop();
    m_addresses.clear();
    m_ttl = 10000;
    m_invalid = false;
#if defined(Q_OS_MACOS) && !defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
    delete m_notifier;
    m_notifier = nullptr;
    if (m_ref)
      DNSServiceRefDeallocate(m_ref);
    m_ref = nullptr;
#endif
#if defined(Q_OS_WIN) || defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
    if (m_query) {
      bool done = false;
      {
        std::lock_guard lock(m_query->mutex);
        done = m_query->done;
      }
      if (!done)
        DnsServiceResolveCancel(&m_query->cancel);
      m_query.reset(); // callback owns pending lifetime
    }
#endif
  }
  void
  start(const deskflow::auth::Context &context, const DiscoveryScope &scope, const QString &host, int port) override
  {
    cancel();
    m_context = context;
    m_scope = scope;
    m_host = DiscoveryCatalog::canonicalHost(host);
    m_port = port;
    if (!scope.valid() || !available() || port < 1024 || port > 65535 || !DiscoveryCatalog::validEndpoint(host, port)) {
      finish(false);
      return;
    }
#ifndef DESKFLOW_SCOPED_RESOLVER_TEST
    if (!deskflow::lab::kProbeNetworkExecutionEnabled) {
      finish(false);
      return;
    }
#endif
    m_deadline.start();
    m_drain.start();
#if defined(Q_OS_MACOS) && !defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
    const auto name = (m_host + '.').toUtf8();
    const auto error = DNSServiceGetAddrInfo(
        &m_ref, kDNSServiceFlagsForceMulticast, m_scope.index, kDNSServiceProtocol_IPv4, name.constData(),
        &NativeScopedResolver::reply, this
    );
    if (error || !m_ref) {
      finish(false);
      return;
    }
    const auto fd = DNSServiceRefSockFD(m_ref);
    if (fd < 0) {
      finish(false);
      return;
    }
    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this] {
      if (!available() || DNSServiceProcessResult(m_ref) != kDNSServiceErr_NoError)
        finish(false);
    });
#elif defined(Q_OS_WIN) || defined(DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST)
    // Re-resolve the selected DNS-SD service on its explicit interface. Avoid
    // DnsQueryEx MULTICAST_ONLY (documented as LLMNR), global DNS, or host files.
    const auto services = m_services ? m_services(m_host, port) : QStringList();
    if (services.isEmpty() || services.size() > 8) {
      finish(false);
      return;
    }
    QString chosen;
    for (const auto &name : services) {
      const QString suffix = "._deskflow._tcp.local";
      auto canonical = DiscoveryCatalog::canonicalHost(name);
      if (!canonical.endsWith(suffix))
        continue;
      const auto label = canonical.left(canonical.size() - suffix.size());
      if (DiscoveryCatalog::validName(label)) {
        chosen = canonical;
        break;
      }
    }
    if (chosen.isEmpty()) {
      finish(false);
      return;
    }
    auto q = std::make_shared<Query>();
    q->lifetime = q;
    q->name = chosen.toStdWString();
    q->expected = m_host;
    q->port = port;
    q->iface = scope.index;
    q->request.Version = DNS_QUERY_REQUEST_VERSION1;
    q->request.InterfaceIndex = scope.index;
    q->request.QueryName = q->name.data();
    q->request.pResolveCompletionCallback = &Query::reply;
    q->request.pQueryContext = q.get();
    m_query = q;
    const auto result = DnsServiceResolve(&q->request, &q->cancel);
    if (result != DNS_REQUEST_PENDING) {
      q->lifetime.reset();
      finish(false);
    }
#else
    finish(false);
#endif
  }
};
} // namespace
ScopedAddressResolver *
createScopedAddressResolver(QObject *parent, DiscoveryScopeSource scopes, ServiceNameSource services)
{
  return new NativeScopedResolver(parent, std::move(scopes), std::move(services));
}
} // namespace deskflow::gui
