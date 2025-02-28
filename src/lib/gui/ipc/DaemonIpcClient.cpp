/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "DaemonIpcClient.h"

#include "common/constants.h"

#include <QDebug>
#include <QLocalSocket>
#include <QObject>
#include <QString>

namespace deskflow::gui::ipc {

const auto kTimeout = 1000;

DaemonIpcClient::DaemonIpcClient(QObject *parent)
    : QObject(parent),
      m_socket{new QLocalSocket(this)} // NOSONAR - Qt memory
{
  connect(m_socket, &QLocalSocket::disconnected, this, &DaemonIpcClient::handleDisconnected);
  connect(m_socket, &QLocalSocket::errorOccurred, this, &DaemonIpcClient::handleErrorOccurred);
}

bool DaemonIpcClient::connectToServer()
{
  qInfo("connecting to daemon ipc");

  m_socket->connectToServer(kDaemonIpcName);
  if (!m_socket->waitForConnected(kTimeout)) {
    qWarning() << "ipc client failed to connect to server:" << kDaemonIpcName;
    return false;
  }

  if (!sendMessage("hello", "hello", false)) {
    qWarning() << "ipc client failed to send hello";
    return false;
  }

  m_connected = true;
  qInfo() << "ipc client connected to server:" << kDaemonIpcName;
  Q_EMIT connected();

  return true;
}

void DaemonIpcClient::handleDisconnected()
{
  qWarning() << "ipc client disconnected from server";
  m_connected = false;

  if (!connectToServer()) {
    qWarning() << "ipc client failed to reconnect to server";
  }
}

void DaemonIpcClient::handleErrorOccurred()
{
  qWarning() << "ipc client error:" << m_socket->errorString();
  m_connected = false;
}

bool DaemonIpcClient::sendMessage(const QString &message, const QString &expectAck, const bool expectConnected)
{
  if (expectConnected && !m_connected) {
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
      qWarning() << "ipc client failed to read response";
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

bool DaemonIpcClient::keepAlive()
{
  if (!isConnected() && !connectToServer()) {
    qWarning() << "ipc client keep alive failed to connect";
    return false;
  }

  if (!sendMessage("noop")) {
    qWarning() << "ipc client keep alive ping failed";
    m_connected = false;
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

bool DaemonIpcClient::sendStartProcess(const QString &command, ElevateMode elevateMode)
{
  if (!keepAlive())
    return false;

  if (!sendMessage("elevate=" + (elevateMode == ElevateMode::kAlways ? QStringLiteral("yes") : QStringLiteral("no")))) {
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
    qWarning() << "ipc client failed to read log path response";
    return QString();
  }

  QByteArray response = m_socket->readAll();
  if (response.isEmpty()) {
    qWarning() << "ipc client got empty log path response";
    return QString();
  }

  QString responseData = QString::fromUtf8(response);
  if (responseData.isEmpty()) {
    qWarning() << "ipc client failed to convert log path response to string";
    return QString();
  }

  // Trimming removes newline from end of message.
  QStringList parts = responseData.trimmed().split("=");
  if (parts.size() != 2) {
    qWarning() << "ipc client got invalid log path response: " << responseData;
    return QString();
  }

  if (parts[0] != "logPath") {
    qWarning() << "ipc client got unexpected log path response: " << responseData;
    return QString();
  }

  return parts[1];
}

} // namespace deskflow::gui::ipc
