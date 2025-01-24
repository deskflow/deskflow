/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ipc/IpcServer.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "common/ipc.h"
#include "io/IStream.h"
#include "ipc/IpcClientProxy.h"
#include "ipc/IpcMessage.h"
#include "net/IDataSocket.h"

//
// IpcServer
//

IpcServer::IpcServer(IEventQueue *events, SocketMultiplexer *socketMultiplexer)
    : m_mock(false),
      m_events(events),
      m_socketMultiplexer(socketMultiplexer),
      m_socket(nullptr),
      m_address(NetworkAddress(kIpcHost, kIpcPort))
{
  init();
}

IpcServer::IpcServer(IEventQueue *events, SocketMultiplexer *socketMultiplexer, int port)
    : m_mock(false),
      m_events(events),
      m_socketMultiplexer(socketMultiplexer),
      m_address(NetworkAddress(kIpcHost, port))
{
  init();
}

void IpcServer::init()
{
  m_socket = new TCPListenSocket(m_events, m_socketMultiplexer, IArchNetwork::EAddressFamily::kINET);

  m_clientsMutex = ARCH->newMutex();
  m_address.resolve();

  m_events->adoptHandler(
      m_events->forIListenSocket().connecting(), m_socket,
      new TMethodEventJob<IpcServer>(this, &IpcServer::handleClientConnecting)
  );
}

IpcServer::~IpcServer()
{
  if (m_mock) {
    return;
  }

  if (m_socket != nullptr) {
    delete m_socket;
  }

  ARCH->lockMutex(m_clientsMutex);
  ClientList::iterator it;
  for (it = m_clients.begin(); it != m_clients.end(); it++) {
    deleteClient(*it);
  }
  m_clients.clear();
  ARCH->unlockMutex(m_clientsMutex);
  ARCH->closeMutex(m_clientsMutex);

  m_events->removeHandler(m_events->forIListenSocket().connecting(), m_socket);
}

void IpcServer::listen()
{
  m_socket->bind(m_address);
}

void IpcServer::handleClientConnecting(const Event &, void *)
{
  deskflow::IStream *stream = m_socket->accept();
  if (stream == NULL) {
    return;
  }

  LOG((CLOG_DEBUG "accepted ipc client connection"));

  ARCH->lockMutex(m_clientsMutex);
  IpcClientProxy *proxy = new IpcClientProxy(*stream, m_events);
  m_clients.push_back(proxy);
  ARCH->unlockMutex(m_clientsMutex);

  m_events->adoptHandler(
      m_events->forIpcClientProxy().disconnected(), proxy,
      new TMethodEventJob<IpcServer>(this, &IpcServer::handleClientDisconnected)
  );

  m_events->adoptHandler(
      m_events->forIpcClientProxy().messageReceived(), proxy,
      new TMethodEventJob<IpcServer>(this, &IpcServer::handleMessageReceived)
  );

  m_events->addEvent(Event(m_events->forIpcServer().clientConnected(), this, proxy, Event::kDontFreeData));
}

void IpcServer::handleClientDisconnected(const Event &e, void *)
{
  IpcClientProxy *proxy = static_cast<IpcClientProxy *>(e.getTarget());

  ArchMutexLock lock(m_clientsMutex);
  m_clients.remove(proxy);
  deleteClient(proxy);

  LOG((CLOG_DEBUG "ipc client proxy removed, connected=%d", m_clients.size()));
}

void IpcServer::handleMessageReceived(const Event &e, void *)
{
  Event event(m_events->forIpcServer().messageReceived(), this);
  event.setDataObject(e.getDataObject());
  m_events->addEvent(event);
}

void IpcServer::deleteClient(IpcClientProxy *proxy)
{
  m_events->removeHandler(m_events->forIpcClientProxy().messageReceived(), proxy);
  m_events->removeHandler(m_events->forIpcClientProxy().disconnected(), proxy);
  delete proxy;
}

bool IpcServer::hasClients(IpcClientType clientType) const
{
  ArchMutexLock lock(m_clientsMutex);

  if (m_clients.empty()) {
    return false;
  }

  ClientList::const_iterator it;
  for (it = m_clients.begin(); it != m_clients.end(); it++) {
    // at least one client is alive and type matches, there are clients.
    IpcClientProxy *p = *it;
    if (!p->m_disconnecting && p->m_clientType == clientType) {
      return true;
    }
  }

  // all clients must be disconnecting, no active clients.
  return false;
}

void IpcServer::send(const IpcMessage &message, IpcClientType filterType)
{
  ArchMutexLock lock(m_clientsMutex);

  ClientList::iterator it;
  for (it = m_clients.begin(); it != m_clients.end(); it++) {
    IpcClientProxy *proxy = *it;
    if (proxy->m_clientType == filterType) {
      proxy->send(message);
    }
  }
}
