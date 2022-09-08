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

TCPServerSocket::TCPServerSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, NetworkAddress address) :
    m_socket(ARCH->getAddrFamily(address.getAddress())),
    m_events(events),
    m_socketMultiplexer(socketMultiplexer),
    m_clientAddress(address)
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
    Lock lock(&m_mutex);
    m_socket.connectSocket(m_clientAddress);
    setListeningJob(true);
}

void
TCPServerSocket::close()
{
    Lock lock(&m_mutex);
    if (!m_socket.isValid()) {
        throw XIOClosed();
    }
    try {
        m_socketMultiplexer->removeSocket(this);
        m_socket.closeSocket();
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
    IDataSocket* socket = nullptr;
    try {
        socket = new TCPSocket(m_events, m_socketMultiplexer, m_socket.getRawSocket());
        if (socket != nullptr) {
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
TCPServerSocket::setListeningJob(bool read)
{
    m_socketMultiplexer->addSocket(this,
                            new TSocketMultiplexerMethodJob<TCPServerSocket>(
                                this, &TCPServerSocket::serviceListening,
                                m_socket.getRawSocket(), true, read));
}

ISocketMultiplexerJob*
TCPServerSocket::serviceListening(ISocketMultiplexerJob* job,
                            bool read, bool write, bool error)
{
    LOG((CLOG_DEBUG1 "Read: %d Write: %d Error: %d", read, write, error));
    if (error) {
        close();
        return NULL;
    }
    if (write) {
        m_events->addEvent(Event(m_events->forIListenSocket().connecting(), this));
        // stop polling on this socket until the client accepts
        return NULL;
    }

    return job;
}
