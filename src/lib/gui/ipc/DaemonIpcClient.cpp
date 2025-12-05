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

const auto kTimeout = 1000;

DaemonIpcClient::DaemonIpcClient(QObject *parent) : IpcClient(parent, kDaemonIpcName)
{
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

  if (!socket()->waitForReadyRead(kTimeout)) {
    qWarning() << "daemon ipc client failed to read log path response";
    return QString();
  }

  QByteArray response = socket()->readAll();
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
