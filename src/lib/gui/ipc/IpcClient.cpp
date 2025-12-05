/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcClient.h"

#include <QDebug>
#include <QLocalSocket>
#include <QObject>
#include <QString>

namespace deskflow::gui::ipc {

const auto kTimeout = 1000;
const auto kRetryLimit = 3;

IpcClient::IpcClient(QObject *parent, const QString &socketName)
    : QObject(parent),
      m_socket{new QLocalSocket(this)},
      m_socketName(socketName) // NOSONAR - Qt memory
{
  connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::handleDisconnected);
  connect(m_socket, &QLocalSocket::errorOccurred, this, &IpcClient::handleErrorOccurred);
}

bool IpcClient::connectToServer()
{
  if (m_state == State::Connecting) {
    qWarning() << "ipc client already connecting to server";
    return false;
  }

  if (m_state != State::Unconnected) {
    qDebug() << "ipc client not in unconnected state, disconnecting";
    disconnectFromServer();
  }

  if (m_socket->state() != QLocalSocket::UnconnectedState) {
    qWarning() << "ipc client socket not in unconnected state, disconnecting";
    disconnectFromServer();
  }

  for (int i = 0; i < kRetryLimit; ++i) {
    if (i == 0) {
      qDebug() << "ipc client connecting to server:" << m_socketName;
    } else {
      qDebug() << "ipc client retrying connection, attempt:" << i + 1;
    }

    m_state = State::Connecting;
    m_socket->connectToServer(m_socketName);

    if (!m_socket->waitForConnected(kTimeout)) {
      qWarning() << "ipc client failed to connect";
      disconnectFromServer();
      continue;
    }

    if (!sendMessage("hello", "hello", false)) {
      qWarning() << "ipc client failed to send hello";
      disconnectFromServer();
      continue;
    }

    m_state = State::Connected;
    qDebug() << "ipc client connected";
    Q_EMIT connected();
    return true;
  }

  qWarning() << "ipc client failed to connect after" << kRetryLimit << "attempts";
  disconnectFromServer();
  Q_EMIT connectionFailed();
  return false;
}

void IpcClient::disconnectFromServer()
{
  m_state = State::Disconnecting;
  qDebug() << "ipc client disconnecting from server";
  m_socket->disconnectFromServer();

  if (m_socket->state() != QLocalSocket::UnconnectedState) {
    qDebug() << "ipc client waiting for socket to disconnect";
    m_socket->waitForDisconnected(kTimeout);
    qDebug() << "ipc client disconnected from server";
  } else {
    qDebug() << "ipc client socket already disconnected";
  }

  m_state = State::Unconnected;
}

void IpcClient::handleDisconnected()
{
  qDebug() << "ipc client disconnected from server";
  if (m_state == State::Connected) {
    Q_EMIT connectionFailed();
  }

  m_state = State::Unconnected;
}

void IpcClient::handleErrorOccurred()
{
  qWarning() << "ipc client error:" << m_socket->errorString();
  disconnectFromServer();

  if (m_state == State::Connected) {
    Q_EMIT connectionFailed();
  }
}

bool IpcClient::sendMessage(const QString &message, const QString &expectAck, const bool expectConnected)
{
  if (expectConnected && !isConnected()) {
    qWarning() << "cannot send command, ipc client not connected";
    return false;
  }

  QByteArray messageData = message.toUtf8() + "\n";
  m_socket->write(messageData);
  if (!m_socket->waitForBytesWritten(kTimeout)) {
    qWarning() << "ipc client failed to write command";
    return false;
  }

  if (!expectAck.isEmpty()) {
    qDebug() << "ipc client waiting for ack: " << expectAck;

    if (!m_socket->waitForReadyRead(kTimeout)) {
      qWarning() << "ipc client socket ready read timed out";
      return false;
    }

    QByteArray response = m_socket->readAll();
    if (response.isEmpty()) {
      qWarning() << "ipc client got empty response";
      return false;
    }

    QString responseData = QString::fromUtf8(response);
    if (responseData.isEmpty()) {
      qWarning() << "ipc client failed to convert response to string";
      return false;
    }

    if (responseData != expectAck + "\n") {
      qWarning() << "ipc client got unexpected response: " << responseData;
      return false;
    }
  }

  qDebug() << "ipc client sent message: " << messageData;
  return true;
}

bool IpcClient::keepAlive()
{
  if (!isConnected() && !connectToServer()) {
    qWarning() << "ipc client keep alive failed to connect";
    return false;
  }

  if (!sendMessage("noop")) {
    qWarning() << "ipc client keep alive ping failed, reconnecting";
    connectToServer();
    return false;
  }

  return true;
}

} // namespace deskflow::gui::ipc
