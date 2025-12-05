/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class QLocalSocket;

namespace deskflow::gui::ipc {

class IpcClient : public QObject
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
  explicit IpcClient(QObject *parent, const QString &socketName);
  bool connectToServer();
  void disconnectFromServer();

  bool isConnected() const
  {
    return m_state == State::Connected;
  }

Q_SIGNALS:
  void connected();
  void connectionFailed();

private Q_SLOTS:
  void handleDisconnected();
  void handleErrorOccurred();

protected:
  bool keepAlive();
  bool sendMessage(const QString &message, const QString &expectAck = "ok", const bool expectConnected = true);

  QLocalSocket *socket() const
  {
    return m_socket;
  }

private:
  QLocalSocket *m_socket;
  State m_state{State::Unconnected};
  QString m_socketName;
};

} // namespace deskflow::gui::ipc
