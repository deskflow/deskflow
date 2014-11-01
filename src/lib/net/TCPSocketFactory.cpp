/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

#include "net/TCPSocket.h"
#include "net/TCPListenSocket.h"

//
// CTCPSocketFactory
//

CTCPSocketFactory::CTCPSocketFactory(IEventQueue* events, CSocketMultiplexer* socketMultiplexer) :
	m_events(events),
	m_socketMultiplexer(socketMultiplexer)
{
	// do nothing
}

CTCPSocketFactory::~CTCPSocketFactory()
{
	// do nothing
}

IDataSocket*
CTCPSocketFactory::create(IArchNetwork::EAddressFamily family) const
{
	return new CTCPSocket(family, m_events, m_socketMultiplexer);
}

IListenSocket*
CTCPSocketFactory::createListen(IArchNetwork::EAddressFamily family) const
{
	return new CTCPListenSocket(family, m_events, m_socketMultiplexer);
}
