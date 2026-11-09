/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcClient.h"

#include "common/Constants.h"

namespace deskflow::gui::ipc {

DaemonIpcClient::DaemonIpcClient(QObject *parent) : IpcClient(parent, kDaemonIpcName, QStringLiteral("daemon"))
{
}

void DaemonIpcClient::sendLogLevel(const QString &logLevel)
{
  sendMessage(QStringLiteral("logLevel=%1").arg(logLevel));
}

void DaemonIpcClient::sendConfigFile(const QString &configFile)
{
  sendMessage(QStringLiteral("configFile=%1").arg(configFile));
}

void DaemonIpcClient::sendPeerSession(const QString &encoded)
{
  sendMessage("startPeer=" + encoded);
}
void DaemonIpcClient::sendPeerStop(const QString &token)
{
  sendMessage("stopPeer=" + token);
}
void DaemonIpcClient::sendStartProcess()
{
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
  if (command == "peerStopped" && parts.size() == 2) {
    Q_EMIT peerStopped(parts[1]);
    return;
  }
  if (command == "peerRejected" && parts.size() == 2) {
    Q_EMIT peerSessionRejected(parts[1]);
    return;
  }
  if (command == QStringLiteral("logPath") && parts.size() == 2) {
    Q_EMIT logPathReceived(parts.at(1));
  }
}

} // namespace deskflow::gui::ipc
