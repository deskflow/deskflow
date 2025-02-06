/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class QLocalSocket;

namespace deskflow::gui::ipc {

class DaemonIpcClient : public QObject
{
  Q_OBJECT

public:
  DaemonIpcClient(QObject *parent = nullptr);
  ~DaemonIpcClient();
  void connect();

private:
  QLocalSocket *socket;
};

} // namespace deskflow::gui::ipc
