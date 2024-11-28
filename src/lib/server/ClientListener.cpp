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

#include "server/ClientListener.h"
#include "server/Server.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "deskflow/PacketStreamFilter.h"
#include "net/IDataSocket.h"
#include "net/IListenSocket.h"
#include "net/ISocketFactory.h"
#include "net/XSocket.h"
#include "server/ClientProxy.h"
#include "server/ClientProxyUnknown.h"

//
// ClientListener
//

ClientListener::ClientListener(
    const NetworkAddress &address, ISocketFactory *socketFactory, IEventQueue *events, SecurityLevel securityLevel
)
    : m_socketFactory(socketFactory),
      m_server(NULL),
      m_events(events),
      m_securityLevel(securityLevel),
      m_address(address)
{
  assert(m_socketFactory != NULL);

  try {
    start();
  } catch (XSocketAddressInUse &) {
    cleanupListenSocket();
    delete m_socketFactory;
    throw;
  } catch (XBase &) {
    cleanupListenSocket();
    delete m_socketFactory;
    throw;
  }
  LOG((CLOG_DEBUG1 "listening for clients"));
}

ClientListener::~ClientListener()
{
  stop();
  delete m_socketFactory;
}

void ClientListener::setServer(Server *server)
{
  assert(server != NULL);
  m_server = server;
}

ClientProxy *ClientListener::getNextClient()
{
  ClientProxy *client = NULL;
  if (!m_waitingClients.empty()) {
    client = m_waitingClients.front();
    m_waitingClients.pop_front();
    m_events->removeHandler(m_events->forClientProxy().disconnected(), client);
  }
  return client;
}

void ClientListener::start()
{
  m_listen = m_socketFactory->createListen(ARCH->getAddrFamily(m_address.getAddress()), m_securityLevel);

  // setup event handler
  m_events->adoptHandler(
      m_events->forIListenSocket().connecting(), m_listen,
      new TMethodEventJob<ClientListener>(this, &ClientListener::handleClientConnecting)
  );

  // bind listen address
  LOG((CLOG_DEBUG1 "binding listen socket"));
  m_listen->bind(m_address);
}

void ClientListener::stop()
{
  LOG((CLOG_DEBUG1 "stop listening for clients"));

  // discard already connected clients
  for (NewClients::iterator index = m_newClients.begin(); index != m_newClients.end(); ++index) {
    ClientProxyUnknown *client = *index;
    m_events->removeHandler(m_events->forClientProxyUnknown().success(), client);
    m_events->removeHandler(m_events->forClientProxyUnknown().failure(), client);
    m_events->removeHandler(m_events->forClientProxy().disconnected(), client);
    delete client;
  }

  // discard waiting clients
  ClientProxy *client = getNextClient();
  while (client != nullptr) {
    delete client;
    client = getNextClient();
  }

  m_events->removeHandler(m_events->forIListenSocket().connecting(), m_listen);
  cleanupListenSocket();
  cleanupClientSockets();
}

void ClientListener::removeUnknownClient(ClientProxyUnknown *unknownClient)
{
  if (unknownClient) {
    m_events->removeHandler(m_events->forClientProxyUnknown().success(), unknownClient);
    m_events->removeHandler(m_events->forClientProxyUnknown().failure(), unknownClient);
    m_newClients.erase(unknownClient);
    delete unknownClient;
  }
}

void ClientListener::restart()
{
  if (m_server && m_server->isClientMode()) {
    stop();
    start();
  }
}

void ClientListener::handleClientConnecting(const Event &, void *)
{
  // accept client connection
  IDataSocket *socket = m_listen->accept();

  if (socket == NULL) {
    return;
  }

  m_clientSockets.insert(socket);

  m_events->adoptHandler(
      m_events->forClientListener().accepted(), socket->getEventTarget(),
      new TMethodEventJob<ClientListener>(this, &ClientListener::handleClientAccepted, socket)
  );

  // When using non SSL, server accepts clients immediately, while SSL
  // has to call secure accept which may require retry
  if (m_securityLevel == SecurityLevel::PlainText) {
    m_events->addEvent(Event(m_events->forClientListener().accepted(), socket->getEventTarget()));
  }
}

void ClientListener::handleClientAccepted(const Event &, void *vsocket)
{
  LOG((CLOG_NOTE "accepted client connection"));

  IDataSocket *socket = static_cast<IDataSocket *>(vsocket);

  // filter socket messages, including a packetizing filter
  deskflow::IStream *stream = new PacketStreamFilter(m_events, socket, false);
  assert(m_server != NULL);

  // create proxy for unknown client
  ClientProxyUnknown *client = new ClientProxyUnknown(stream, 30.0, m_server, m_events);

  m_newClients.insert(client);

  // watch for events from unknown client
  m_events->adoptHandler(
      m_events->forClientProxyUnknown().success(), client,
      new TMethodEventJob<ClientListener>(this, &ClientListener::handleUnknownClient, client)
  );
  m_events->adoptHandler(
      m_events->forClientProxyUnknown().failure(), client,
      new TMethodEventJob<ClientListener>(this, &ClientListener::handleUnknownClientFailure, client)
  );
}

void ClientListener::handleUnknownClient(const Event &, void *vclient)
{
  auto unknownClient = static_cast<ClientProxyUnknown *>(vclient);

  // we should have the client in our new client list
  assert(m_newClients.count(unknownClient) == 1);

  // get the real client proxy and install it
  auto client = unknownClient->orphanClientProxy();
  if (client) {
    // handshake was successful
    m_waitingClients.push_back(client);
    m_events->addEvent(Event(m_events->forClientListener().connected(), this));

    // watch for client to disconnect while it's in our queue
    m_events->adoptHandler(
        m_events->forClientProxy().disconnected(), client,
        new TMethodEventJob<ClientListener>(this, &ClientListener::handleClientDisconnected, client)
    );
  } else {
    auto *stream = unknownClient->getStream();
    if (stream) {
      stream->close();
    }
  }

  // now finished with unknown client
  removeUnknownClient(unknownClient);
}

void ClientListener::handleUnknownClientFailure(const Event &, void *vclient)
{
  auto unknownClient = static_cast<ClientProxyUnknown *>(vclient);
  removeUnknownClient(unknownClient);
  restart();
}

void ClientListener::handleClientDisconnected(const Event &, void *vclient)
{
  ClientProxy *client = static_cast<ClientProxy *>(vclient);

  // find client in waiting clients queue
  for (WaitingClients::iterator i = m_waitingClients.begin(), n = m_waitingClients.end(); i != n; ++i) {
    if (*i == client) {
      m_waitingClients.erase(i);
      m_events->removeHandler(m_events->forClientProxy().disconnected(), client);

      // pull out the socket before deleting the client so
      // we know which socket we no longer need
      IDataSocket *socket = static_cast<IDataSocket *>(client->getStream());
      delete client;
      m_clientSockets.erase(socket);
      delete socket;

      break;
    }
  }
}

void ClientListener::cleanupListenSocket()
{
  delete m_listen;
}

void ClientListener::cleanupClientSockets()
{
  ClientSockets::iterator it;
  for (it = m_clientSockets.begin(); it != m_clientSockets.end(); it++) {
    delete *it;
  }
  m_clientSockets.clear();
}
