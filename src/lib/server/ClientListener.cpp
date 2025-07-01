/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientListener.h"
#include "server/Server.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
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
    const NetworkAddress &address, std::unique_ptr<ISocketFactory> socketFactory, IEventQueue *events,
    SecurityLevel securityLevel
)
    : m_socketFactory{std::move(socketFactory)},
      m_events(events),
      m_securityLevel(securityLevel),
      m_address(address)
{
  assert(m_socketFactory != nullptr);

  try {
    start();
  } catch (XSocketAddressInUse &) {
    cleanupListenSocket();
    m_socketFactory.reset();
    throw;
  } catch (XBase &) {
    cleanupListenSocket();
    m_socketFactory.reset();
    throw;
  }
  LOG((CLOG_DEBUG1 "listening for clients"));
}

ClientListener::~ClientListener()
{
  stop();
  m_socketFactory.reset();
}

void ClientListener::setServer(Server *server)
{
  assert(server != nullptr);
  m_server = server;
}

ClientProxy *ClientListener::getNextClient()
{
  ClientProxy *client = nullptr;
  if (!m_waitingClients.empty()) {
    client = m_waitingClients.front();
    m_waitingClients.pop_front();
    m_events->removeHandler(EventTypes::ClientProxyDisconnected, client);
  }
  return client;
}

void ClientListener::start()
{
  m_listen = m_socketFactory->createListen(ARCH->getAddrFamily(m_address.getAddress()), m_securityLevel);

  // setup event handler
  m_events->addHandler(EventTypes::ListenSocketConnecting, m_listen, [this](const auto &) {
    handleClientConnecting();
  });

  // bind listen address
  LOG((CLOG_DEBUG1 "binding listen socket"));
  m_listen->bind(m_address);
}

void ClientListener::stop()
{
  using enum EventTypes;
  LOG((CLOG_DEBUG1 "stop listening for clients"));

  // discard already connected clients
  for (auto index = m_newClients.begin(); index != m_newClients.end(); ++index) {
    ClientProxyUnknown *client = *index;
    m_events->removeHandler(ClientProxyUnknownSuccess, client);
    m_events->removeHandler(ClientProxyUnknownFailure, client);
    m_events->removeHandler(ClientProxyDisconnected, client);
    delete client;
  }

  // discard waiting clients
  ClientProxy *client = getNextClient();
  while (client != nullptr) {
    delete client;
    client = getNextClient();
  }

  m_events->removeHandler(ListenSocketConnecting, m_listen);
  cleanupListenSocket();
  cleanupClientSockets();
}

void ClientListener::removeUnknownClient(ClientProxyUnknown *unknownClient)
{
  if (unknownClient) {
    m_events->removeHandler(EventTypes::ClientProxyUnknownSuccess, unknownClient);
    m_events->removeHandler(EventTypes::ClientProxyUnknownFailure, unknownClient);
    m_newClients.erase(unknownClient);
    delete unknownClient;
  }
}

void ClientListener::handleClientConnecting()
{
  // accept client connection
  auto socket = m_listen->accept();

  if (!socket)
    return;

  auto rawSocketPointer = socket.release();
  m_clientSockets.insert(rawSocketPointer);

  m_events->addHandler(
      EventTypes::ClientListenerAccepted, rawSocketPointer->getEventTarget(),
      [this, rawSocketPointer](const auto &) { handleClientAccepted(rawSocketPointer); }
  );

  // When using non SSL, server accepts clients immediately, while SSL
  // has to call secure accept which may require retry
  if (m_securityLevel == SecurityLevel::PlainText) {
    m_events->addEvent(Event(EventTypes::ClientListenerAccepted, rawSocketPointer->getEventTarget()));
  }
}

void ClientListener::handleClientAccepted(IDataSocket *socket)
{
  LOG((CLOG_NOTE "accepted client connection"));

  // filter socket messages, including a packetizing filter
  deskflow::IStream *stream = new PacketStreamFilter(m_events, socket, false);
  assert(m_server != nullptr);

  // create proxy for unknown client
  auto *client = new ClientProxyUnknown(stream, 30.0, m_server, m_events);

  m_newClients.insert(client);

  // watch for events from unknown client
  m_events->addHandler(EventTypes::ClientProxyUnknownSuccess, client, [this, client](const auto &) {
    handleUnknownClient(client);
  });
  m_events->addHandler(EventTypes::ClientProxyUnknownFailure, client, [this, client](const auto &) {
    removeUnknownClient(client);
  });
}

void ClientListener::handleUnknownClient(ClientProxyUnknown *unknownClient)
{
  // we should have the client in our new client list
  assert(m_newClients.count(unknownClient) == 1);

  // get the real client proxy and install it
  if (auto client = unknownClient->orphanClientProxy(); client) {
    // handshake was successful
    m_waitingClients.push_back(client);
    m_events->addEvent(Event(EventTypes::ClientListenerAccepted, this));

    // watch for client to disconnect while it's in our queue
    m_events->addHandler(EventTypes::ClientProxyDisconnected, client, [this, client](const auto &) {
      handleClientDisconnected(client);
    });
  } else {
    auto *stream = unknownClient->getStream();
    if (stream) {
      stream->close();
    }
  }

  // now finished with unknown client
  removeUnknownClient(unknownClient);
}

void ClientListener::handleClientDisconnected(ClientProxy *client)
{
  // find client in waiting clients queue
  for (WaitingClients::iterator i = m_waitingClients.begin(), n = m_waitingClients.end(); i != n; ++i) {
    if (*i == client) {
      m_waitingClients.erase(i);
      m_events->removeHandler(EventTypes::ClientProxyDisconnected, client);

      // pull out the socket before deleting the client so
      // we know which socket we no longer need
      auto *socket = static_cast<IDataSocket *>(client->getStream());
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
  for (auto client : m_clientSockets) {
    delete client;
  }
  m_clientSockets.clear();
}
