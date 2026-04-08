/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcClient.h"

#include "common/Constants.h"

#include <QDebug>

namespace deskflow::gui::ipc {

DaemonIpcClient::DaemonIpcClient(QObject *parent) : IpcClient(parent, kDaemonIpcName, QStringLiteral("daemon"))
{
}

void DaemonIpcClient::sendLogLevel(const QString &logLevel)
{
  sendMessage(QStringLiteral("logLevel=%1").arg(logLevel));
}

void DaemonIpcClient::sendStartProcess(const QString &command, bool elevate)
{
  const auto elevateStr = elevate ? QStringLiteral("yes") : QStringLiteral("no");
  sendMessage(QStringLiteral("elevate=%1").arg(elevateStr));
  sendMessage(QStringLiteral("command=%1").arg(command));
  sendMessage(QStringLiteral("start"));
}

void DaemonIpcClient::sendStopProcess()
{
  sendMessage(QStringLiteral("stop"));
}

void DaemonIpcClient::sendClearSettings()
{
  sendMessage(QStringLiteral("clearSettings"));
}

void DaemonIpcClient::requestLogPath()
{
  sendMessage(QStringLiteral("logPath"));
}

void DaemonIpcClient::processCommand(const QString &command, const QStringList &parts)
{
  if (command == QStringLiteral("logPath") && parts.size() == 2) {
    Q_EMIT logPathReceived(parts[1]);
  }
}

} // namespace deskflow::gui::ipc
