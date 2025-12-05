/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreIpcServer.h"

#include "base/Log.h"
#include "common/Constants.h"

#include <QLocalSocket>

namespace deskflow::core::ipc {

const auto kAckMessage = "ok";
const auto kErrorMessage = "error";

CoreIpcServer::CoreIpcServer(QObject *parent) : IpcServer(parent, kCoreIpcName)
{
  // do nothing
}

// TODO: abstract handling of message format, hello, and noop to base.
void CoreIpcServer::processMessage(QLocalSocket *clientSocket, const QString &message)
{
  LOG_DEBUG1("core ipc server got message: %s", message.toUtf8().constData());
  const auto parts = message.split('=');
  if (parts.size() < 1) {
    LOG_ERR("core ipc server got invalid message: %s", message.toUtf8().constData());
    writeToClientSocket(clientSocket, kErrorMessage);
    return;
  }

  const auto &command = parts[0];
  if (command == "hello") { // NOSONAR - if-init is confusing here
    LOG_DEBUG("core ipc server got hello message, sending hello back");
    writeToClientSocket(clientSocket, "hello");
  } else if (command == "noop") {
    LOG_DEBUG("core ipc server got noop message");
    writeToClientSocket(clientSocket, kAckMessage);
  } else {
    LOG_WARN("core ipc server got unknown message: %s", message.toUtf8().constData());
  }

  clientSocket->flush();
}

} // namespace deskflow::core::ipc
