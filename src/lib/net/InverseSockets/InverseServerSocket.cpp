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

#include "InverseServerSocket.h"

#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocket.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "net/XSocket.h"
#include "io/XIO.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/Log.h"
#include "base/IEventQueue.h"

//
// InverseServerSocket
//

InverseServerSocket::InverseServerSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family) :
    m_socket(family),
    m_events(events),
    m_socketMultiplexer(socketMultiplexer)
{
}

InverseServerSocket::~InverseServerSocket()
{
    m_socketMultiplexer->removeSocket(this);
}

void
InverseServerSocket::bind(const NetworkAddress& addr)
{
    Lock lock(&m_mutex);
    m_address = addr;
    m_socket.connectSocket(m_address);
    setListeningJob(true);
}

void
InverseServerSocket::close()
{
    Lock lock(&m_mutex);
    m_socketMultiplexer->removeSocket(this);
    m_socket.closeSocket();
}

void*
InverseServerSocket::getEventTarget() const
{
    return const_cast<void*>(static_cast<const void*>(this));
}

IDataSocket*
InverseServerSocket::accept()
{
    IDataSocket* socket = nullptr;
    try {
        socket = new TCPSocket(m_events, m_socketMultiplexer, m_socket.getRawSocket());
        if (socket != nullptr) {
            setListeningJob();
        }
        return socket;
    }
    catch (const XArchNetwork&) {
        if (socket != nullptr) {
            delete socket;
            setListeningJob();
        }
        return nullptr;
    }
    catch (const std::exception&) {
        if (socket != nullptr) {
            delete socket;
            setListeningJob();
        }
        throw;
    }
}

void
InverseServerSocket::setListeningJob(bool read)
{
    m_socketMultiplexer->addSocket(this,
                            new TSocketMultiplexerMethodJob<InverseServerSocket>(
                                this, &InverseServerSocket::serviceListening,
                                m_socket.getRawSocket(), true, read));
}

ISocketMultiplexerJob*
InverseServerSocket::serviceListening(ISocketMultiplexerJob* job,
                            bool, bool write, bool error)
{
    if (error) {
        m_socket.connectSocket(m_address);
        return job;
    }
    if (write) {
        m_events->addEvent(Event(m_events->forIListenSocket().connecting(), this));
        // stop polling on this socket until the client accepts
        return nullptr;
    }
    return job;
}
