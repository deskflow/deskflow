/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Deskflow Developers
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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
