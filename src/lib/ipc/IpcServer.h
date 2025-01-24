/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "base/EventTypes.h"
#include "common/ipc.h"
#include "net/NetworkAddress.h"
#include "net/TCPListenSocket.h"

#include <list>

class Event;
class IpcClientProxy;
class IpcMessage;
class IEventQueue;
class SocketMultiplexer;

//! IPC server for communication between daemon and GUI.
/*!
The IPC server listens on localhost. The IPC client runs on both the
client/server process or the GUI. The IPC server runs on the daemon process.
This allows the GUI to send config changes to the daemon and client/server,
and allows the daemon and client/server to send log data to the GUI.
*/
class IpcServer
{
public:
  IpcServer(IEventQueue *events, SocketMultiplexer *socketMultiplexer);
  IpcServer(IEventQueue *events, SocketMultiplexer *socketMultiplexer, int port);
  IpcServer(IpcServer const &) = delete;
  IpcServer(IpcServer &&) = delete;
  virtual ~IpcServer();

  IpcServer &operator=(IpcServer const &) = delete;
  IpcServer &operator=(IpcServer &&) = delete;

  //! @name manipulators
  //@{

  //! Opens a TCP socket only allowing local connections.
  virtual void listen();

  //! Send a message to all clients matching the filter type.
  virtual void send(const IpcMessage &message, IpcClientType filterType);

  //@}
  //! @name accessors
  //@{

  //! Returns true when there are clients of the specified type connected.
  virtual bool hasClients(IpcClientType clientType) const;

  //@}

private:
  void init();
  void handleClientConnecting(const Event &, void *);
  void handleClientDisconnected(const Event &, void *);
  void handleMessageReceived(const Event &, void *);
  void deleteClient(IpcClientProxy *proxy);

private:
  using ClientList = std::list<IpcClientProxy *>;

  bool m_mock;
  IEventQueue *m_events;
  SocketMultiplexer *m_socketMultiplexer;
  TCPListenSocket *m_socket;
  NetworkAddress m_address;
  ClientList m_clients;
  ArchMutex m_clientsMutex;

#ifdef TEST_ENV
public:
  IpcServer() : m_mock(true), m_events(nullptr), m_socketMultiplexer(nullptr), m_socket(nullptr)
  {
  }
#endif
};
