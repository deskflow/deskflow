/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NetworkDiscovery.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QTimer>
#include <QUdpSocket>
#include <utility>

namespace deskflow::gui {

namespace {
// Dedicated discovery port; distinct from the coordination mesh (24851) and
// the deskflow transport (24800) so a scan never disturbs a live session.
constexpr quint16 kDiscoveryPort = 24853;
constexpr auto kTypeProbe = "deskflow-discover";
constexpr auto kTypeHere = "deskflow-here";
} // namespace

NetworkDiscovery::NetworkDiscovery(QString selfName, QObject *parent)
    : QObject(parent),
      m_selfName(std::move(selfName)),
      m_socket(new QUdpSocket(this)),
      m_scanTimer(new QTimer(this))
{
  m_scanTimer->setSingleShot(true);
  connect(m_scanTimer, &QTimer::timeout, this, [this] { Q_EMIT scanFinished(); });
  connect(m_socket, &QUdpSocket::readyRead, this, &NetworkDiscovery::onDatagram);
}

NetworkDiscovery::~NetworkDiscovery() = default;

bool NetworkDiscovery::start()
{
  // ShareAddress so the GUI and a local auto-mode core can both bind the
  // discovery port on the same machine.
  return m_socket->bind(
      QHostAddress::AnyIPv4, kDiscoveryPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
  );
}

void NetworkDiscovery::scan(int durationMs)
{
  m_seenThisScan.clear();
  sendProbe();
  m_scanTimer->start(durationMs);
}

void NetworkDiscovery::sendProbe()
{
  QJsonObject probe;
  probe[QStringLiteral("t")] = QString::fromLatin1(kTypeProbe);
  probe[QStringLiteral("name")] = m_selfName;
  const auto payload = QJsonDocument(probe).toJson(QJsonDocument::Compact);
  m_socket->writeDatagram(payload, QHostAddress::Broadcast, kDiscoveryPort);
}

void NetworkDiscovery::onDatagram()
{
  while (m_socket->hasPendingDatagrams()) {
    const QNetworkDatagram dgram = m_socket->receiveDatagram();
    const auto doc = QJsonDocument::fromJson(dgram.data());
    if (!doc.isObject()) {
      continue;
    }
    const QJsonObject obj = doc.object();
    const QString type = obj[QStringLiteral("t")].toString();
    const QString name = obj[QStringLiteral("name")].toString().trimmed();
    if (name.isEmpty()) {
      continue;
    }

    if (type == QLatin1String(kTypeProbe)) {
      // Someone is scanning; announce ourselves back to them (unicast).
      if (name == m_selfName) {
        continue; // our own broadcast
      }
      QJsonObject reply;
      reply[QStringLiteral("t")] = QString::fromLatin1(kTypeHere);
      reply[QStringLiteral("name")] = m_selfName;
      const auto payload = QJsonDocument(reply).toJson(QJsonDocument::Compact);
      m_socket->writeDatagram(payload, dgram.senderAddress(), static_cast<quint16>(dgram.senderPort()));
    } else if (type == QLatin1String(kTypeHere)) {
      if (name == m_selfName || m_seenThisScan.contains(name)) {
        continue;
      }
      m_seenThisScan.insert(name);
      Q_EMIT discovered(name);
    }
  }
}

} // namespace deskflow::gui
