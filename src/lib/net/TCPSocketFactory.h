/*
 * barrier -- mouse and keyboard sharing utility
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

#pragma once

#include "net/ISocketFactory.h"
#include "arch/IArchNetwork.h"

class IEventQueue;
class SocketMultiplexer;

//! Socket factory for TCP sockets
class TCPSocketFactory : public ISocketFactory {
public:
    TCPSocketFactory(IEventQueue* events, SocketMultiplexer* socketMultiplexer);
    virtual ~TCPSocketFactory();

    // ISocketFactory overrides
    virtual IDataSocket* create(
            IArchNetwork::EAddressFamily family,
            bool secure) const;
    virtual IListenSocket* createListen(
            IArchNetwork::EAddressFamily family,
            bool secure) const;

private:
    IEventQueue*        m_events;
    SocketMultiplexer*    m_socketMultiplexer;
};
