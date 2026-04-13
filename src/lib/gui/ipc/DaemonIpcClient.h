/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IpcClient.h"

#include <QObject>

namespace deskflow::gui::ipc {

class DaemonIpcClient : public IpcClient
{
  Q_OBJECT

public:
  explicit DaemonIpcClient(QObject *parent = nullptr);
  void sendLogLevel(const QString &logLevel);
  void sendConfigFile(const QString &configFile);
  void sendStartProcess();
  void sendStopProcess();
  void sendClearSettings();
  void requestLogPath();

Q_SIGNALS:
  void logPathReceived(const QString &logPath);

protected:
  void processCommand(const QString &command, const QStringList &parts) override;
};

} // namespace deskflow::gui::ipc
