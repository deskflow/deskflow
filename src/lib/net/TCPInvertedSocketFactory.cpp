/*
 * synergy -- mouse and keyboard sharing utility
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

#include "net/TCPInvertedSocketFactory.h"
#include "net/TCPClientSocket.h"
#include "net/TCPServerSocket.h"
#include "net/SecureSocket.h"
#include "net/SecureListenSocket.h"

//
// TCPInvertedSocketFactory
//

TCPInvertedSocketFactory::TCPInvertedSocketFactory(IEventQueue* events, SocketMultiplexer* socketMultiplexer) :
    m_events(events),
    m_socketMultiplexer(socketMultiplexer)
{
    // do nothing
}

TCPInvertedSocketFactory::TCPInvertedSocketFactory(IEventQueue* events, SocketMultiplexer* socketMultiplexer, const NetworkAddress& address) :
    m_events(events),
    m_socketMultiplexer(socketMultiplexer),
    m_clientAddress(address)
{
    // do nothing
}

TCPInvertedSocketFactory::~TCPInvertedSocketFactory()
{
    // do nothing
}

IDataSocket*
TCPInvertedSocketFactory::create(bool secure, IArchNetwork::EAddressFamily family) const
{
    if (secure) {
        SecureSocket* secureSocket = new SecureSocket(m_events, m_socketMultiplexer, family);
        secureSocket->initSsl (false);
        return secureSocket;
    }
    else {
        return new TCPClientSocket(m_events, m_socketMultiplexer, family);
    }
}

IListenSocket*
TCPInvertedSocketFactory::createListen(bool secure, IArchNetwork::EAddressFamily family) const
{
    if (secure) {
        return new SecureListenSocket(m_events, m_socketMultiplexer, family);
    }
    else {
        return new TCPServerSocket(m_events, m_socketMultiplexer, m_clientAddress);
    }
}
