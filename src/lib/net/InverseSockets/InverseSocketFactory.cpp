/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "InverseSocketFactory.h"
#include "net/InverseSockets/InverseClientSocket.h"
#include "net/InverseSockets/InverseServerSocket.h"
#include "net/InverseSockets/SecureClientSocket.h"
#include "net/InverseSockets/SecureServerSocket.h"

//
// InverseSocketFactory
//

InverseSocketFactory::InverseSocketFactory(IEventQueue *events, SocketMultiplexer *socketMultiplexer)
    : m_events(events),
      m_socketMultiplexer(socketMultiplexer)
{
}

IDataSocket *InverseSocketFactory::create(IArchNetwork::EAddressFamily family, SecurityLevel securityLevel) const
{
  if (securityLevel != SecurityLevel::PlainText) {
    auto secureSocket = new SecureClientSocket(m_events, m_socketMultiplexer, family);
    return secureSocket;
  } else {
    return new InverseClientSocket(m_events, m_socketMultiplexer, family);
  }
}

IListenSocket *
InverseSocketFactory::createListen(IArchNetwork::EAddressFamily family, SecurityLevel securityLevel) const
{
  IListenSocket *socket = nullptr;

  if (securityLevel != SecurityLevel::PlainText) {
    socket = new SecureServerSocket(m_events, m_socketMultiplexer, family);
  } else {
    socket = new InverseServerSocket(m_events, m_socketMultiplexer, family);
  }

  return socket;
}
