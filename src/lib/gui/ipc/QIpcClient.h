/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
