/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Symless Ltd.
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

void DaemonIpcServer::processCommand(QLocalSocket *clientSocket, const QString &command, const QStringList &parts)
{
  if (command == "logLevel") {
    processLogLevel(clientSocket, parts);
  } else if (command == "elevate") {
    processElevate(clientSocket, parts);
  } else if (command == "command") {
    processCommandMessage(clientSocket, parts);
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
    LOG_WARN("daemon ipc server got unknown command: %s", command.toUtf8().constData());
  }
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

void DaemonIpcServer::processCommandMessage(QLocalSocket *&clientSocket, const QStringList &messageParts)
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
