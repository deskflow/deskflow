/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcServer.h"

#include "base/Log.h"
#include "common/constants.h"

#include <QLocalServer>
#include <QLocalSocket>

namespace deskflow::core::ipc {

DaemonIpcServer::DaemonIpcServer(QObject *parent) : QObject(parent), m_server{new QLocalServer(this)}
{
  // Daemon runs as system, but GUI runs as regular user, so we need to allow world access.
  m_server->setSocketOptions(QLocalServer::WorldAccessOption);

  connect(m_server, &QLocalServer::newConnection, this, &DaemonIpcServer::handleNewConnection);
  m_server->removeServer(kDaemonIpcName);
  if (m_server->listen(kDaemonIpcName)) {
    LOG_DEBUG("ipc server listening on: %s", kDaemonIpcName);
  } else {
    LOG_ERR("ipc server failed to listen on: %s", kDaemonIpcName);
  }
}

DaemonIpcServer::~DaemonIpcServer()
{
  m_server->close();
}

void DaemonIpcServer::handleNewConnection()
{
  QLocalSocket *clientSocket = m_server->nextPendingConnection();
  if (!clientSocket) {
    LOG_ERR("ipc server failed to get new connection");
    return;
  }

  LOG_DEBUG("ipc server got new connection");
  m_clients.insert(clientSocket);

  connect(clientSocket, &QLocalSocket::readyRead, this, &DaemonIpcServer::handleReadyRead);
  connect(clientSocket, &QLocalSocket::disconnected, this, &DaemonIpcServer::handleDisconnected);
  connect(clientSocket, &QLocalSocket::errorOccurred, this, &DaemonIpcServer::handleErrorOccurred);
}

void DaemonIpcServer::handleReadyRead()
{
  const auto clientSocket = qobject_cast<QLocalSocket *>(sender());
  LOG_DEBUG("ipc server ready to read data");

  QByteArray data = clientSocket->readAll();
  if (data.isEmpty()) {
    LOG_WARN("ipc server got empty message");
    return;
  }

  // we don't handle incomplete messages yet; each socket read must have delimiters.
  if (!data.contains('\n')) {
    LOG_WARN("ipc server got incomplete message: %s", data.constData());
    return;
  }

  // each message is delimited by a newline to keep the protocol super simple.
  while (data.contains('\n')) {
    int index = data.indexOf('\n');
    QByteArray messageData = data.left(index);
    data.remove(0, index + 1);
    QString message = QString::fromUtf8(messageData);
    processMessage(clientSocket, message);
  }
}

void DaemonIpcServer::handleDisconnected()
{
  const auto clientSocket = qobject_cast<QLocalSocket *>(sender());
  LOG_DEBUG("ipc server client disconnected");
  m_clients.remove(clientSocket);
  clientSocket->deleteLater();
}

void DaemonIpcServer::handleErrorOccurred()
{
  const auto clientSocket = qobject_cast<QLocalSocket *>(sender());
  LOG_ERR("ipc server client error: %s", clientSocket->errorString().toUtf8().constData());
  m_clients.remove(clientSocket);
  clientSocket->deleteLater();
}

void DaemonIpcServer::processMessage(QLocalSocket *clientSocket, const QString &message)
{
  const auto kAckMessage = "ok\n";
  const auto kErrorMessage = "error\n";

  LOG_DEBUG("ipc server got message: %s", message.toUtf8().constData());
  if (message == "hello") {
    clientSocket->write("hello\n");
  } else if (message.startsWith("elevate=")) {
    const auto elevateMode = message.split('=')[1].toInt();
    if (elevateMode < 0 || elevateMode > 2) {
      LOG_ERR("ipc server got invalid elevate mode: %d", elevateMode);
      clientSocket->write(kErrorMessage);
      return;
    } else {
      LOG_DEBUG("ipc server got new elevate mode: %d", elevateMode);
      Q_EMIT elevateModeChanged(elevateMode);
      clientSocket->write(kAckMessage);
    }
  } else if (message.startsWith("command=")) {
    const auto command = message.split('=')[1];
    if (command.isEmpty()) {
      LOG_ERR("ipc server got empty command");
      clientSocket->write(kErrorMessage);
    } else {
      LOG_DEBUG("ipc server got new command: %s", command.toUtf8().constData());
      Q_EMIT commandChanged(command);
      clientSocket->write(kAckMessage);
    }
  } else if (message == "restart") {
    LOG_DEBUG("ipc server got restart message");
    Q_EMIT restartRequested();
    clientSocket->write(kAckMessage);
  } else {
    LOG_WARN("ipc server got unknown message: %s", message.toUtf8().constData());
  }
  clientSocket->flush();
}

} // namespace deskflow::core::ipc
