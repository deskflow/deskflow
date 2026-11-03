/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ServerDiscovery.h"

#include <QElapsedTimer>
#include <QMutexLocker>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QUuid>

#include <algorithm>
#include <utility>

namespace deskflow::discovery {
namespace {
const QByteArray kQuery = QByteArrayLiteral("DeskflowDiscover/1");
const QByteArray kReply = QByteArrayLiteral("DeskflowServer/1");
constexpr qint64 kMaxPacket = 1024;

QList<QHostAddress> broadcastAddresses()
{
  QSet<QHostAddress> addresses{QHostAddress(QHostAddress::Broadcast)};
  for (const auto &interface : QNetworkInterface::allInterfaces()) {
    const auto flags = interface.flags();
    if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning) ||
        (flags & QNetworkInterface::IsLoopBack) || !(flags & QNetworkInterface::CanBroadcast)) {
      continue;
    }
    for (const auto &entry : interface.addressEntries()) {
      if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.broadcast().isNull()) {
        addresses.insert(entry.broadcast());
      }
    }
  }
  return addresses.values();
}
} // namespace

Responder::Responder(quint16 servicePort, quint16 discoveryPort, QHostAddress bindAddress)
    : m_servicePort(servicePort),
      m_discoveryPort(discoveryPort),
      m_bindAddress(std::move(bindAddress))
{
}

Responder::~Responder()
{
  requestInterruption();
  wait();
}

void Responder::setAllowedClients(const QSet<QString> &names)
{
  QSet<QString> normalized;
  for (const auto &name : names) {
    normalized.insert(name.toCaseFolded());
  }
  QMutexLocker lock(&m_mutex);
  m_allowedClients = std::move(normalized);
}

bool Responder::startListening()
{
  QMutexLocker lock(&m_mutex);
  m_listenReady = false;
  m_bound = false;
  start();
  while (!m_listenReady) {
    if (!m_ready.wait(&m_mutex, 2000)) {
      return false;
    }
  }
  return m_bound;
}

void Responder::run()
{
  QUdpSocket socket;
  const bool bound = socket.bind(m_bindAddress, m_discoveryPort, QAbstractSocket::DontShareAddress);
  {
    QMutexLocker lock(&m_mutex);
    m_bound = bound;
    m_listenReady = true;
    m_ready.wakeAll();
  }
  if (!bound) {
    return;
  }

  while (!isInterruptionRequested()) {
    if (!socket.waitForReadyRead(250)) {
      continue;
    }
    while (socket.hasPendingDatagrams()) {
      const auto datagram = socket.receiveDatagram(kMaxPacket);
      if (datagram.data().size() >= kMaxPacket) {
        continue;
      }
      const auto parts = datagram.data().split('\n');
      if (parts.size() != 3 || parts[0] != kQuery || parts[1].size() != 36 ||
          QUuid(QString::fromLatin1(parts[1])).isNull() || parts[2].isEmpty() || parts[2].size() > 255) {
        continue;
      }
      {
        QMutexLocker lock(&m_mutex);
        if (!m_allowedClients.contains(QString::fromUtf8(parts[2]).toCaseFolded())) {
          continue;
        }
      }
      const auto reply = kReply + '\n' + parts[1] + '\n' + QByteArray::number(m_servicePort);
      socket.writeDatagram(reply, datagram.senderAddress(), datagram.senderPort());
    }
  }
}

Result findServer(const QString &clientName, int timeoutMs, quint16 discoveryPort, const QList<QHostAddress> &destinations)
{
  const auto name = clientName.toUtf8();
  if (name.isEmpty() || name.size() > 255 || name.contains('\n') || timeoutMs <= 0) {
    return {Result::Status::NotFound, {}, 0};
  }

  QUdpSocket socket;
  if (!socket.bind(QHostAddress::AnyIPv4, 0)) {
    return {Result::Status::SocketError, {}, 0};
  }
  const auto nonce = QUuid::createUuid().toString(QUuid::WithoutBraces).toLatin1();
  const auto query = kQuery + '\n' + nonce + '\n' + name;
  const auto targets = destinations.isEmpty() ? broadcastAddresses() : destinations;
  bool sent = false;
  for (const auto &target : targets) {
    sent = socket.writeDatagram(query, target, discoveryPort) >= 0 || sent;
  }
  if (!sent) {
    return {Result::Status::SocketError, {}, 0};
  }

  QElapsedTimer timer;
  timer.start();
  QSet<QString> endpoints;
  QHostAddress foundAddress;
  quint16 foundPort = 0;
  while (timer.elapsed() < timeoutMs) {
    const auto remaining = std::max(1, timeoutMs - static_cast<int>(timer.elapsed()));
    if (!socket.waitForReadyRead(remaining)) {
      break;
    }
    while (socket.hasPendingDatagrams()) {
      const auto datagram = socket.receiveDatagram(kMaxPacket);
      const auto parts = datagram.data().split('\n');
      bool validPort = false;
      const auto port = parts.size() == 3 ? parts[2].toUInt(&validPort) : 0;
      if (datagram.data().size() >= kMaxPacket || parts.size() != 3 || parts[0] != kReply || parts[1] != nonce ||
          !validPort || port == 0 || port > 65535 ||
          datagram.senderPort() != discoveryPort || datagram.senderAddress().protocol() != QAbstractSocket::IPv4Protocol) {
        continue;
      }
      foundAddress = datagram.senderAddress();
      foundPort = static_cast<quint16>(port);
      endpoints.insert(foundAddress.toString() + ':' + QString::number(foundPort));
    }
  }
  if (endpoints.size() > 1) {
    return {Result::Status::Multiple, {}, 0};
  }
  if (endpoints.isEmpty()) {
    return {Result::Status::NotFound, {}, 0};
  }
  return {Result::Status::Found, foundAddress, foundPort};
}

} // namespace deskflow::discovery
