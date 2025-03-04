/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
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

IDataSocket *TCPSocketFactory::create(IArchNetwork::EAddressFamily family, SecurityLevel securityLevel) const
{
  if (securityLevel != SecurityLevel::PlainText) {
    SecureSocket *secureSocket = new SecureSocket(m_events, m_socketMultiplexer, family, securityLevel);
    secureSocket->initSsl(false);
    return secureSocket;
  } else {
    return new TCPSocket(m_events, m_socketMultiplexer, family);
  }
}

IListenSocket *TCPSocketFactory::createListen(IArchNetwork::EAddressFamily family, SecurityLevel securityLevel) const
{
  IListenSocket *socket = NULL;
  if (securityLevel != SecurityLevel::PlainText) {
    socket = new SecureListenSocket(m_events, m_socketMultiplexer, family, securityLevel);
  } else {
    socket = new TCPListenSocket(m_events, m_socketMultiplexer, family);
  }

  return socket;
}
