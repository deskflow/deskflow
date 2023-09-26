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
    ~InverseClientSocket() override;

    InverseClientSocket& operator=(InverseClientSocket const &) =delete;
    InverseClientSocket& operator=(InverseClientSocket &&) =delete;

    // ISocket overrides
    void        bind(const NetworkAddress&) override;
    void        close() override;
    void*       getEventTarget() const override;

    // IStream overrides
    UInt32      read(void* buffer, UInt32 n) override;
    void        write(const void* buffer, UInt32 n) override;
    void        flush() override;
    void        shutdownInput() override;
    void        shutdownOutput() override;
    bool        isReady() const override;
    bool        isFatal() const override;
    UInt32      getSize() const override;

    // IDataSocket overrides
    void        connect(const NetworkAddress&) override;


    virtual ISocketMultiplexerJob*
                        newJob(ArchSocket socket);

protected:
    enum class EJobResult {
        kBreak = -1,    //!< Break the Job chain
        kRetry,            //!< Retry the same job
        kNew            //!< Require a new job
    };

    ArchSocket          getSocket() { return m_socket.getRawSocket(); }
    IEventQueue*        getEvents() { return m_events; }
    virtual EJobResult  doRead();
    virtual EJobResult  doWrite();

    void                setJob(ISocketMultiplexerJob*);

    bool                isReadable() const { return m_readable; }
    bool                isWritable() const { return m_writable; }

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
    Mutex                m_mutex;
    AutoArchSocket       m_socket;
    AutoArchSocket       m_listener;
    CondVar<bool>        m_flushed;
    SocketMultiplexer*   m_socketMultiplexer;
};
