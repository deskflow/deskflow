/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>
#include <QSet>

class QLocalServer;
class QLocalSocket;

namespace deskflow::core::ipc {

class IpcServer : public QObject
{
  Q_OBJECT

public:
  explicit IpcServer(QObject *parent, const QString &serverName);
  ~IpcServer() override;

  void listen();

Q_SIGNALS:
  void logLevelChanged(const QString &logLevel);
  void elevateModeChanged(bool elevate);
  void commandChanged(const QString &command);
  void startProcessRequested();
  void stopProcessRequested();
  void clearSettingsRequested();

protected:
  /**!
   * Write a message to the client socket and append a newline character.
   *
   * \param clientSocket The client socket to write to.
   * \param message The message to write (without trailing newline).
   */
  void writeToClientSocket(QLocalSocket *&clientSocket, const QString &message) const;

private:
  virtual void processMessage(QLocalSocket *clientSocket, const QString &message) = 0;

private Q_SLOTS:
  void handleNewConnection();
  void handleReadyRead();
  void handleDisconnected();
  void handleErrorOccurred();

private:
  QLocalServer *m_server;
  QSet<QLocalSocket *> m_clients;
  QString m_serverName;
};

} // namespace deskflow::core::ipc
