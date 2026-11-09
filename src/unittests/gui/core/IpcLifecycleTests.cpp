// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/IsolatedSession.h"
#include "common/PeerAuthentication.h"
#include "deskflow/ipc/CoreIpcServer.h"
#include "gui/ipc/CoreIpcClient.h"
#include "gui/ipc/IpcClient.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
namespace deskflow::gui::ipc {
class IpcLifecycleTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void realLocalSocketHandshakeAndFragmentedCommands()
  {
    QTemporaryDir dir(QDir::tempPath() + "/deskflow-lab-ipc-XXXXXX");
    QVERIFY(dir.isValid());
    QLocalServer server;
    QVERIFY2(server.listen(dir.path() + "/socket"), qPrintable(server.errorString()));
    IpcClient client(nullptr, server.fullServerName(), "fixture");
    QSignalSpy connected(&client, &IpcClient::connected);
    client.beginConnection();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 1000);
    auto *peer = server.nextPendingConnection();
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, 1000);
    QVERIFY(peer->readAll().startsWith("hello="));
    peer->write("hel");
    peer->flush();
    QTest::qWait(10);
    QCOMPARE(connected.count(), 0);
    peer->write("lo=synthetic\n");
    peer->flush();
    QTRY_COMPARE_WITH_TIMEOUT(connected.count(), 1, 1000);
    QVERIFY(client.isConnected());
    client.disconnectFromServer();
    QVERIFY(!client.isConnected());
  }
  void stopCancelsQueuedRetryBeforeServerAppears()
  {
    QTemporaryDir dir(QDir::tempPath() + "/deskflow-lab-ipc-XXXXXX");
    QVERIFY(dir.isValid());
    IpcClient client(nullptr, dir.path() + "/socket", "fixture");
    client.beginConnection(); // Absent socket schedules a zero-delay retry.
    client.disconnectFromServer();
    QLocalServer server;
    QVERIFY(server.listen(dir.path() + "/socket"));
    QTest::qWait(30);
    QVERIFY(!server.hasPendingConnections());
    QVERIFY(!client.m_shouldConnect);
  }
  void oldRetryCannotReachNewEndpoint()
  {
    QTemporaryDir dir(QDir::tempPath() + "/deskflow-lab-ipc-XXXXXX");
    QVERIFY(dir.isValid());
    IpcClient client(nullptr, dir.path() + "/old", "fixture");
    client.beginConnection();
    client.disconnectFromServer();
    QLocalServer server;
    QVERIFY(server.listen(dir.path() + "/new"));
    client.m_socketName = server.fullServerName();
    client.beginConnection();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 1000);
    auto *peer = server.nextPendingConnection();
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, 1000);
    const auto hello = peer->readAll();
    QCOMPARE(hello.count("hello="), 1);
    QTest::qWait(30);
    QVERIFY(!server.hasPendingConnections());
    client.disconnectFromServer();
  }
  void oversizedReadFailsClosed()
  {
    QTemporaryDir dir(QDir::tempPath() + "/deskflow-lab-ipc-XXXXXX");
    QVERIFY(dir.isValid());
    QLocalServer server;
    QVERIFY(server.listen(dir.path() + "/socket"));
    IpcClient client(nullptr, server.fullServerName(), "fixture");
    QSignalSpy failure(&client, &IpcClient::connectionFailed);
    client.beginConnection();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 1000);
    auto *peer = server.nextPendingConnection();
    peer->write(QByteArray(70000, 'x'));
    peer->flush();
    QTRY_VERIFY_WITH_TIMEOUT(failure.count() > 0, 1000);
    QVERIFY(!client.isConnected());
    QVERIFY(client.m_readBuffer.isEmpty());
  }
  void actualCoreIpcEndpointsUseSessionNamespaceWithoutListening()
  {
    QTemporaryDir temp(QDir::tempPath() + "/deskflow-lab-ipc-XXXXXX");
    QVERIFY(temp.isValid());
    const auto parent = QFileInfo(temp.path()).canonicalFilePath();
    const auto id = QUuid::createUuid().toString(QUuid::Id128);
    const auto root = parent + '/' + id;
    for (const auto *d : {"config/DeskflowLab/tls", "runtime", "state", "logs"})
      QVERIFY(QDir().mkpath(root + '/' + d));
    QFile file(root + "/session.json");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(
                   QJsonObject{
                       {"schema", 1},
                       {"purpose", "deskflow-lab-network"},
                       {"session", id},
                       {"mode", "client"},
                       {"bindAddress", "127.0.0.1"},
                       {"peerAddress", "127.0.0.1"},
                       {"port", 34800},
                       {"tls", true},
                       {"peerChecks", true}
                   }
    ).toJson());
    file.close();
    QVERIFY(deskflow::lab::IsolatedSession::activate(root, parent));
    const auto expected = "deskflow-lab-ipc-" + id;
    CoreIpcClient client;
    QCOMPARE(client.m_socketName, expected);
    QVERIFY(!client.m_shouldConnect);
    deskflow::core::ipc::CoreIpcServer server(nullptr);
    QCOMPARE(server.socketName(), expected);
    auto *native = server.findChild<QLocalServer *>();
    QVERIFY(native);
    QVERIFY(!native->isListening());
    QCOMPARE(native->socketOptions(), QLocalServer::SocketOptions(QLocalServer::UserAccessOption));
    QVERIFY(!deskflow::lab::IsolatedSession::activate(root, parent));
  }
  void structuredAuthenticationSurvivesFragmentedCoreIpc()
  {
    using namespace deskflow::auth;
    QTemporaryDir dir(QDir::tempPath() + "/deskflow-lab-ipc-XXXXXX");
    QVERIFY(dir.isValid());
    QLocalServer server;
    QVERIFY(server.listen(dir.path() + "/socket"));
    CoreIpcClient client;
    client.m_socketName = server.fullServerName();
    QSignalSpy messages(&client, &CoreIpcClient::commandReceived);
    client.beginConnection();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 1000);
    auto *peer = server.nextPendingConnection();
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, 1000);
    peer->readAll();
    const Context context{QString(32, '1'), QString(64, 'a'), 7};
    Gate gate;
    QVERIFY(gate.begin(context));
    const Report started{context, 4, Outcome::Started, {}};
    const Report verified{context, 4, Outcome::Verified, QString(64, 'a')};
    const auto packet =
        ("hello=synthetic\npeerAuthentication=" + started.encode() + "\npeerAuthentication=" + verified.encode() + "\n")
            .toUtf8();
    const auto split = packet.size() - 8;
    peer->write(packet.left(split));
    peer->flush();
    QTRY_COMPARE_WITH_TIMEOUT(messages.count(), 1, 1000);
    peer->write(packet.mid(split));
    peer->flush();
    QTRY_COMPARE_WITH_TIMEOUT(messages.count(), 2, 1000);
    for (const auto &message : messages) {
      QCOMPARE(message.at(0).toString(), QString("peerAuthentication"));
      const auto proof = Report::decode(message.at(1).toString());
      QVERIFY(proof);
      QCOMPARE(proof->context, context);
      QVERIFY(gate.consume(*proof));
    }
    gate.cancel();
    QVERIFY(!gate.consume(verified));
    client.disconnectFromServer();
  }
};
} // namespace deskflow::gui::ipc
QTEST_GUILESS_MAIN(deskflow::gui::ipc::IpcLifecycleTests)
#include "IpcLifecycleTests.moc"
