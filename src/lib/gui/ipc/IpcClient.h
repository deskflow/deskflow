/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Symless Ltd.
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
  explicit IpcClient(QObject *parent, const QString &socketName, const QString &typeName);
  void connectToServer();
  void disconnectFromServer();

  bool isConnected() const
  {
    return m_state == State::Connected;
  }

Q_SIGNALS:
  void connected();
  void connectionFailed();
  void serverShutdown();
  void versionMismatch();

private Q_SLOTS:
  void handleDisconnected();
  void handleErrorOccurred();
  void handleReadyRead();

protected:
  virtual void processCommand(const QString &command, const QStringList &parts)
  {
    Q_UNUSED(command)
    Q_UNUSED(parts)
  }

  void sendMessage(const QString &message);

private:
  void attemptConnection();
  void handleHandshakeMessage(const QStringList &parts);

  QLocalSocket *m_socket;
  State m_state{State::Unconnected};
  QString m_socketName;
  QByteArray m_readBuffer;
  int m_retryCount{0};
  QString m_typeName;
};

} // namespace deskflow::gui::ipc
