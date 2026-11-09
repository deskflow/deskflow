/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreIpcClient.h"

#include "common/Constants.h"
#include "common/IsolatedSession.h"

#include <QString>

namespace deskflow::gui::ipc {

CoreIpcClient::CoreIpcClient(QObject *parent)
    : IpcClient(
          parent,
          deskflow::lab::IsolatedSession::active() ? deskflow::lab::IsolatedSession::active()->ipcName
                                                   : QString(kCoreIpcName),
          QStringLiteral("core")
      )
{
  deskflow::lab::IsolatedSession::sealStartup();
}

void CoreIpcClient::sendStop()
{
  sendMessage(QStringLiteral("stop"));
}

bool CoreIpcClient::sendWorkspaceApply(const QByteArray &request)
{
  if (!isConnected() || request.size() > 14000 || request.contains('\n'))
    return false;
  sendMessage(QStringLiteral("applyWorkspace=") + QString::fromUtf8(request));
  return true;
}

void CoreIpcClient::processCommand(const QString &command, const QStringList &parts)
{
  const auto args = parts.size() >= 2 ? parts.at(1) : QString();
  Q_EMIT commandReceived(command, args);
}

} // namespace deskflow::gui::ipc
