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

class DaemonIpcServer : public QObject
{
  Q_OBJECT

public:
  explicit DaemonIpcServer(QObject *parent, const QString &logFilename);
  ~DaemonIpcServer() override;

  void listen();

Q_SIGNALS:
  void logLevelChanged(const QString &logLevel);
  void elevateModeChanged(bool elevate);
  void commandChanged(const QString &command);
  void startProcessRequested();
  void stopProcessRequested();
  void clearSettingsRequested();

private:
  void processMessage(QLocalSocket *clientSocket, const QString &message);
  void processLogLevel(QLocalSocket *&clientSocket, const QStringList &messageParts);
  void processElevate(QLocalSocket *&clientSocket, const QStringList &messageParts);
  void processCommand(QLocalSocket *&clientSocket, const QStringList &messageParts);

  /**!
   * Write a message to the client socket and append a newline character.
   *
   * \param clientSocket The client socket to write to.
   * \param message The message to write (without trailing newline).
   */
  void writeToClientSocket(QLocalSocket *&clientSocket, const QString &message) const;

private Q_SLOTS:
  void handleNewConnection();
  void handleReadyRead();
  void handleDisconnected();
  void handleErrorOccurred();

private:
  const QString m_logFilename;
  QLocalServer *m_server;
  QSet<QLocalSocket *> m_clients;
};

} // namespace deskflow::core::ipc
