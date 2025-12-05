/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcServer.h"

#include "base/Log.h"

#include <QLocalServer>
#include <QLocalSocket>

namespace deskflow::core::ipc {

const auto kAckMessage = "ok";
const auto kErrorMessage = "error";

IpcServer::IpcServer(QObject *parent, const QString &serverName)
    : QObject(parent),
      m_serverName(serverName),
      m_server{new QLocalServer(this)} // NOSONAR - Qt memory
{
  // do nothing
}

IpcServer::~IpcServer()
{
  m_server->close();
}

void IpcServer::listen()
{
  // IPC server normally runs as system, but GUI runs as regular user, so we need to allow world access.
  m_server->setSocketOptions(QLocalServer::WorldAccessOption);

  connect(m_server, &QLocalServer::newConnection, this, &IpcServer::handleNewConnection);
  QLocalServer::removeServer(m_serverName);
  if (m_server->listen(m_serverName)) {
    LOG_DEBUG("ipc server listening on: %s", m_serverName.toUtf8().constData());
  } else {
    LOG_ERR("ipc server failed to listen on: %s", m_serverName.toUtf8().constData());
  }
}

void IpcServer::handleNewConnection()
{
  QLocalSocket *clientSocket = m_server->nextPendingConnection();
  if (!clientSocket) {
    LOG_ERR("ipc server failed to get new connection");
    return;
  }

  LOG_DEBUG("ipc server got new connection");
  m_clients.insert(clientSocket);

  connect(clientSocket, &QLocalSocket::readyRead, this, &IpcServer::handleReadyRead);
  connect(clientSocket, &QLocalSocket::disconnected, this, &IpcServer::handleDisconnected);
  connect(clientSocket, &QLocalSocket::errorOccurred, this, &IpcServer::handleErrorOccurred);
}

void IpcServer::handleReadyRead()
{
  const auto clientSocket = qobject_cast<QLocalSocket *>(sender());
  LOG_DEBUG1("ipc server ready to read data");

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
    const auto index = data.indexOf('\n');
    QByteArray messageData = data.left(index);
    data.remove(0, index + 1);
    QString message = QString::fromUtf8(messageData);
    processMessage(clientSocket, message);
  }
}

void IpcServer::handleDisconnected()
{
  const auto clientSocket = qobject_cast<QLocalSocket *>(sender());
  LOG_DEBUG("ipc server client disconnected");
  m_clients.remove(clientSocket);
  clientSocket->deleteLater();
}

void IpcServer::handleErrorOccurred()
{
  const auto clientSocket = qobject_cast<QLocalSocket *>(sender());
  LOG_ERR("ipc server client error: %s", clientSocket->errorString().toUtf8().constData());
  m_clients.remove(clientSocket);
  clientSocket->deleteLater();
}

void IpcServer::writeToClientSocket(QLocalSocket *&clientSocket, const QString &message) const
{
  QByteArray messageData = message.toUtf8() + '\n';
  qint64 bytesWritten = clientSocket->write(messageData);
  if (bytesWritten != messageData.size()) {
    LOG_ERR("ipc server failed to write full message to client socket");
  } else {
    LOG_DEBUG1("ipc server wrote message to client socket: %s", message.toUtf8().constData());
  }
}

} // namespace deskflow::core::ipc
