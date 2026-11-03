/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QHostAddress>
#include <QList>
#include <QMutex>
#include <QSet>
#include <QString>
#include <QThread>
#include <QWaitCondition>

namespace deskflow::discovery {

inline constexpr quint16 kDiscoveryPort = 24801;

struct Result
{
  enum class Status { Found, NotFound, Multiple, SocketError };
  Status status = Status::NotFound;
  QHostAddress address;
  quint16 port = 0;
};

// Runs its UDP socket in its own thread; no Qt event loop is needed in the
// core's custom event queue. Only names configured on the server get replies.
class Responder final : public QThread
{
public:
  explicit Responder(
      quint16 servicePort, quint16 discoveryPort = kDiscoveryPort,
      QHostAddress bindAddress = QHostAddress(QHostAddress::AnyIPv4)
  );
  ~Responder() override;

  bool startListening();
  void setAllowedClients(const QSet<QString> &names);

protected:
  void run() override;

private:
  quint16 m_servicePort;
  quint16 m_discoveryPort;
  QHostAddress m_bindAddress;
  QMutex m_mutex;
  QWaitCondition m_ready;
  QSet<QString> m_allowedClients;
  bool m_listenReady = false;
  bool m_bound = false;
};

// Each reply must contain this query's random nonce and is connected through
// the existing Deskflow TLS/fingerprint verification after discovery.
Result findServer(
    const QString &clientName, int timeoutMs = 800, quint16 discoveryPort = kDiscoveryPort,
    const QList<QHostAddress> &destinations = {}
);

} // namespace deskflow::discovery
