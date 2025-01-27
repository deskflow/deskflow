/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ipc/IpcClient.h"
#include "base/TMethodEventJob.h"
#include "common/ipc.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcServerProxy.h"

//
// IpcClient
//

IpcClient::IpcClient(IEventQueue *events, SocketMultiplexer *socketMultiplexer)
    : m_serverAddress(NetworkAddress(kIpcHost, kIpcPort)),
      m_socket(events, socketMultiplexer),
      m_server(nullptr),
      m_events(events)
{
  init();
}

IpcClient::IpcClient(IEventQueue *events, SocketMultiplexer *socketMultiplexer, int port)
    : m_serverAddress(NetworkAddress(kIpcHost, port)),
      m_socket(events, socketMultiplexer),
      m_server(nullptr),
      m_events(events)
{
  init();
}

void IpcClient::init()
{
  m_serverAddress.resolve();
}

IpcClient::~IpcClient()
{
}

void IpcClient::connect()
{
  m_events->adoptHandler(
      m_events->forIDataSocket().connected(), m_socket.getEventTarget(),
      new TMethodEventJob<IpcClient>(this, &IpcClient::handleConnected)
  );

  m_socket.connect(m_serverAddress);
  m_server = new IpcServerProxy(m_socket, m_events);

  m_events->adoptHandler(
      m_events->forIpcServerProxy().messageReceived(), m_server,
      new TMethodEventJob<IpcClient>(this, &IpcClient::handleMessageReceived)
  );
}

void IpcClient::disconnect()
{
  m_events->removeHandler(m_events->forIDataSocket().connected(), m_socket.getEventTarget());
  m_events->removeHandler(m_events->forIpcServerProxy().messageReceived(), m_server);

  m_server->disconnect();
  delete m_server;
  m_server = nullptr;
}

void IpcClient::send(const IpcMessage &message)
{
  assert(m_server != nullptr);
  m_server->send(message);
}

void IpcClient::handleConnected(const Event &, void *)
{
  m_events->addEvent(Event(m_events->forIpcClient().connected(), this, m_server, Event::kDontFreeData));

  IpcHelloMessage message(IpcClientType::Node);
  send(message);
}

void IpcClient::handleMessageReceived(const Event &e, void *)
{
  Event event(m_events->forIpcClient().messageReceived(), this);
  event.setDataObject(e.getDataObject());
  m_events->addEvent(event);
}
