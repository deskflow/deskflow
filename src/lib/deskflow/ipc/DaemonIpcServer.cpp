/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcServer.h"

#include "base/Log.h"
#include "common/Constants.h"

#include <QLocalSocket>

namespace deskflow::core::ipc {

const auto kAckMessage = "ok";
const auto kErrorMessage = "error";

DaemonIpcServer::DaemonIpcServer(QObject *parent, const QString &logFilename)
    : IpcServer(parent, kDaemonIpcName),
      m_logFilename(logFilename)
{
  // do nothing
}

void DaemonIpcServer::processMessage(QLocalSocket *clientSocket, const QString &message)
{
  LOG_DEBUG1("daemon ipc server got message: %s", message.toUtf8().constData());
  const auto parts = message.split('=');
  if (parts.size() < 1) {
    LOG_ERR("daemon ipc server got invalid message: %s", message.toUtf8().constData());
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &command = parts[0];
  if (command == "hello") { // NOSONAR - if-init is confusing here
    LOG_DEBUG("daemon ipc server got hello message, sending hello back");
    writeToClientSocket(clientSocket, "hello");
  } else if (command == "noop") {
    LOG_DEBUG("daemon ipc server got noop message");
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == "logLevel") {
    processLogLevel(clientSocket, parts);
  } else if (command == "elevate") {
    processElevate(clientSocket, parts);
  } else if (command == "command") {
    processCommand(clientSocket, parts);
  } else if (command == "start") {
    LOG_DEBUG("daemon ipc server got start message");
    Q_EMIT startProcessRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == "stop") {
    LOG_DEBUG("daemon ipc server got stop message");
    Q_EMIT stopProcessRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == "logPath") {
    LOG_DEBUG("daemon ipc server got log path request");
    writeToClientSocket(clientSocket, "logPath=" + m_logFilename.toUtf8());
  } else if (command == "clearSettings") {
    LOG_DEBUG("daemon ipc server got clear settings message");
    Q_EMIT clearSettingsRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else {
    LOG_WARN("daemon ipc server got unknown message: %s", message.toUtf8().constData());
  }

  clientSocket->flush();
}

void DaemonIpcServer::processLogLevel(QLocalSocket *&clientSocket, const QStringList &messageParts)
{
  if (messageParts.size() < 2) {
    LOG_ERR("daemon ipc server got invalid log level message");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &logLevel = messageParts[1];
  if (logLevel.isEmpty()) {
    LOG_ERR("daemon ipc server got empty log level");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("daemon ipc server got new log level: %s", logLevel.toUtf8().constData());
  Q_EMIT logLevelChanged(logLevel);
  writeToClientSocket(clientSocket, kAckMessage);
}

void DaemonIpcServer::processElevate(QLocalSocket *&clientSocket, const QStringList &messageParts)
{
  if (messageParts.size() < 2) {
    LOG_ERR("daemon ipc server got invalid elevate message");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &elevate = messageParts[1];
  if (elevate != "yes" && elevate != "no") {
    LOG_ERR("daemon ipc server got invalid elevate value: %s", elevate.toUtf8().constData());
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("daemon ipc server got new elevate value: %s", elevate.toUtf8().constData());
  Q_EMIT elevateModeChanged(elevate == "yes");
  writeToClientSocket(clientSocket, kAckMessage);
}

void DaemonIpcServer::processCommand(QLocalSocket *&clientSocket, const QStringList &messageParts)
{
  if (messageParts.size() < 2) {
    LOG_ERR("daemon ipc server got invalid command message");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &command = messageParts[1];
  if (command.isEmpty()) {
    LOG_ERR("daemon ipc server got empty command");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("daemon ipc server got new command: %s", command.toUtf8().constData());
  Q_EMIT commandChanged(command);
  writeToClientSocket(clientSocket, kAckMessage);
}

} // namespace deskflow::core::ipc
