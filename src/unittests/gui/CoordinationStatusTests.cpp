/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoordinationStatusTests.h"

#include "gui/CoordinationStatus.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QElapsedTimer>
#include <QTest>

using deskflow::gui::CoordinationStatus;
using deskflow::gui::formatFleetGraph;

void CoordinationStatusTests::formatFleetGraph_emptyFleet()
{
  QVERIFY(formatFleetGraph({}).isEmpty());
}

void CoordinationStatusTests::formatFleetGraph_screensAndEdges()
{
  QJsonObject fleet;
  fleet[QStringLiteral("screens")] = QJsonArray{
      QStringLiteral("alpha"), QStringLiteral("beta")
  };
  fleet[QStringLiteral("links")] = QJsonArray{
      QJsonObject{
          {QStringLiteral("from"), QStringLiteral("alpha")},
          {QStringLiteral("to"), QStringLiteral("beta")},
          {QStringLiteral("dir"), QStringLiteral("right")},
      }
  };

  const QString graph = formatFleetGraph(fleet);
  QCOMPARE(graph, QStringLiteral("alpha · beta — alpha→beta (right)"));
}

void CoordinationStatusTests::poll_emitsOnlineWithFleetGraph()
{
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  const auto port = static_cast<quint16>(server.serverPort());
  const QByteArray reply =
      QByteArray(R"({"role":"client","server_ip":"hackintosh.tail.ts.net","name":"macbookpro","fleet":)"
                 R"({"screens":["hackintosh","macbookpro"],"links":[{"from":"hackintosh","to":"macbookpro","dir":"right"}]}})")
          .append('\n');

  CoordinationStatus status;
  QSignalSpy onlineSpy(&status, &CoordinationStatus::online);
  status.start(port, 60000);

  QElapsedTimer timer;
  timer.start();
  while (!server.hasPendingConnections() && timer.elapsed() < 3000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QTest::qWait(20);
  }
  QVERIFY(server.hasPendingConnections());

  QTcpSocket *client = server.nextPendingConnection();
  while (client->bytesAvailable() == 0 && timer.elapsed() < 3000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QTest::qWait(20);
  }
  QVERIFY(client->bytesAvailable() > 0);
  client->readAll();
  client->write(reply);
  client->waitForBytesWritten(3000);
  client->disconnectFromHost();

  for (int attempt = 0; attempt < 60 && onlineSpy.count() < 1; ++attempt) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QTest::qWait(50);
  }

  status.stop();

  QCOMPARE(onlineSpy.count(), 1);
  const auto args = onlineSpy.first();
  QCOMPARE(args.at(0).toString(), QStringLiteral("client"));
  QCOMPARE(args.at(1).toString(), QStringLiteral("hackintosh"));
  QVERIFY(args.at(2).toString().contains(QStringLiteral("hackintosh")));
}

void CoordinationStatusTests::poll_emitsOfflineWhenNoCoordinator()
{
  CoordinationStatus status;
  QSignalSpy offlineSpy(&status, &CoordinationStatus::offline);
  status.start(59999, 60000);

  for (int attempt = 0; attempt < 40 && offlineSpy.count() < 1; ++attempt) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QTest::qWait(50);
  }

  status.stop();
  QCOMPARE(offlineSpy.count(), 1);
}

QTEST_MAIN(CoordinationStatusTests)
