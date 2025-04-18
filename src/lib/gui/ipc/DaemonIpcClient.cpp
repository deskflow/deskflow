/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcClient.h"

#include "common/Constants.h"

#include <QDebug>
#include <QLocalSocket>
#include <QObject>
#include <QString>

namespace deskflow::gui::ipc {

// At 1 second, we seem to be timing out on the first connection attempt, even though the server is
// listening and responding. Let's try 2 seconds and see if that makes a difference.
const auto kTimeout = 2000;

DaemonIpcClient::DaemonIpcClient(QObject *parent)
    : QObject(parent),
      m_socket{new QLocalSocket(this)} // NOSONAR - Qt memory
{
  connect(m_socket, &QLocalSocket::disconnected, this, &DaemonIpcClient::handleDisconnected);
  connect(m_socket, &QLocalSocket::errorOccurred, this, &DaemonIpcClient::handleErrorOccurred);
}

bool DaemonIpcClient::connectToServer()
{
  if (m_state == State::Connected) {
    qDebug() << "daemon ipc client already connected to server";
    return true;
  }

  if (m_state == State::Connecting) {
    qWarning() << "daemon ipc client already connecting to server";
    return true;
  }

  if (m_socket->state() == QLocalSocket::ConnectedState) {
    qWarning() << "daemon ipc client underlying socket is already connected, reconnecting";
    disconnectFromServer();
  }

  qDebug() << "daemon ipc client connecting to server:" << kDaemonIpcName;
  m_state = State::Connecting;
  m_socket->connectToServer(kDaemonIpcName);

  if (!m_socket->waitForConnected(kTimeout)) {
    qWarning() << "daemon ipc client failed to connect";
    disconnectFromServer();
    Q_EMIT connectFailed();
    return false;
  }

  if (!sendMessage("hello", "hello", false)) {
    qWarning() << "daemon ipc client failed to send hello";
    disconnectFromServer();
    Q_EMIT connectFailed();
    return false;
  }

  m_state = State::Connected;
  qDebug() << "daemon ipc client connected";
  Q_EMIT connected();

  return true;
}

void DaemonIpcClient::disconnectFromServer()
{
  if (m_socket->state() == QLocalSocket::ConnectedState) {
    qDebug() << "daemon ipc client disconnecting from server";
    m_socket->disconnectFromServer();
    m_socket->waitForDisconnected(kTimeout);
  } else {
    qDebug() << "daemon ipc client already disconnected from server";
  }

  m_state = State::Unconnected;
  qDebug() << "daemon ipc client disconnected from server";
}

void DaemonIpcClient::handleDisconnected()
{
  qWarning() << "daemon ipc client disconnected from server";
  if (m_state == State::Connected) {
    disconnectFromServer();
    Q_EMIT connectFailed();
  }
}

void DaemonIpcClient::handleErrorOccurred()
{
  qWarning() << "daemon ipc client error:" << m_socket->errorString();
  if (m_state == State::Connected) {
    disconnectFromServer();
    Q_EMIT connectFailed();
  }
}

bool DaemonIpcClient::sendMessage(const QString &message, const QString &expectAck, const bool expectConnected)
{
  if (expectConnected && !isConnected()) {
    qWarning() << "cannot send command, ipc client not connected";
    return false;
  }

  QByteArray messageData = message.toUtf8() + "\n";
  m_socket->write(messageData);
  if (!m_socket->waitForBytesWritten(kTimeout)) {
    qWarning() << "daemon ipc client failed to write command";
    return false;
  }

  if (!expectAck.isEmpty()) {
    qDebug() << "daemon ipc client waiting for ack: " << expectAck;

    if (!m_socket->waitForReadyRead(kTimeout)) {
      qWarning() << "daemon ipc client socket ready read timed out";
      return false;
    }

    QByteArray response = m_socket->readAll();
    if (response.isEmpty()) {
      qWarning() << "daemon ipc client got empty response";
      return false;
    }

    QString responseData = QString::fromUtf8(response);
    if (responseData.isEmpty()) {
      qWarning() << "daemon ipc client failed to convert response to string";
      return false;
    }

    if (responseData != expectAck + "x\n") {
      qWarning() << "daemon ipc client got unexpected response: " << responseData;
      return false;
    }
  }

  qDebug() << "daemon ipc client sent message: " << messageData;
  return true;
}

bool DaemonIpcClient::keepAlive()
{
  if (!isConnected() && !connectToServer()) {
    qWarning() << "daemon ipc client keep alive failed to connect";
    return false;
  }

  if (!sendMessage("noop")) {
    qWarning() << "daemon ipc client keep alive ping failed, reconnecting";
    connectToServer();
    return false;
  }

  return true;
}

bool DaemonIpcClient::sendLogLevel(const QString &logLevel)
{
  if (!keepAlive())
    return false;

  sendMessage("logLevel=" + logLevel);
  return true;
}

bool DaemonIpcClient::sendStartProcess(const QString &command, bool elevate)
{
  if (!keepAlive())
    return false;

  if (!sendMessage("elevate=" + (elevate ? QStringLiteral("yes") : QStringLiteral("no")))) {
    return false;
  }

  if (!sendMessage("command=" + command)) {
    return false;
  }

  return sendMessage("start");
}

bool DaemonIpcClient::sendStopProcess()
{
  return sendMessage("stop");
}

QString DaemonIpcClient::requestLogPath()
{
  if (!keepAlive())
    return QString();

  if (!sendMessage("logPath", QString())) {
    return QString();
  }

  if (!m_socket->waitForReadyRead(kTimeout)) {
    qWarning() << "daemon ipc client failed to read log path response";
    return QString();
  }

  QByteArray response = m_socket->readAll();
  if (response.isEmpty()) {
    qWarning() << "daemon ipc client got empty log path response";
    return QString();
  }

  QString responseData = QString::fromUtf8(response);
  if (responseData.isEmpty()) {
    qWarning() << "daemon ipc client failed to convert log path response to string";
    return QString();
  }

  // Trimming removes newline from end of message.
  QStringList parts = responseData.trimmed().split("=");
  if (parts.size() != 2) {
    qWarning() << "daemon ipc client got invalid log path response: " << responseData;
    return QString();
  }

  if (parts[0] != "logPath") {
    qWarning() << "daemon ipc client got unexpected log path response: " << responseData;
    return QString();
  }

  return parts[1];
}

bool DaemonIpcClient::sendClearSettings()
{
  return sendMessage("clearSettings");
}

} // namespace deskflow::gui::ipc
