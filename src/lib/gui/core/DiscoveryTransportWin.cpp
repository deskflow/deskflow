// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DiscoveryTransport.h"
#if defined(Q_OS_WIN) || defined(DESKFLOW_DNSSD_WINDOWS_TEST)
// Native Windows 10 DNS-SD. No Bonjour DLL/service or firewall changes.
#include "DiscoveryCatalog.h"
#ifdef DESKFLOW_DNSSD_WINDOWS_TEST
#include "DnsApiTestShim.h"
#else
#include <windns.h>
#include <windows.h>
#include <winsock2.h>
#endif
#include <QElapsedTimer>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>
namespace deskflow::gui {
namespace {
QString boundedWide(const wchar_t *value, size_t max)
{
  if (!value)
    return {};
  size_t size = 0;
  while (size <= max && value[size])
    ++size;
  return size <= max ? QString::fromWCharArray(value, static_cast<int>(size)) : QString();
}
QString serviceName(QString full)
{
  if (full.endsWith('.'))
    full.chop(1);
  const QString suffix = "._deskflow._tcp.local";
  if (!full.endsWith(suffix, Qt::CaseInsensitive))
    return {};
  full.chop(suffix.size());
  return DiscoveryCatalog::validName(full) ? full : QString();
}
struct Result
{
  enum Kind
  {
    Candidate,
    Removed,
    Resolved,
    Failed,
    Active
  } kind;
  QString key, name, host;
  int port = 0, lease = 120000;
  quint64 epoch = 0;
};
struct Pipe
{
  std::mutex mutex;
  bool active = true;
  std::deque<Result> results;
  void push(Result result)
  {
    std::lock_guard lock(mutex);
    if (active && results.size() < 128)
      results.push_back(std::move(result));
  }
};
struct Browse : std::enable_shared_from_this<Browse>
{
  std::shared_ptr<Browse> lifetime;
  std::shared_ptr<Pipe> pipe;
  DNS_SERVICE_BROWSE_REQUEST request{};
  DNS_SERVICE_CANCEL cancel{};
  std::atomic_bool done = false;
  std::wstring query = L"_deskflow._tcp.local";
  static VOID WINAPI callback(DWORD code, PVOID raw, PDNS_RECORD records)
  {
    auto self = static_cast<Browse *>(raw)->shared_from_this();
    if (code == ERROR_SUCCESS) {
      int count = 0;
      for (auto *r = records; r && count++ < 256; r = r->pNext) {
        if (r->wType != DNS_TYPE_PTR)
          continue;
        const auto full = boundedWide(r->Data.PTR.pNameHost, 300);
        const auto name = serviceName(full);
        if (name.isEmpty())
          continue;
        // Never consume TXT/certificate claims. PTR leases of zero withdraw.
        self->pipe->push(
            {r->dwTtl ? Result::Candidate : Result::Removed,
             full.toLower(),
             name,
             {},
             0,
             static_cast<int>(std::min<DWORD>(r->dwTtl, 120) * 1000)}
        );
      }
    }
    if (records)
      DnsRecordListFree(records, DnsFreeRecordList);
    if (code != ERROR_SUCCESS) {
      self->done = true;
      if (code != ERROR_CANCELLED)
        self->pipe->push({Result::Failed, {}, {}, {}});
      // Cancellation produces the final callback; preserve context until then.
      self->lifetime.reset();
    }
  }
};
struct Resolve : std::enable_shared_from_this<Resolve>
{
  std::shared_ptr<Resolve> lifetime;
  std::shared_ptr<Pipe> pipe;
  DNS_SERVICE_RESOLVE_REQUEST request{};
  DNS_SERVICE_CANCEL cancel{};
  std::wstring query;
  QString key, name;
  quint64 epoch = 0;
  quint32 interfaceIndex = 0;
  std::atomic_bool done = false;
  QElapsedTimer age;
  static VOID WINAPI callback(DWORD code, PVOID raw, PDNS_SERVICE_INSTANCE instance)
  {
    auto self = static_cast<Resolve *>(raw)->shared_from_this();
    if (code == ERROR_SUCCESS && instance) {
      const auto host = boundedWide(instance->pszHostName, 254);
      if (instance->dwInterfaceIndex == self->interfaceIndex && DiscoveryCatalog::validEndpoint(host, instance->wPort))
        self->pipe->push({Result::Resolved, self->key, self->name, host, instance->wPort, 120000, self->epoch});
    }
    if (instance)
      DnsServiceFreeInstance(instance);
    self->done = true;
    self->lifetime.reset();
  }
};
struct Registration : std::enable_shared_from_this<Registration>
{
  std::shared_ptr<Registration> lifetime;
  std::shared_ptr<Pipe> pipe;
  DNS_SERVICE_REGISTER_REQUEST request{};
  std::atomic_bool stopped = false, registered = false, deregistering = false;
  ~Registration()
  {
    if (request.pServiceInstance)
      DnsServiceFreeInstance(request.pServiceInstance);
  }
  bool withdraw()
  {
    stopped = true;
    if (!registered || deregistering.exchange(true))
      return true;
    const auto result = DnsServiceDeRegister(&request, nullptr);
    if (result != DNS_REQUEST_PENDING) {
      pipe->push({Result::Failed, {}, {}, {}});
      // No async request was accepted; no callback will use this context.
      lifetime.reset();
      return false;
    }
    return true;
  }
  static VOID WINAPI callback(DWORD code, PVOID raw, PDNS_SERVICE_INSTANCE instance)
  {
    auto self = static_cast<Registration *>(raw)->shared_from_this();
    const bool withdrawal = self->deregistering;
    if (instance)
      DnsServiceFreeInstance(instance);
    if (withdrawal || code != ERROR_SUCCESS) {
      if (code != ERROR_SUCCESS)
        self->pipe->push({Result::Failed, {}, {}, {}});
      self->lifetime.reset();
      return;
    }
    self->registered = true;
    self->pipe->push({Result::Active, {}, {}, {}});
    if (self->stopped)
      self->withdraw();
  }
};
class WinDiscoveryTransport final : public DiscoveryTransport
{
  std::shared_ptr<Pipe> m_pipe;
  std::shared_ptr<Browse> m_browser;
  std::shared_ptr<Registration> m_registration;
  std::vector<std::shared_ptr<Resolve>> m_queries;
  struct Service
  {
    QString name;
    qint64 lastAttempt = -30000, lastSuccess = 0;
    quint64 epoch = 0;
  };
  QMap<QString, Service> m_services;
  quint64 m_nextEpoch = 0;
  QElapsedTimer m_clock;
  QTimer m_poll;
  DiscoveryScope m_scope;
  QElapsedTimer m_session;
  QTimer m_deadline;
  void resolve(const QString &key)
  {
    if (!scopeAvailable(m_scope) || m_queries.size() >= 16 || !m_services.contains(key))
      return;
    auto &s = m_services[key];
    if (m_clock.elapsed() - s.lastAttempt < 30000)
      return;
    s.lastAttempt = m_clock.elapsed();
    auto q = std::make_shared<Resolve>();
    q->lifetime = q;
    q->pipe = m_pipe;
    q->key = key;
    q->name = s.name;
    q->epoch = s.epoch;
    q->query = key.toStdWString();
    q->age.start();
    q->request.Version = DNS_QUERY_REQUEST_VERSION1;
    q->interfaceIndex = m_scope.index;
    q->request.InterfaceIndex = m_scope.index;
    q->request.QueryName = q->query.data();
    q->request.pQueryContext = q.get();
    q->request.pResolveCompletionCallback = &Resolve::callback;
    const auto code = DnsServiceResolve(&q->request, &q->cancel);
    if (code == DNS_REQUEST_PENDING)
      m_queries.push_back(q);
    else
      q->lifetime.reset();
  }
  void poll()
  {
    if (!m_pipe)
      return;
    if (!scopeAvailable(m_scope) || m_session.elapsed() >= kDiscoverySessionMs) {
      stop();
      Q_EMIT status(tr("Discovery stopped. Select a local network and start again."));
      Q_EMIT sessionEnded();
      return;
    }
    std::deque<Result> results;
    {
      std::lock_guard lock(m_pipe->mutex);
      results.swap(m_pipe->results);
    }
    for (const auto &r : results) {
      if (r.kind == Result::Candidate) {
        if (m_services.contains(r.key) || m_services.size() < 64) {
          if (!m_services.contains(r.key))
            m_services[r.key] = {r.name, -30000, m_clock.elapsed(), ++m_nextEpoch};
          resolve(r.key);
        }
      } else if (r.kind == Result::Removed) {
        m_services.remove(r.key);
        Q_EMIT removed(r.key);
      } else if (r.kind == Result::Resolved) {
        // A resolve completing after goodbye must not resurrect the device.
        if (m_services.contains(r.key) && m_services[r.key].epoch == r.epoch) {
          m_services[r.key].lastSuccess = m_clock.elapsed();
          Q_EMIT resolved(r.key, r.name, r.host, r.port, r.lease);
        }
      } else if (r.kind == Result::Active) {
        Q_EMIT activeChanged(true);
        Q_EMIT status(tr("Discoverable on the selected network. Fingerprint confirmation is still required."));
      } else {
        stop();
        Q_EMIT status(tr("Local discovery stopped. Check local network access and refresh."));
        Q_EMIT sessionEnded();
        return;
      }
    }
    for (auto it = m_queries.begin(); it != m_queries.end();) {
      if ((*it)->done)
        it = m_queries.erase(it);
      else if ((*it)->age.elapsed() >= 10000) {
        DnsServiceResolveCancel(&(*it)->cancel);
        it = m_queries.erase(it);
      } else
        ++it;
    }
    for (const auto &key : m_services.keys()) {
      if (m_clock.elapsed() - m_services[key].lastSuccess >= 150000) {
        m_services.remove(key);
        Q_EMIT removed(key);
      } else
        resolve(key);
    }
  }

public:
  explicit WinDiscoveryTransport(QObject *parent, DiscoveryScopeSource source)
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
    m_poll.setInterval(250);
    connect(&m_poll, &QTimer::timeout, this, [this] { poll(); });
  }
  ~WinDiscoveryTransport() override
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
    m_pipe = std::make_shared<Pipe>();
    m_browser = std::make_shared<Browse>();
    auto &b = *m_browser;
    b.lifetime = m_browser;
    b.pipe = m_pipe;
    b.request.Version = DNS_QUERY_REQUEST_VERSION1;
    b.request.InterfaceIndex = m_scope.index;
    b.request.QueryName = b.query.c_str();
    b.request.pBrowseCallback = &Browse::callback;
    b.request.pQueryContext = &b;
    const auto code = DnsServiceBrowse(&b.request, &b.cancel);
    if (code != DNS_REQUEST_PENDING) {
      b.lifetime.reset();
      m_browser.reset();
      m_pipe.reset();
      Q_EMIT status(tr("Nearby discovery is unavailable."));
      Q_EMIT sessionEnded();
      return;
    }
    m_poll.start();
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
    const auto service = (ad.instance() + "._deskflow._tcp.local").toStdWString(), hostname = ad.host().toStdWString();
    IP4_ADDRESS ipv4 = htonl(m_scope.address.toIPv4Address());
    m_pipe = std::make_shared<Pipe>();
    m_registration = std::make_shared<Registration>();
    auto &r = *m_registration;
    r.pipe = m_pipe;
    r.lifetime = m_registration;
    r.request.Version = DNS_QUERY_REQUEST_VERSION1;
    r.request.InterfaceIndex = m_scope.index;
    // Explicit random host and selected private address: no system/computer name, TXT or identity claims.
    r.request.pServiceInstance = DnsServiceConstructInstance(
        service.c_str(), hostname.c_str(), &ipv4, nullptr, static_cast<WORD>(ad.port), 0, 0, 0, nullptr, nullptr
    );
    r.request.pRegisterCompletionCallback = &Registration::callback;
    r.request.pQueryContext = &r;
    r.request.unicastEnabled = FALSE;
    if (r.request.pServiceInstance)
      r.request.pServiceInstance->dwInterfaceIndex = m_scope.index;
    if (!r.request.pServiceInstance || DnsServiceRegister(&r.request, nullptr) != DNS_REQUEST_PENDING) {
      r.lifetime.reset();
      m_registration.reset();
      m_pipe.reset();
      Q_EMIT status(tr("Could not advertise this server. Check local network access."));
      Q_EMIT sessionEnded();
      return;
    }
    m_poll.start();
  }
  void stop() override
  {
    m_deadline.stop();
    m_poll.stop();
    if (m_pipe) {
      std::lock_guard lock(m_pipe->mutex);
      m_pipe->active = false;
      m_pipe->results.clear();
    }
    if (m_browser && !m_browser->done)
      DnsServiceBrowseCancel(&m_browser->cancel);
    for (auto &q : m_queries)
      if (!q->done)
        DnsServiceResolveCancel(&q->cancel);
    if (m_registration && !m_registration->withdraw())
      Q_EMIT status(tr("Could not withdraw the discovery advertisement. Quit the application to remove it."));
    m_browser.reset();
    m_registration.reset();
    m_queries.clear();
    m_services.clear();
    m_pipe.reset();
    m_scope = {};
    m_session.invalidate();
    Q_EMIT activeChanged(false);
  }
};
} // namespace
DiscoveryTransport *createDiscoveryTransport(QObject *parent, DiscoveryScopeSource source)
{
  return new WinDiscoveryTransport(parent, std::move(source));
}
} // namespace deskflow::gui
#endif
