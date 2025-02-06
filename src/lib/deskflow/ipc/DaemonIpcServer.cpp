/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcServer.h"

#include "base/Log.h"
#include "common/constants.h"

#include <QLocalServer>
#include <QLocalSocket>

namespace deskflow::core::ipc {

DaemonIpcServer::DaemonIpcServer(QObject *parent) : QObject(parent), m_server{new QLocalServer(this)}
{
  // Daemon runs as system, but GUI runs as regular user, so we need to allow world access.
  m_server->setSocketOptions(QLocalServer::WorldAccessOption);

  connect(m_server, &QLocalServer::newConnection, this, &DaemonIpcServer::handleNewConnection);
  m_server->removeServer(kDaemonIpcName);
  if (m_server->listen(kDaemonIpcName)) {
    LOG_DEBUG("ipc server listening on: %s", kDaemonIpcName);
  } else {
    LOG_ERR("ipc server failed to listen on: %s", kDaemonIpcName);
  }
}

DaemonIpcServer::~DaemonIpcServer()
{
  m_server->close();
}

void DaemonIpcServer::handleNewConnection()
{
  QLocalSocket *clientSocket = m_server->nextPendingConnection();
  if (!clientSocket) {
    LOG_ERR("ipc server failed to get new connection");
    return;
  }

  LOG_DEBUG("ipc server got new connection");

  connect(clientSocket, &QLocalSocket::readyRead, this, [clientSocket]() {
    LOG_DEBUG("ipc server ready to read data");

    QByteArray data = clientSocket->readAll();
    if (data.isEmpty()) {
      LOG_WARN("ipc server got empty message");
      return;
    }

    QString dataStr = QString::fromUtf8(data);
    if (dataStr.isEmpty()) {
      LOG_ERR("ipc server failed to convert message to string");
      return;
    }

    if (dataStr == "hello") {
      LOG_DEBUG("ipc server got message: %s", data.constData());
      clientSocket->write("hello");
      clientSocket->flush();
    }
  });
}

} // namespace deskflow::core::ipc
