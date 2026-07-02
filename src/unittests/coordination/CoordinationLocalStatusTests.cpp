/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoordinationLocalStatusTests.h"

#include "common/CoordinationLocalStatus.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <thread>

using deskflow::common::serverHostsFromStatus;

void CoordinationLocalStatusTests::serverHostsFromStatus_excludesSelf()
{
  QJsonObject fleet;
  QJsonArray peers;
  QJsonObject selfPeer;
  selfPeer[QStringLiteral("name")] = QStringLiteral("macbookpro");
  selfPeer[QStringLiteral("ip")] = QStringLiteral("10.0.0.5");
  peers.append(selfPeer);
  fleet[QStringLiteral("peers")] = peers;

  QJsonObject status;
  status[QStringLiteral("server_ip")] = QStringLiteral("10.0.0.1");
  status[QStringLiteral("fleet")] = fleet;

  const auto hosts = serverHostsFromStatus(status, QStringLiteral("macbookpro"));
  QCOMPARE(hosts.size(), 1);
  QCOMPARE(hosts.front(), QStringLiteral("10.0.0.1"));
}

void CoordinationLocalStatusTests::serverHostsFromStatus_includesPeerAddresses()
{
  QJsonObject fleet;
  QJsonArray peers;
  QJsonObject peer;
  peer[QStringLiteral("name")] = QStringLiteral("hackintosh");
  peer[QStringLiteral("ip")] = QStringLiteral("10.0.0.9");
  peer[QStringLiteral("lan")] = QStringLiteral("hackintosh.local");
  peers.append(peer);
  fleet[QStringLiteral("server")] = QStringLiteral("hackintosh");
  fleet[QStringLiteral("peers")] = peers;

  QJsonObject status;
  status[QStringLiteral("server_ip")] = QStringLiteral("10.0.0.9");
  status[QStringLiteral("fleet")] = fleet;

  const auto hosts = serverHostsFromStatus(status, QStringLiteral("macbookpro"));
  QVERIFY(hosts.contains(QStringLiteral("10.0.0.9")));
  QVERIFY(hosts.contains(QStringLiteral("hackintosh.local")));
  QVERIFY(hosts.contains(QStringLiteral("hackintosh")));
}

void CoordinationLocalStatusTests::pollLocalFleetStatus_readsSnapshot()
{
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));

  const auto port = static_cast<quint16>(server.serverPort());
  const QByteArray reply =
      QByteArray(R"({"role":"client","server_ip":"10.0.0.9","name":"macbookpro","fleet":)"
                 R"({"server":"hackintosh","cursor_host":"hackintosh","cursor_screen":"hackintosh",)"
                 R"("peers":[{"name":"hackintosh","ip":"10.0.0.9","lan":"hackintosh.local"}]}})") +
      '\n';

  std::optional<deskflow::common::LocalStatusFleet> snapshot;
  std::thread pollThread([&]() { snapshot = deskflow::common::pollLocalFleetStatus(port, 2000); });

  QVERIFY(server.waitForNewConnection(2000));
  QTcpSocket *client = server.nextPendingConnection();
  QVERIFY(client->waitForReadyRead(2000));
  client->readAll();
  client->write(reply);
  client->waitForBytesWritten(2000);
  client->disconnectFromHost();

  pollThread.join();
  QVERIFY(snapshot.has_value());
  QCOMPARE(snapshot->cursorHost, QStringLiteral("hackintosh"));
  QCOMPARE(snapshot->server, QStringLiteral("hackintosh"));
  QVERIFY(snapshot->peerHosts.contains(QStringLiteral("10.0.0.9")));
}

QTEST_MAIN(CoordinationLocalStatusTests)
