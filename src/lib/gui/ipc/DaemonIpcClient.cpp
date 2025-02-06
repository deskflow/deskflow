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

namespace deskflow::gui::ipc {

const auto kTimeout = 1000;

DaemonIpcClient::DaemonIpcClient(QObject *parent) : QObject(parent), m_socket{new QLocalSocket(this)}
{
}

DaemonIpcClient::~DaemonIpcClient()
{
}

void DaemonIpcClient::connectToServer()
{
  m_socket->connectToServer(kDaemonIpcName);
  if (!m_socket->waitForConnected(kTimeout)) {
    qCritical() << "ipc client failed to connect to server:" << kDaemonIpcName;
    return;
  }

  sendMessage("hello", "hello", false);

  connect(m_socket, &QLocalSocket::disconnected, this, &DaemonIpcClient::handleDisconnected);
  connect(m_socket, &QLocalSocket::errorOccurred, this, &DaemonIpcClient::handleErrorOccurred);

  m_connected = true;
  qInfo() << "ipc client connected to server:" << kDaemonIpcName;
}

void DaemonIpcClient::handleDisconnected()
{
  qInfo() << "ipc client disconnected from server";
  m_connected = false;
}

void DaemonIpcClient::handleErrorOccurred()
{
  qCritical() << "ipc client error:" << m_socket->errorString();
  m_connected = false;
}

void DaemonIpcClient::sendCommand(const QString &command, ElevateMode elevateMode)
{
  sendMessage("elevate=" + QString::number(static_cast<int>(elevateMode)));
  sendMessage("command=" + command);
  sendMessage("restart");
}

void DaemonIpcClient::sendMessage(const QString &message, const QString &expectAck, const bool expectConnected)
{
  if (expectConnected && !m_connected) {
    qCritical() << "cannot send command, ipc not connected";
    return;
  }

  QByteArray messageData = message.toUtf8() + "\n";
  m_socket->write(messageData);
  if (!m_socket->waitForBytesWritten(kTimeout)) {
    qCritical() << "ipc client failed to write command";
    return;
  }

  if (!m_socket->waitForReadyRead(kTimeout)) {
    qCritical() << "ipc client failed to read response";
    return;
  }

  QByteArray response = m_socket->readAll();
  if (response.isEmpty()) {
    qCritical() << "ipc client got empty response";
    return;
  }

  QString responseData = QString::fromUtf8(response);
  if (responseData.isEmpty()) {
    qCritical() << "ipc client failed to convert response to string";
    return;
  }

  if (responseData != expectAck + "\n") {
    qCritical() << "ipc client got unexpected response: " << responseData;
    return;
  }

  qInfo() << "ipc client sent message: " << messageData;
}

} // namespace deskflow::gui::ipc
