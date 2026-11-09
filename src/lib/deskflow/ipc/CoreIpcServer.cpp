/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreIpcServer.h"

#include "CoreIpc.h"
#include "base/Log.h"
#include "common/Constants.h"
#include "common/IsolatedSession.h"
#include "common/Settings.h"
#include "common/WorkspaceApply.h"
#include "deskflow/App.h"

#include <QLocalSocket>

namespace deskflow::core::ipc {

static CoreIpcServer *s_instance = nullptr;

CoreIpcServer::CoreIpcServer(QObject *parent)
    : IpcServer(
          parent,
          deskflow::lab::IsolatedSession::active() ? deskflow::lab::IsolatedSession::active()->ipcName
                                                   : QString(kCoreIpcName),
          QStringLiteral("core")
      )
{
  assert(s_instance == nullptr);
  s_instance = this;
}

CoreIpcServer &CoreIpcServer::instance()
{
  assert(s_instance != nullptr);
  return *s_instance;
}

void CoreIpcServer::processCommand(QLocalSocket *clientSocket, const QString &command, const QStringList &parts)
{
  Q_UNUSED(parts)
  if (command == QStringLiteral("applyWorkspace")) {
    if (parts.size() != 2 ||
        Settings::value(Settings::Core::CoreMode).value<Settings::CoreMode>() != Settings::CoreMode::Server)
      return;
    const auto bytes = parts.at(1).toUtf8();
    if (!deskflow::display::WorkspaceApplyRequest::decode(bytes))
      return;
    auto *events = App::instance().getEvents();
    events->addEvent(
        Event(EventTypes::ServerAppApplyWorkspace, events->getSystemTarget(), new WorkspaceApplyEvent(bytes))
    );
    return;
  }
  if (command == QStringLiteral("stop")) {
    LOG_DEBUG("core ipc server got stop message");
    writeToClientSocket(clientSocket, QStringLiteral("ok"));
    broadcastCommand(QStringLiteral("bye"));
    Q_EMIT stopProcessRequested();
    return;
  }
  LOG_WARN("core ipc server got unknown command: %s", command.toUtf8().constData());
}

} // namespace deskflow::core::ipc
