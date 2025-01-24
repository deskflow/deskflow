/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/TCPSocketFactory.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "net/SecureListenSocket.h"
#include "net/SecureSocket.h"
#include "net/TCPListenSocket.h"
#include "net/TCPSocket.h"

//
// TCPSocketFactory
//

TCPSocketFactory::TCPSocketFactory(IEventQueue *events, SocketMultiplexer *socketMultiplexer)
    : m_events(events),
      m_socketMultiplexer(socketMultiplexer)
{
  // do nothing
}

TCPSocketFactory::~TCPSocketFactory()
{
  // do nothing
}

IDataSocket *TCPSocketFactory::create(bool secure, IArchNetwork::EAddressFamily family) const
{
  if (secure) {
    SecureSocket *secureSocket = new SecureSocket(m_events, m_socketMultiplexer, family);
    secureSocket->initSsl(false);
    return secureSocket;
  } else {
    return new TCPSocket(m_events, m_socketMultiplexer, family);
  }
}

IListenSocket *TCPSocketFactory::createListen(bool secure, IArchNetwork::EAddressFamily family) const
{
  IListenSocket *socket = NULL;
  if (secure) {
    socket = new SecureListenSocket(m_events, m_socketMultiplexer, family);
  } else {
    socket = new TCPListenSocket(m_events, m_socketMultiplexer, family);
  }

  return socket;
}
