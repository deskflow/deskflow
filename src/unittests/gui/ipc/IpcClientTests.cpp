/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcClientTests.h"

#include "gui/ipc/IpcClient.h"

#include <QEventLoop>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <QTimer>

using namespace deskflow::gui::ipc;

class SelfDeletingIpcClient : public IpcClient
{
public:
  SelfDeletingIpcClient(const QString &socketName, QStringList &commands)
      : IpcClient(nullptr, socketName, QStringLiteral("test")),
        m_commands(commands)
  {
  }

protected:
  void processCommand(const QString &command, const QStringList &parts) override
  {
    Q_UNUSED(parts)
    m_commands.append(command);

    // Mimics a modal dialog whose event loop runs a deleteLater() on this client.
    if (command == QStringLiteral("first")) {
      QEventLoop loop;
      QTimer::singleShot(0, this, [this] { deleteLater(); });
      QTimer::singleShot(50, &loop, &QEventLoop::quit);
      loop.exec();
    }
  }

private:
  QStringList &m_commands;
};

void IpcClientTests::destroyedWhileHandlingMessages()
{
  const auto socketName = QStringLiteral("deskflow-ipc-client-test-%1").arg(QCoreApplication::applicationPid());
  QLocalServer server;
  QVERIFY(server.listen(socketName));

  QStringList commands;
  QPointer<IpcClient> client = new SelfDeletingIpcClient(socketName, commands);
  client->connectToServer();

  QVERIFY(server.waitForNewConnection(5000));
  auto *serverSocket = server.nextPendingConnection();
  serverSocket->write("hello=1\nfirst\nsecond\n");
  serverSocket->flush();

  QTRY_VERIFY(client.isNull());
  QCOMPARE(commands, QStringList{QStringLiteral("first")});
}

QTEST_MAIN(IpcClientTests)
