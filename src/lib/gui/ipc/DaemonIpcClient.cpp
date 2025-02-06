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

DaemonIpcClient::DaemonIpcClient(QObject *parent) : QObject(parent), socket{new QLocalSocket(this)}
{
}

DaemonIpcClient::~DaemonIpcClient()
{
}

void DaemonIpcClient::connect()
{
  socket->connectToServer(kDaemonIpcName);
  if (!socket->waitForConnected(kTimeout)) {
    qCritical() << "ipc client failed to connect to server:" << kDaemonIpcName;
    return;
  }

  socket->write("hello");
  if (!socket->waitForBytesWritten(kTimeout)) {
    qCritical() << "ipc client failed to write message";
    return;
  }

  if (!socket->waitForReadyRead(kTimeout)) {
    qCritical() << "ipc client failed to read response";
    return;
  }

  QByteArray response = socket->readAll();
  if (response.isEmpty()) {
    qCritical() << "ipc client got empty response";
    return;
  }

  QString responseData = QString::fromUtf8(response);
  if (responseData.isEmpty()) {
    qCritical() << "ipc client failed to convert response to string";
    return;
  }

  if (responseData != "hello") {
    qCritical() << "ipc client got unexpected response: " << responseData;
    return;
  }

  qInfo() << "ipc client connected to server:" << kDaemonIpcName;
}

} // namespace deskflow::gui::ipc
