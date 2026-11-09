// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "NearbyPublisher.h"
#include "DiscoveryTransport.h"
namespace deskflow::gui {
NearbyPublisher::NearbyPublisher(QObject *parent, DiscoveryTransport *t, DiscoveryScopeSource source)
    : QObject(parent),
      m_transport(t ? t : createDiscoveryTransport(this, source)),
      m_scopes(std::move(source))
{
  m_deadline.setParent(this);
  m_deadline.setObjectName("publisherDeadline");
  m_deadline.setSingleShot(true);
  m_deadline.setTimerType(Qt::PreciseTimer);
  m_deadline.setInterval(kDiscoverySessionMs);
  connect(&m_deadline, &QTimer::timeout, this, &NearbyPublisher::renew);
  connect(m_transport, &DiscoveryTransport::status, this, &NearbyPublisher::status);
  connect(m_transport, &DiscoveryTransport::sessionEnded, this, [this] {
    const auto old = m_scope;
    // An address change on the already selected link can be revalidated.
    // Backend errors on an unchanged link fail closed, never an infinite restart.
    if (m_enabled && m_untilStopped && (!refreshScope() || old != m_scope))
      return;
    finish();
  });
}
NearbyPublisher::~NearbyPublisher()
{
  m_enabled = false;
  m_deadline.stop();
  m_transport->stop();
}
void NearbyPublisher::finish()
{
  m_enabled = false;
  m_deadline.stop();
  m_token.clear();
  if (m_advertising) {
    m_advertising = false;
    m_transport->stop();
  }
  Q_EMIT exposureEnded();
}
void NearbyPublisher::configure(
    bool enabled, bool server, int port, bool tls, bool peerChecks, const DiscoveryScope &scope, bool untilStopped
)
{
  const bool changed = m_port != port || m_scope != scope || m_untilStopped != untilStopped;
  const bool fresh = enabled && !m_enabled;
  m_server = server;
  m_port = port;
  m_tls = tls;
  m_peerChecks = peerChecks;
  m_untilStopped = untilStopped;
  m_scope = scope;
  if (!enabled || !server) {
    finish();
    return;
  }
  m_enabled = true;
  if (fresh || changed) {
    m_token = DiscoveryAdvertisement::newToken();
    m_deadline.start(untilStopped ? 240000 : kDiscoverySessionMs);
  }
  if (!scope.valid()) {
    finish();
    Q_EMIT status(tr("Select an available private Wi-Fi or Ethernet network."));
    return;
  }
  reconcile(changed);
}
void NearbyPublisher::processChanged(deskflow::core::ProcessState state)
{
  m_process = state;
  if (state != deskflow::core::ProcessState::Started)
    m_connection = deskflow::core::ConnectionState::Disconnected;
  if (m_advertising && state != deskflow::core::ProcessState::Started && !m_untilStopped) {
    finish();
    return;
  }
  reconcile();
}
void NearbyPublisher::connectionChanged(deskflow::core::ConnectionState state)
{
  m_connection = state;
  reconcile();
}
bool NearbyPublisher::refreshScope()
{
  const auto mask = 0xffffffffu >> m_scope.prefix;
  for (const auto &s : m_scopes()) {
    if (s.valid() && s.index == m_scope.index && s.name == m_scope.name && s.prefix == m_scope.prefix &&
        (s.address.toIPv4Address() & ~mask) == (m_scope.address.toIPv4Address() & ~mask)) {
      const auto before = m_scope;
      const bool changed = s != m_scope;
      m_scope = s;
      if (changed) {
        m_advertising = false;
        m_transport->stop();
        m_token = DiscoveryAdvertisement::newToken();
        Q_EMIT selectedAddressChanged(before, s);
      }
      reconcile(changed);
      return true;
    }
  }
  if (m_advertising) {
    m_advertising = false;
    m_transport->stop();
  }
  Q_EMIT status(tr("Discovery is waiting for the selected network. Stop turns it off."));
  return false;
}
void NearbyPublisher::networkChanged()
{
  if (!m_enabled)
    return;
  if (m_untilStopped) {
    refreshScope();
    return;
  }
  finish();
  Q_EMIT status(tr("The network changed. Select a network and enable discovery again."));
}
void NearbyPublisher::renew()
{
  if (!m_enabled)
    return;
  if (!m_untilStopped) {
    finish();
    Q_EMIT status(tr("Discovery stopped after 5 minutes. Turn it on again to continue."));
    return;
  }
  if (!m_scope.valid()) {
    finish();
    return;
  }
  m_deadline.start(240000);
  if (!refreshScope())
    return;
  if (m_advertising) {
    m_advertising = false;
    m_transport->stop();
  }
  m_token = DiscoveryAdvertisement::newToken();
  reconcile();
}
void NearbyPublisher::reconcile(bool changed)
{
  const DiscoveryAdvertisement ad{m_scope, m_token, m_port};
  const bool ready = m_enabled && m_server && m_tls && m_peerChecks &&
                     m_process == deskflow::core::ProcessState::Started &&
                     (m_connection == deskflow::core::ConnectionState::Listening ||
                      m_connection == deskflow::core::ConnectionState::Connected) &&
                     ad.valid();
  if (m_advertising && (!ready || changed)) {
    m_advertising = false;
    m_transport->stop();
  }
  if (ready && !m_advertising) {
    m_advertising = true;
    m_transport->advertise(ad);
  }
}
} // namespace deskflow::gui
