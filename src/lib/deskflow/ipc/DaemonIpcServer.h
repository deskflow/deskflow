/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IpcServer.h"

#include <QObject>
#include <QString>

class QLocalSocket;

namespace deskflow::core::ipc {

class DaemonIpcServer : public IpcServer
{
  Q_OBJECT

public:
  explicit DaemonIpcServer(QObject *parent, const QString &logFilename);

Q_SIGNALS:
  void logLevelChanged(const QString &logLevel);
  void elevateModeChanged(bool elevate);
  void commandChanged(const QString &command);
  void startProcessRequested();
  void stopProcessRequested();
  void clearSettingsRequested();

private:
  void processMessage(QLocalSocket *clientSocket, const QString &message) override;
  void processLogLevel(QLocalSocket *&clientSocket, const QStringList &messageParts);
  void processElevate(QLocalSocket *&clientSocket, const QStringList &messageParts);
  void processCommand(QLocalSocket *&clientSocket, const QStringList &messageParts);

private:
  const QString m_logFilename;
};

} // namespace deskflow::core::ipc
