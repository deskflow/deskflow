// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "DiscoveryScope.h"
#include "common/Enums.h"
#include <QObject>
#include <QTimer>
namespace deskflow::gui {
class DiscoveryTransport;
class NearbyPublisher : public QObject
{
  Q_OBJECT
public:
  explicit NearbyPublisher(
      QObject *parent = nullptr, DiscoveryTransport *transport = nullptr,
      DiscoveryScopeSource source = DiscoveryScope::current
  );
  ~NearbyPublisher() override;
  void configure(
      bool enabled, bool server, int port, bool tls, bool peerChecks, const DiscoveryScope &scope,
      bool untilStopped = false
  );
  void processChanged(deskflow::core::ProcessState state);
  void connectionChanged(deskflow::core::ConnectionState state);
  void networkChanged();
  bool enabled() const
  {
    return m_enabled;
  }
  DiscoveryScope scope() const
  {
    return m_scope;
  }
  void renew(); // Timer and deterministic offline tests use the same path.
Q_SIGNALS:
  void status(const QString &text);
  void exposureEnded();
  void selectedAddressChanged(const DiscoveryScope &before, const DiscoveryScope &after);

private:
  DiscoveryTransport *m_transport;
  bool m_enabled = false, m_server = false, m_tls = false, m_peerChecks = false, m_advertising = false;
  bool m_untilStopped = false;
  DiscoveryScopeSource m_scopes;
  QString m_token;
  DiscoveryScope m_scope;
  QTimer m_deadline;
  int m_port = 0;
  void finish();
  bool refreshScope();
  deskflow::core::ProcessState m_process = deskflow::core::ProcessState::Stopped;
  deskflow::core::ConnectionState m_connection = deskflow::core::ConnectionState::Disconnected;
  void reconcile(bool changed = false);
};
} // namespace deskflow::gui
