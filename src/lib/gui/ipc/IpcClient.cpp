/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcClient.h"

#include "common/VersionInfo.h"

#include <QDebug>
#include <QLocalSocket>
#include <QTimer>

namespace deskflow::gui::ipc {

IpcClient::IpcClient(QObject *parent, const QString &socketName)
    : QObject(parent),
      m_socket{new QLocalSocket(this)},
      m_socketName(socketName) // NOSONAR - Qt memory
{
  connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::handleDisconnected);
  connect(m_socket, &QLocalSocket::errorOccurred, this, &IpcClient::handleErrorOccurred);
  connect(m_socket, &QLocalSocket::readyRead, this, &IpcClient::handleReadyRead);
}

void IpcClient::connectToServer()
{
  if (m_state == State::Connecting) {
    qWarning() << "ipc client already connecting to server";
    return;
  }

  if (m_state != State::Unconnected) {
    qDebug() << "ipc client not in unconnected state, disconnecting";
    disconnectFromServer();
  }

  if (m_socket->state() != QLocalSocket::UnconnectedState) {
    qWarning() << "ipc client socket not in unconnected state, disconnecting";
    disconnectFromServer();
  }

  m_retryCount = 0;
  attemptConnection();
}

void IpcClient::attemptConnection()
{
  const auto kRetryLimit = 3;

  if (m_retryCount >= kRetryLimit) {
    qWarning() << "ipc client failed to connect after" << kRetryLimit << "attempts";
    m_state = State::Unconnected;
    Q_EMIT connectionFailed();
    return;
  }

  if (m_retryCount == 0) {
    qDebug() << "ipc client connecting to server:" << m_socketName;
  } else {
    qDebug() << "ipc client retrying connection, attempt:" << m_retryCount + 1;
  }

  m_state = State::Connecting;
  m_retryCount++;

  connect(
      m_socket, &QLocalSocket::connected, this,
      [this] {
        const auto versionId = QStringLiteral("%1+%2").arg(kVersion, kVersionGitSha);
        m_socket->write(QString("hello=%1\n").arg(versionId).toUtf8());
        qDebug() << "ipc client sent hello with version:" << versionId;
      },
      Qt::SingleShotConnection
  );

  connect(
      m_socket, &QLocalSocket::errorOccurred, this,
      [this] {
        qWarning() << "ipc client failed to connect:" << m_socket->errorString();
        m_socket->disconnectFromServer();
        m_state = State::Unconnected;
        QTimer::singleShot(0, this, &IpcClient::attemptConnection);
      },
      Qt::SingleShotConnection
  );

  m_socket->connectToServer(m_socketName);
}

void IpcClient::disconnectFromServer()
{
  m_state = State::Disconnecting;
  qDebug() << "ipc client disconnecting from server";
  m_socket->disconnectFromServer();
  m_state = State::Unconnected;
}

void IpcClient::handleDisconnected()
{
  if (m_state == State::Connecting) {
    return;
  }

  qDebug() << "ipc client disconnected from server";
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

  qWarning() << "ipc client error:" << m_socket->errorString();

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

    qDebug("ipc client message: %s", message.toUtf8().constData());
    const auto parts = message.split('=');
    if (parts.isEmpty()) {
      qWarning("ipc client got invalid message: %s", message.toUtf8().constData());
      continue;
    }

    if (m_state == State::Connecting) {
      handleHandshakeMessage(parts);
      continue;
    }

    processCommand(parts.at(0), parts);
  }

  if (!data.isEmpty()) {
    m_readBuffer = data;
  }
}

void IpcClient::handleHandshakeMessage(const QStringList &parts)
{
  if (parts.at(0) != "hello") {
    return;
  }

  const auto versionId = QStringLiteral("%1+%2").arg(kVersion, kVersionGitSha);
  const auto serverVersion = parts.size() >= 2 ? parts.at(1) : QString();
  if (serverVersion != versionId) {
    qCritical() << "ipc version mismatch (client:" << versionId << "server:" << serverVersion << ")";
    disconnectFromServer();
    Q_EMIT connectionFailed();
    return;
  }

  m_state = State::Connected;
  qDebug() << "ipc client connected";
  Q_EMIT connected();
}

void IpcClient::sendMessage(const QString &message)
{
  if (m_state != State::Connected) {
    qWarning() << "cannot send command, ipc client not connected";
    return;
  }

  m_socket->write(message.toUtf8() + "\n");
  qDebug() << "ipc client sent message:" << message;
}

} // namespace deskflow::gui::ipc
