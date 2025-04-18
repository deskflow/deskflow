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

  // Represents underlying socket state and whether the server responded to the hello message.
  enum class State
  {
    Unconnected,
    Connecting,
    Connected,
    Disconnecting,
  };

public:
  explicit DaemonIpcClient(QObject *parent = nullptr);
  bool connectToServer();
  void disconnectFromServer();
  bool sendLogLevel(const QString &logLevel);
  bool sendStartProcess(const QString &command, bool elevate);
  bool sendStopProcess();
  bool sendClearSettings();
  QString requestLogPath();

  bool isConnected() const
  {
    return m_state == State::Connected;
  }

signals:
  void connected();
  void connectionFailed();

private slots:
  void handleDisconnected();
  void handleErrorOccurred();

private:
  bool keepAlive();
  bool sendMessage(const QString &message, const QString &expectAck = "ok", const bool expectConnected = true);

private:
  QLocalSocket *m_socket;
  State m_state{State::Unconnected};
};

} // namespace deskflow::gui::ipc
