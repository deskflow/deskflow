/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Deskflow Developers
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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
