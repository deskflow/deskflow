/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
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
  bool sendLogLevel(const QString &logLevel);
  bool sendStartProcess(const QString &command, bool elevate);
  bool sendStopProcess();
  bool sendClearSettings();
  QString requestLogPath();
};

} // namespace deskflow::gui::ipc
