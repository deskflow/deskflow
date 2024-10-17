/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "IpcReader.h"
#include "QDataStreamProxy.h"
#include "gui/config/ElevateMode.h"
#include "gui/ipc/IQIpcClient.h"

class IpcReader;

class QIpcClient : public deskflow::gui::ipc::IQIpcClient
{
  Q_OBJECT

public:
  using StreamProvider = std::function<std::shared_ptr<QDataStreamProxy>()>;

  explicit QIpcClient(const StreamProvider &streamProvider = nullptr);

  void sendHello() const override;
  void sendCommand(const QString &command, ElevateMode elevate) const override;
  void connectToHost() override;
  void disconnectFromHost() override;
  bool isConnected() const override
  {
    return m_isConnected;
  }

private slots:
  void onRetryConnect();
  void onSocketConnected() const;
  void onIpcReaderHelloBack();
  void onSocketError(QAbstractSocket::SocketError error);
  void onIpcReaderRead(const QString &text);

private:
  std::unique_ptr<QTcpSocket> m_pSocket;
  std::unique_ptr<IpcReader> m_pReader;
  bool m_readerStarted = false;
  StreamProvider m_streamProvider;
  bool m_isConnected = false;
  bool m_isConnecting = false;
};
