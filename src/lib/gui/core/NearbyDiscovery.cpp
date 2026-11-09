// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "NearbyDiscovery.h"
#include "DiscoveryCatalog.h"
#include "DiscoveryTransport.h"
#include <QElapsedTimer>
#include <QTimer>
namespace deskflow::gui {
struct NearbyDiscovery::Impl
{
  DiscoveryTransport *transport;
  DiscoveryCatalog catalog;
  QElapsedTimer clock;
  QTimer expiry;
  quint64 generation = 0;
  explicit Impl(DiscoveryTransport *t) : transport(t)
  {
    clock.start();
    expiry.setInterval(1000);
  }
};
NearbyDiscovery::NearbyDiscovery(QObject *parent, DiscoveryTransport *transport)
    : QObject(parent),
      m_impl(std::make_unique<Impl>(transport ? transport : createDiscoveryTransport(this)))
{
  connect(&m_impl->catalog, &DiscoveryCatalog::found, this, &NearbyDiscovery::found);
  connect(&m_impl->catalog, &DiscoveryCatalog::removed, this, &NearbyDiscovery::removed);
  connect(&m_impl->expiry, &QTimer::timeout, this, [this] { m_impl->catalog.expire(m_impl->clock.elapsed()); });
}
NearbyDiscovery::~NearbyDiscovery()
{
  stop();
}
bool NearbyDiscovery::validEndpoint(const QString &host, int port)
{
  return DiscoveryCatalog::validEndpoint(host, port);
}
void NearbyDiscovery::start(const DiscoveryScope &scope)
{
  stop();
  if (!scope.valid()) {
    Q_EMIT status(tr("Select an available private Wi-Fi or Ethernet network."));
    return;
  }
  const auto generation = m_impl->generation;
  auto *t = m_impl->transport;
  connect(
      t, &DiscoveryTransport::resolved, this,
      [this, generation](const QString &key, const QString &name, const QString &host, int port, int lease) {
        if (generation == m_impl->generation)
          m_impl->catalog.upsert(key, name, host, port, m_impl->clock.elapsed(), lease);
      },
      Qt::QueuedConnection
  );
  connect(
      t, &DiscoveryTransport::removed, this,
      [this, generation](const QString &key) {
        if (generation == m_impl->generation)
          m_impl->catalog.remove(key);
      },
      Qt::QueuedConnection
  );
  connect(
      t, &DiscoveryTransport::status, this,
      [this, generation](const QString &text) {
        if (generation == m_impl->generation)
          Q_EMIT status(text);
      },
      Qt::QueuedConnection
  );
  connect(
      t, &DiscoveryTransport::activeChanged, this,
      [this, generation](bool active) {
        if (generation == m_impl->generation)
          Q_EMIT searchingChanged(active);
      },
      Qt::QueuedConnection
  );
  connect(
      t, &DiscoveryTransport::sessionEnded, this,
      [this, generation] {
        if (generation == m_impl->generation)
          stop();
      },
      Qt::QueuedConnection
  );
  m_impl->expiry.start();
  t->browse(scope);
}
QStringList NearbyDiscovery::serviceNames(const QString &host, int port) const
{
  return m_impl->catalog.serviceNames(host, port);
}
void NearbyDiscovery::stop()
{
  ++m_impl->generation;
  disconnect(m_impl->transport, nullptr, this, nullptr);
  m_impl->transport->stop();
  m_impl->expiry.stop();
  m_impl->catalog.clear();
  Q_EMIT searchingChanged(false);
}
} // namespace deskflow::gui
