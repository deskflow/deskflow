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

#include "net/TCPServerSocket.h"

#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocket.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "net/XSocket.h"
#include "io/XIO.h"
#include "mt/Lock.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include <iostream>

//
// TCPServerSocket
//

TCPServerSocket::TCPServerSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family) :
    m_listener(family),
    m_socket(family),
    m_events(events),
    m_socketMultiplexer(socketMultiplexer)
{
}

TCPServerSocket::~TCPServerSocket()
{
    try {
        m_socketMultiplexer->removeSocket(this);
    }
    catch (...) {
        // ignore
        LOG((CLOG_WARN "error while closing TCP socket"));
    }
}

void
TCPServerSocket::bind(const NetworkAddress& addr)
{
    try {
        Lock lock(&m_mutex);
        m_listener.setReuseAddrOnSocket();
        m_listener.bindSocket(addr);
        m_listener.listenOnSocket();
        m_socketMultiplexer->addSocket(this,
                            new TSocketMultiplexerMethodJob<TCPServerSocket>(
                                this, &TCPServerSocket::serviceListening,
                                m_listener.getRawSocket(), true, false));
    }
    catch (XArchNetworkAddressInUse& e) {
        throw XSocketAddressInUse(e.what());
    }
    catch (XArchNetwork& e) {
        throw XSocketBind(e.what());
    }
}

void
TCPServerSocket::close()
{
    Lock lock(&m_mutex);
    if (!m_listener.isValid()) {
        throw XIOClosed();
    }
    try {
        m_socketMultiplexer->removeSocket(this);
        m_listener.closeSocket();
    }
    catch (XArchNetwork& e) {
        throw XSocketIOClose(e.what());
    }
}

void*
TCPServerSocket::getEventTarget() const
{
    return const_cast<void*>(static_cast<const void*>(this));
}

IDataSocket*
TCPServerSocket::accept()
{
    connect();
    IDataSocket* socket = NULL;
    try {
        socket = new TCPSocket(m_events, m_socketMultiplexer, m_listener.acceptSocket());
        if (socket != NULL) {
            setListeningJob();
        }
        return socket;
    }
    catch (XArchNetwork&) {
        if (socket != NULL) {
            delete socket;
            setListeningJob();
        }
        return NULL;
    }
    catch (std::exception &ex) {
        if (socket != NULL) {
            delete socket;
            setListeningJob();
        }
        throw ex;
    }
}

void
TCPServerSocket::setListeningJob()
{
    m_socketMultiplexer->addSocket(this,
                            new TSocketMultiplexerMethodJob<TCPServerSocket>(
                                this, &TCPServerSocket::serviceListening,
                                m_listener.getRawSocket(), true, false));
}

ISocketMultiplexerJob*
TCPServerSocket::serviceListening(ISocketMultiplexerJob* job,
                            bool read, bool, bool error)
{
    if (error) {
        close();
        return NULL;
    }
    if (read) {
        m_events->addEvent(Event(m_events->forIListenSocket().connecting(), this));
        // stop polling on this socket until the client accepts
        return NULL;
    }
    return job;
}

void TCPServerSocket::connect()
{
    std::cout<<"SGADTRACE: "<<__FUNCTION__<<std::endl;
    NetworkAddress addr("192.168.1.191", 14800);
    addr.resolve();
    if (m_socket.connectSocket(addr)) {
        std::cout<<"SGADTRACE: Connected!"<<std::endl;
    } else {
        std::cout<<"SGADTRACE: Connection failed!"<<std::endl;
    }
}
