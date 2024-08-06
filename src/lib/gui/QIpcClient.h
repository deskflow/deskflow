/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QAbstractSocket>
#include <QDataStream>
#include <QObject>
#include <QTcpSocket>
#include <memory>

#include "ElevateMode.h"
#include "QDataStreamProxy.h"

class IpcReader;

class QIpcClient : public QObject {
  Q_OBJECT

public:
  using StreamProvider = std::function<std::shared_ptr<QDataStreamProxy>()>;

  explicit QIpcClient(const StreamProvider &streamProvider = nullptr);

  void sendHello() const;
  void sendCommand(const QString &command, ElevateMode elevate) const;
  void connectToHost();
  void disconnectFromHost();
  bool isConnected() const { return m_isConnected; }

private slots:
  void onRetryConnect();
  void onSocketConnected() const;
  void onIpcReaderHelloBack();
  void onSocketError(QAbstractSocket::SocketError error) const;
  void onIpcReaderRead(const QString &text);

signals:
  void read(const QString &text);
  void serviceReady();

private:
  std::unique_ptr<QTcpSocket> m_pSocket;
  std::unique_ptr<IpcReader> m_pReader;
  bool m_readerStarted = false;
  StreamProvider m_streamProvider;
  bool m_isConnected = false;
  bool m_isConnecting = false;
};
