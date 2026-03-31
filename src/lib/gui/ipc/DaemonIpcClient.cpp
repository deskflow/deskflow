/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcClient.h"

#include "common/Constants.h"

#include <QDebug>

namespace deskflow::gui::ipc {

DaemonIpcClient::DaemonIpcClient(QObject *parent) : IpcClient(parent, kDaemonIpcName, "daemon")
{
}

void DaemonIpcClient::sendLogLevel(const QString &logLevel)
{
  sendMessage("logLevel=" + logLevel);
}

void DaemonIpcClient::sendStartProcess(const QString &command, bool elevate)
{
  const auto elevateStr = elevate ? QStringLiteral("yes") : QStringLiteral("no");
  sendMessage("elevate=" + elevateStr);
  sendMessage("command=" + command);
  sendMessage("start");
}

void DaemonIpcClient::sendStopProcess()
{
  sendMessage("stop");
}

void DaemonIpcClient::sendClearSettings()
{
  sendMessage("clearSettings");
}

void DaemonIpcClient::requestLogPath()
{
  sendMessage("logPath");
}

void DaemonIpcClient::processCommand(const QString &command, const QStringList &parts)
{
  if (command == "logPath" && parts.size() == 2) {
    Q_EMIT logPathReceived(parts[1]);
  }
}

} // namespace deskflow::gui::ipc
