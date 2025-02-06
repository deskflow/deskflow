/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

#include "gui/config/ElevateMode.h"

class QLocalSocket;

namespace deskflow::gui::ipc {

class DaemonIpcClient : public QObject
{
  Q_OBJECT

public:
  DaemonIpcClient(QObject *parent = nullptr);
  ~DaemonIpcClient();
  void connectToServer();
  void sendCommand(const QString &command, ElevateMode elevateMode);

  bool isConnected() const
  {
    return m_connected;
  }

private slots:
  void handleDisconnected();
  void handleErrorOccurred();

private:
  void sendMessage(const QString &message, const QString &expectAck = "ok", const bool expectConnected = true);

private:
  QLocalSocket *m_socket;
  bool m_connected{false};
};

} // namespace deskflow::gui::ipc
