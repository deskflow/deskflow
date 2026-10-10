/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcClient.h"

#include "common/VersionInfo.h"

#include <QDebug>
#include <QLocalSocket>
#include <QTimer>

namespace deskflow::gui::ipc {

IpcClient::IpcClient(QObject *parent, const QString &socketName, const QString &typeName)
    : QObject(parent),
      m_socket{new QLocalSocket(this)},
      m_retryTimer{new QTimer(this)},
      m_socketName(socketName), // NOSONAR - Qt memory
      m_typeName(typeName)
{
  m_retryTimer->setSingleShot(true);
  m_retryTimer->setInterval(kConnectRetryDelay);
  connect(m_retryTimer, &QTimer::timeout, this, &IpcClient::attemptConnection);

  connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::handleDisconnected);
  connect(m_socket, &QLocalSocket::errorOccurred, this, &IpcClient::handleErrorOccurred);
  connect(m_socket, &QLocalSocket::readyRead, this, &IpcClient::handleReadyRead);
}

void IpcClient::connectToServer()
{
  if (m_state == State::Connecting) {
    qWarning().noquote() << QStringLiteral("%1 ipc client already connecting to server").arg(m_typeName);
    return;
  }

  if (m_state != State::Unconnected) {
    qDebug().noquote() << QStringLiteral("%1 ipc client not in unconnected state, disconnecting").arg(m_typeName);
    disconnectFromServer();
  }

  if (m_socket->state() != QLocalSocket::UnconnectedState) {
    qWarning().noquote(
    ) << QStringLiteral("%1 ipc client socket not in unconnected state, disconnecting").arg(m_typeName);
    disconnectFromServer();
  }

  m_retryTimer->stop();
  m_retryCount = 0;
  attemptConnection();
}

void IpcClient::attemptConnection()
{
  if (m_retryCount >= kConnectRetryLimit) {
    qWarning().noquote() << QStringLiteral("%1 ipc client failed to connect after %2 attempts")
                                .arg(m_typeName, QString::number(kConnectRetryLimit));
    m_state = State::Unconnected;
    Q_EMIT connectionFailed();
    return;
  }

  if (m_retryCount == 0) {
    qDebug().noquote() << QStringLiteral("%1 ipc client connecting to server: %2").arg(m_typeName, m_socketName);
  } else {
    qDebug().noquote() << QStringLiteral("%1 ipc client retrying connection, attempt: %2")
                              .arg(m_typeName, QString::number(m_retryCount + 1));
  }

  m_state = State::Connecting;
  m_retryCount++;

  // Each attempt's handlers are single shot, so a failure leaves its hello sender and a success its retry handler.
  disconnect(m_socket, &QLocalSocket::connected, this, nullptr);
  connect(
      m_socket, &QLocalSocket::connected, this,
      [this] {
        disconnect(m_attemptFailedConnection);
        const auto versionId = QStringLiteral("%1+%2").arg(kVersion, kVersionGitSha);
        m_socket->write(QStringLiteral("hello=%1\n").arg(versionId).toUtf8());
        qDebug().noquote() << QStringLiteral("%1 ipc client sent hello with version: %2").arg(m_typeName, versionId);
      },
      Qt::SingleShotConnection
  );

  m_attemptFailedConnection = connect(
      m_socket, &QLocalSocket::errorOccurred, this,
      [this] {
        qWarning().noquote(
        ) << QStringLiteral("%1 ipc client failed to connect: %2").arg(m_typeName, m_socket->errorString());
        m_socket->disconnectFromServer();
        m_state = State::Unconnected;
        m_retryTimer->start();
      },
      Qt::SingleShotConnection
  );

  m_socket->connectToServer(m_socketName);
}

void IpcClient::disconnectFromServer()
{
  m_retryTimer->stop();
  m_state = State::Disconnecting;
  qDebug().noquote() << QStringLiteral("%1 ipc client disconnecting from server").arg(m_typeName);
  m_socket->disconnectFromServer();
  m_state = State::Unconnected;
}

