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
  DaemonIpcServer(QObject *parent);
  ~DaemonIpcServer();

signals:
  void elevateModeChanged(int mode);
  void commandChanged(const QString &command);
  void restartRequested();

private:
  void processMessage(QLocalSocket *clientSocket, const QString &message);

private slots:
  void handleNewConnection();
  void handleReadyRead();
  void handleDisconnected();
  void handleErrorOccurred();

private:
  QLocalServer *m_server;
  QSet<QLocalSocket *> m_clients;
};

} // namespace deskflow::core::ipc
