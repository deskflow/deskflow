/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "net/NetworkAddress.h"
#include "net/TCPSocket.h"

class IpcServerProxy;
class IpcMessage;
class IEventQueue;
class SocketMultiplexer;

//! IPC client for communication between daemon and GUI.
/*!
 * See \ref IpcServer description.
 */
class IpcClient
{
public:
  IpcClient(IEventQueue *events, SocketMultiplexer *socketMultiplexer);
  IpcClient(IEventQueue *events, SocketMultiplexer *socketMultiplexer, int port);
  virtual ~IpcClient();

  //! @name manipulators
  //@{

  //! Connects to the IPC server at localhost.
  void connect();

  //! Disconnects from the IPC server.
  void disconnect();

  //! Sends a message to the server.
  void send(const IpcMessage &message);

  //@}

private:
  void init();
  void handleConnected(const Event &, void *);
  void handleMessageReceived(const Event &, void *);

private:
  NetworkAddress m_serverAddress;
  TCPSocket m_socket;
  IpcServerProxy *m_server;
  IEventQueue *m_events;
};
