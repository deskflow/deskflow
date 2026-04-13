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
    : IpcServer(parent, kDaemonIpcName, QStringLiteral("daemon")),
      m_logFilename(logFilename)
{
  // do nothing
}

void DaemonIpcServer::processCommand(QLocalSocket *clientSocket, const QString &command, const QStringList &parts)
{
  if (command == QStringLiteral("logLevel")) {
    processLogLevel(clientSocket, parts);
  } else if (command == QStringLiteral("configFile")) {
    processConfigFile(clientSocket, parts);
  } else if (command == QStringLiteral("start")) {
    LOG_DEBUG("daemon ipc server got start message");
    Q_EMIT startProcessRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == QStringLiteral("stop")) {
    LOG_DEBUG("daemon ipc server got stop message");
    Q_EMIT stopProcessRequested();
    writeToClientSocket(clientSocket, kAckMessage);
  } else if (command == QStringLiteral("logPath")) {
    LOG_DEBUG("daemon ipc server got log path request");
    writeToClientSocket(clientSocket, QStringLiteral("logPath=%1").arg(m_logFilename.toUtf8()));
  } else if (command == QStringLiteral("clearSettings")) {
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

  const auto &logLevel = messageParts.at(1);
  if (logLevel.isEmpty()) {
    LOG_ERR("daemon ipc server got empty log level");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("daemon ipc server got new log level: %s", logLevel.toUtf8().constData());
  Q_EMIT logLevelChanged(logLevel);
  writeToClientSocket(clientSocket, kAckMessage);
}

void DaemonIpcServer::processConfigFile(QLocalSocket *&clientSocket, const QStringList &messageParts)
{
  if (messageParts.size() < 2) {
    LOG_ERR("daemon ipc server got invalid config file message");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &configFile = messageParts.at(1);
  if (configFile.isEmpty()) {
    LOG_ERR("daemon ipc server got empty config file path");
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  LOG_DEBUG("daemon ipc server got config file: %s", configFile.toUtf8().constData());
  Q_EMIT configFileChanged(configFile);
  writeToClientSocket(clientSocket, kAckMessage);
}

} // namespace deskflow::core::ipc
