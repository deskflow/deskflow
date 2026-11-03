/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ServerDiscovery.h"

#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QUdpSocket>

#include <atomic>
#include <iostream>
#include <utility>

using namespace deskflow::discovery;

class FakeResponder : public QThread
{
public:
  FakeResponder(QHostAddress bindAddress, quint16 port) : m_address(std::move(bindAddress)), m_port(port)
  {
  }

protected:
  void run() override
  {
    QUdpSocket socket;
    const bool bound = socket.bind(m_address, m_port, QAbstractSocket::DontShareAddress);
    m_bound.store(bound);
    m_ready.store(true);
    if (!bound || !socket.waitForReadyRead(2000)) {
      return;
    }
    const auto query = socket.receiveDatagram();
    const auto parts = query.data().split('\n');
    if (parts.size() == 3) {
      socket.writeDatagram(
          QByteArrayLiteral("DeskflowServer/1\n") + parts[1] + QByteArrayLiteral("\n25344"),
          query.senderAddress(), query.senderPort()
      );
    }
  }

public:
  bool waitUntilReady()
  {
    for (int i = 0; i < 200 && !m_ready.load(); ++i) {
      QThread::msleep(10);
    }
    return m_ready.load() && m_bound.load();
  }

private:
  std::atomic<bool> m_ready{false};
  std::atomic<bool> m_bound{false};
  QHostAddress m_address;
  quint16 m_port;
};

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  QUdpSocket probe;
  if (!probe.bind(QHostAddress::LocalHost, 0)) {
    std::cerr << "failed to find a test port\n";
    return 1;
  }
  const auto port = probe.localPort();
  probe.close();

  Responder responder(25344, port, QHostAddress(QHostAddress::LocalHost));
  responder.setAllowedClients({QStringLiteral("mac.local")});
  if (!responder.startListening()) {
    std::cerr << "failed to start responder\n";
    return 1;
  }
  const QList<QHostAddress> loopback{QHostAddress(QHostAddress::LocalHost)};
  const auto found = findServer(QStringLiteral("MAC.local"), 250, port, loopback);
  if (found.status != Result::Status::Found || found.address != QHostAddress::LocalHost || found.port != 25344) {
    std::cerr << "configured client did not discover server\n";
    return 1;
  }
  if (findServer(QStringLiteral("unknown.local"), 250, port, loopback).status != Result::Status::NotFound) {
    std::cerr << "unknown client received a discovery reply\n";
    return 1;
  }
  responder.setAllowedClients({QStringLiteral("new.local")});
  if (findServer(QStringLiteral("mac.local"), 250, port, loopback).status != Result::Status::NotFound ||
      findServer(QStringLiteral("new.local"), 250, port, loopback).status != Result::Status::Found) {
    std::cerr << "updated client names were not applied\n";
    return 1;
  }

  // macOS does not bind 127.0.0.2 by default. Use another active IPv4
  // interface if available; Linux can also use its 127.0.0.2 loopback.
  QList<QHostAddress> candidates;
  for (const auto &interface : QNetworkInterface::allInterfaces()) {
    const auto flags = interface.flags();
    if (!(flags & QNetworkInterface::IsUp) || (flags & QNetworkInterface::IsLoopBack)) {
      continue;
    }
    for (const auto &entry : interface.addressEntries()) {
      if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
        candidates.append(entry.ip());
      }
    }
  }
  candidates.append(QHostAddress(QStringLiteral("127.0.0.2")));
  bool checkedMultiple = false;
  for (const auto &candidate : candidates) {
    FakeResponder second(candidate, port);
    second.start();
    if (!second.waitUntilReady()) {
      second.wait();
      continue;
    }
    const auto ambiguous = findServer(QStringLiteral("new.local"), 500, port, {QHostAddress::LocalHost, candidate});
    second.wait();
    if (ambiguous.status != Result::Status::Multiple) {
      std::cerr << "multiple servers were not reported\n";
      return 1;
    }
    checkedMultiple = true;
    break;
  }
  if (!checkedMultiple) {
    std::cout << "SKIP: no second IPv4 address available for the multiple-server check\n";
  }

  std::cout << "Server discovery checks passed\n";
  return 0;
}