void IpcClient::handleDisconnected()
{
  if (m_state == State::Connecting) {
    return;
  }

  qDebug().noquote() << QStringLiteral("%1 ipc client disconnected from server").arg(m_typeName);
  const auto wasConnected = m_state == State::Connected;
  m_state = State::Unconnected;

  if (wasConnected) {
    Q_EMIT connectionFailed();
  }
}

void IpcClient::handleErrorOccurred()
{
  if (m_state == State::Connecting) {
    return;
  }

  qWarning().noquote() << QStringLiteral("%1 ipc client error: %2").arg(m_typeName, m_socket->errorString());

  if (m_state == State::Connected) {
    disconnectFromServer();
    Q_EMIT connectionFailed();
  }
}

void IpcClient::handleReadyRead()
{
  QByteArray data = m_readBuffer + m_socket->readAll();
  m_readBuffer.clear();

  while (data.contains('\n')) {
    const auto index = data.indexOf('\n');
    const auto message = QString::fromUtf8(data.left(index));
    data.remove(0, index + 1);

    qDebug().noquote() << QStringLiteral("%1 ipc client message: %2").arg(m_typeName, message);
    const auto parts = message.split('=');
    if (parts.isEmpty()) {
      qWarning().noquote() << QStringLiteral("%1 ipc client got invalid message: %2").arg(m_typeName, message);
      continue;
    }

    if (m_state == State::Connecting) {
      handleHandshakeMessage(parts);
      continue;
    }

    if (parts.at(0) == QStringLiteral("bye")) {
      qDebug().noquote() << QStringLiteral("%1 ipc server is shutting down").arg(m_typeName);
      disconnectFromServer();
      Q_EMIT serverShutdown();
      return;
    }

    processCommand(parts.at(0), parts);
  }

  if (!data.isEmpty()) {
    m_readBuffer = data;
  }
}

void IpcClient::handleHandshakeMessage(const QStringList &parts)
{
  if (parts.at(0) == QStringLiteral("error")) {
    const auto detail = parts.size() >= 2 ? parts.at(1) : QStringLiteral("unknown");
    qCritical().noquote() << QStringLiteral("%1 ipc server rejected connection: %2").arg(m_typeName, detail);
    disconnectFromServer();
    Q_EMIT connectionFailed();
    return;
  }

  const auto versionId = QStringLiteral("%1+%2").arg(kVersion, kVersionGitSha);

  if (parts.at(0) == QStringLiteral("versionMismatch")) {
    const auto serverVersion = parts.size() >= 2 ? parts.at(1) : QStringLiteral("unknown");
    qWarning().noquote(
    ) << QStringLiteral("%1 ipc version mismatch (client: %2, server: %3)").arg(m_typeName, versionId, serverVersion);
    m_state = State::Connected;
    Q_EMIT versionMismatch();
    return;
  }

  if (parts.at(0) != QStringLiteral("hello")) {
    qDebug().noquote(
    ) << QStringLiteral("%1 ipc client dropped message during handshake: %2").arg(m_typeName, parts.join('='));
    return;
  }

  if (parts.size() < 2) {
    qCritical().noquote() << QStringLiteral("%1 ipc server hello missing version").arg(m_typeName);
    disconnectFromServer();
    Q_EMIT connectionFailed();
    return;
  }

  m_state = State::Connected;
  qDebug().noquote() << QStringLiteral("%1 ipc client connected").arg(m_typeName);
  Q_EMIT connected();
}

void IpcClient::sendMessage(const QString &message)
{
  if (m_state != State::Connected) {
    qWarning().noquote() << QStringLiteral("%1 cannot send command, ipc client not connected").arg(m_typeName);
    return;
  }

  m_socket->write(message.toUtf8() + "\n");
  qDebug().noquote() << QStringLiteral("%1 ipc client sent message: %2").arg(m_typeName, message);
}

} // namespace deskflow::gui::ipc
