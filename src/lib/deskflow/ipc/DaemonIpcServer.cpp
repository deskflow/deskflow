/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcServer.h"

#include "base/Log.h"
#include "common/Constants.h"

#include <QLocalServer>
#include <QLocalSocket>

namespace deskflow::core::ipc {

const auto kAckMessage = "ok";
const auto kErrorMessage = "error";

DaemonIpcServer::DaemonIpcServer(QObject *parent, const QString &logFilename)
    : QObject(parent),
      m_logFilename(logFilename),
      m_server{new QLocalServer(this)} // NOSONAR - Qt memory
{
  // do nothing
}

DaemonIpcServer::~DaemonIpcServer()
{
  m_server->close();
}

void DaemonIpcServer::listen()
{
  // Daemon runs as system, but GUI runs as regular user, so we need to allow world access.
  m_server->setSocketOptions(QLocalServer::WorldAccessOption);

  connect(m_server, &QLocalServer::newConnection, this, &DaemonIpcServer::handleNewConnection);
  QLocalServer::removeServer(kDaemonIpcName);
  if (m_server->listen(kDaemonIpcName)) {
    LOG_DEBUG("ipc server listening on: %s", kDaemonIpcName);
  } else {
    LOG_ERR("ipc server failed to listen on: %s", kDaemonIpcName);
  }
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
  LOG_DEBUG1("ipc server got message: %s", message.toUtf8().constData());
  const auto parts = message.split('=');
  if (parts.size() < 1) {
    LOG_ERR("ipc server got invalid message: %s", message.toUtf8().constData());
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &command = parts[0];
  if (command == "hello") { // NOSONAR - if-init is confusing here
    LOG_DEBUG("ipc server got hello message, sending hello back");
    writeToClientSocket(clientSocket, "hello");
  } else if (command == "noop") {
    LOG_DEBUG("ipc server got noop message");
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == "logLevel") {
    processLogLevel(clientSocket, parts);
  } else if (command == "elevate") {
    processElevate(clientSocket, parts);
  } else if (command == "command") {
    processCommand(clientSocket, parts);
  } else if (command == "start") {
    LOG_DEBUG("ipc server got start message");
    Q_EMIT startProcessRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == "stop") {
    LOG_DEBUG("ipc server got stop message");
    Q_EMIT stopProcessRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == "logPath") {
    LOG_DEBUG("ipc server got log path request");
    writeToClientSocket(clientSocket, "logPath=" + m_logFilename.toUtf8());
  } else if (command == "clearSettings") {
    LOG_DEBUG("ipc server got clear settings message");
    Q_EMIT clearSettingsRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else {
    LOG_WARN("ipc server got unknown message: %s", message.toUtf8().constData());
  }

  clientSocket->flush();
}

void DaemonIpcServer::processLogLevel(QLocalSocket *&clientSocket, const QStringList &messageParts)
{
  if (messageParts.size() < 2) {
    LOG_ERR("ipc server got invalid log level message");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &logLevel = messageParts[1];
  if (logLevel.isEmpty()) {
    LOG_ERR("ipc server got empty log level");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("ipc server got new log level: %s", logLevel.toUtf8().constData());
  Q_EMIT logLevelChanged(logLevel);
  writeToClientSocket(clientSocket, kAckMessage);
}

void DaemonIpcServer::processElevate(QLocalSocket *&clientSocket, const QStringList &messageParts)
{
  if (messageParts.size() < 2) {
    LOG_ERR("ipc server got invalid elevate message");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &elevate = messageParts[1];
  if (elevate != "yes" && elevate != "no") {
    LOG_ERR("ipc server got invalid elevate value: %s", elevate.toUtf8().constData());
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("ipc server got new elevate value: %s", elevate.toUtf8().constData());
  Q_EMIT elevateModeChanged(elevate == "yes");
  writeToClientSocket(clientSocket, kAckMessage);
}

void DaemonIpcServer::processCommand(QLocalSocket *&clientSocket, const QStringList &messageParts)
{
  if (messageParts.size() < 2) {
    LOG_ERR("ipc server got invalid command message");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &command = messageParts[1];
  if (command.isEmpty()) {
    LOG_ERR("ipc server got empty command");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("ipc server got new command: %s", command.toUtf8().constData());
  Q_EMIT commandChanged(command);
  writeToClientSocket(clientSocket, kAckMessage);
}

void DaemonIpcServer::writeToClientSocket(QLocalSocket *&clientSocket, const QString &message) const
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
