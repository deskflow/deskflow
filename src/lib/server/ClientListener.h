/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"
#include "common/stddeque.h"
#include "common/stdset.h"
#include "net/SecurityLevel.h"
#include "server/Config.h"

class ClientProxy;
class ClientProxyUnknown;
class NetworkAddress;
class IListenSocket;
class ISocketFactory;
class Server;
class IEventQueue;
class IDataSocket;

class ClientListener
{
public:
  // The factories are adopted.
  ClientListener(const NetworkAddress &, ISocketFactory *, IEventQueue *events, SecurityLevel securityLevel);
  ClientListener(ClientListener const &) = delete;
  ClientListener(ClientListener &&) = delete;
  ~ClientListener();

  ClientListener &operator=(ClientListener const &) = delete;
  ClientListener &operator=(ClientListener &&) = delete;

  //! @name manipulators
  //@{

  void setServer(Server *server);

  //@}

  //! @name accessors
  //@{

  //! Get next connected client
  /*!
  Returns the next connected client and removes it from the internal
  list.  The client is responsible for deleting the returned client.
  Returns NULL if no clients are available.
  */
  ClientProxy *getNextClient();

  //! Get server which owns this listener
  Server *getServer()
  {
    return m_server;
  }

  //! This method restarts the listener
  void restart();

  //@}

private:
  // client connection event handlers
  void handleClientConnecting(const Event &, void *);
  void handleClientAccepted(const Event &, void *);
  void handleUnknownClient(const Event &, void *);
  void handleUnknownClientFailure(const Event &, void *);
  void handleClientDisconnected(const Event &, void *);

  void cleanupListenSocket();
  void cleanupClientSockets();
  void start();
  void stop();
  void removeUnknownClient(ClientProxyUnknown *unknownClient);

private:
  using NewClients = std::set<ClientProxyUnknown *>;
  using WaitingClients = std::deque<ClientProxy *>;
  using ClientSockets = std::set<IDataSocket *>;

  IListenSocket *m_listen;
  ISocketFactory *m_socketFactory;
  NewClients m_newClients;
  WaitingClients m_waitingClients;
  Server *m_server;
  IEventQueue *m_events;
  SecurityLevel m_securityLevel;
  ClientSockets m_clientSockets;
  NetworkAddress m_address;
};
