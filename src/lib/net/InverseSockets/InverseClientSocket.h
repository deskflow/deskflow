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

#include "net/IDataSocket.h"
#include "io/StreamBuffer.h"
#include "mt/CondVar.h"
#include "mt/Mutex.h"
#include "arch/IArchNetwork.h"
#include "AutoArchSocket.h"

class ISocketMultiplexerJob;
class IEventQueue;
class SocketMultiplexer;

class InverseClientSocket : public IDataSocket {
public:
    InverseClientSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family = IArchNetwork::kINET);
    InverseClientSocket(InverseClientSocket const &) =delete;
    InverseClientSocket(InverseClientSocket &&) =delete;
    virtual ~InverseClientSocket();

    InverseClientSocket& operator=(InverseClientSocket const &) =delete;
    InverseClientSocket& operator=(InverseClientSocket &&) =delete;

    // ISocket overrides
    virtual void        bind(const NetworkAddress&);
    virtual void        close();
    virtual void*       getEventTarget() const;

    // IStream overrides
    virtual UInt32      read(void* buffer, UInt32 n);
    virtual void        write(const void* buffer, UInt32 n);
    virtual void        flush();
    virtual void        shutdownInput();
    virtual void        shutdownOutput();
    virtual bool        isReady() const;
    virtual bool        isFatal() const;
    virtual UInt32      getSize() const;

    // IDataSocket overrides
    virtual void        connect(const NetworkAddress&);


    virtual ISocketMultiplexerJob*
                        newJob();

protected:
    enum EJobResult {
        kBreak = -1,    //!< Break the Job chain
        kRetry,            //!< Retry the same job
        kNew            //!< Require a new job
    };

    ArchSocket          getSocket() { return m_socket.getRawSocket(); }
    IEventQueue*        getEvents() { return m_events; }
    virtual EJobResult  doRead();
    virtual EJobResult  doWrite();

    void                setJob(ISocketMultiplexerJob*);

    bool                isReadable() { return m_readable; }
    bool                isWritable() { return m_writable; }

    Mutex&              getMutex() { return m_mutex; }

    void                sendEvent(Event::Type);
    void                discardWrittenData(int bytesWrote);

private:
    void                sendConnectionFailedEvent(const char*);
    void                onConnected();
    void                onInputShutdown();
    void                onOutputShutdown();
    void                onDisconnected();

    ISocketMultiplexerJob*
                        serviceConnecting(ISocketMultiplexerJob*,
                            bool, bool, bool);
    ISocketMultiplexerJob*
                        serviceConnected(ISocketMultiplexerJob*,
                            bool, bool, bool);

protected:
    bool                m_readable = false;
    bool                m_writable = false;
    bool                m_connected = false;
    IEventQueue*        m_events;
    StreamBuffer        m_inputBuffer;
    StreamBuffer        m_outputBuffer;

private:
    Mutex                m_mutex;
    AutoArchSocket       m_socket;
    CondVar<bool>        m_flushed;
    SocketMultiplexer*   m_socketMultiplexer;
};
