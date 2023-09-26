/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2022 Symless Ltd.
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

#include "net/IListenSocket.h"
#include "arch/IArchNetwork.h"
#include "mt/Mutex.h"
#include "AutoArchSocket.h"

class ISocketMultiplexerJob;
class IEventQueue;
class SocketMultiplexer;

class InverseServerSocket : public IListenSocket {
public:
    InverseServerSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family);
    InverseServerSocket(InverseServerSocket const &) =delete;
    InverseServerSocket(InverseServerSocket &&) =delete;
    ~InverseServerSocket() override;

    InverseServerSocket& operator=(InverseServerSocket const &) =delete;
    InverseServerSocket& operator=(InverseServerSocket &&) =delete;

    // ISocket overrides
    void        bind(const NetworkAddress&) override;
    void        close() override;
    void*        getEventTarget() const override;

    // IListenSocket overrides
    IDataSocket* accept() override;

protected:
    void                setListeningJob(bool read = false);

public:
    ISocketMultiplexerJob*
                        serviceListening(ISocketMultiplexerJob*,
                            bool, bool, bool);

protected:
    AutoArchSocket      m_socket;
    Mutex               m_mutex;
    IEventQueue*        m_events;
    SocketMultiplexer*  m_socketMultiplexer;

private:
    NetworkAddress      m_address;
};
