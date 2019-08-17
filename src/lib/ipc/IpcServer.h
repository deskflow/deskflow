/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "ipc/Ipc.h"
#include "net/TCPListenSocket.h"
#include "net/NetworkAddress.h"
#include "arch/Arch.h"
#include "base/EventTypes.h"

#include <list>
#include <mutex>

class Event;
class IpcClientProxy;
class IpcMessage;
class IEventQueue;
class SocketMultiplexer;

//! IPC server for communication between daemon and GUI.
/*!
The IPC server listens on localhost. The IPC client runs on both the
client/server process or the GUI. The IPC server runs on the daemon process.
This allows the GUI to send config changes to the daemon and client/server,
and allows the daemon and client/server to send log data to the GUI.
*/
class IpcServer {
public:
    IpcServer(IEventQueue* events, SocketMultiplexer* socketMultiplexer);
    IpcServer(IEventQueue* events, SocketMultiplexer* socketMultiplexer, int port);
    virtual ~IpcServer();

    //! @name manipulators
    //@{

    //! Opens a TCP socket only allowing local connections.
    virtual void        listen();

    //! Send a message to all clients matching the filter type.
    virtual void        send(const IpcMessage& message, EIpcClientType filterType);

    //@}
    //! @name accessors
    //@{

    //! Returns true when there are clients of the specified type connected.
    virtual bool        hasClients(EIpcClientType clientType) const;

    //@}

private:
    void                init();
    void                handleClientConnecting(const Event&, void*);
    void                handleClientDisconnected(const Event&, void*);
    void                handleMessageReceived(const Event&, void*);
    void                deleteClient(IpcClientProxy* proxy);

private:
    typedef std::list<IpcClientProxy*> ClientList;
    
    bool                m_mock;
    IEventQueue*        m_events;
    SocketMultiplexer*    m_socketMultiplexer;
    TCPListenSocket*    m_socket;
    NetworkAddress        m_address;
    ClientList            m_clients;
    mutable std::mutex m_clientsMutex;

#ifdef TEST_ENV
public:
    IpcServer() :
        m_mock(true),
        m_events(nullptr),
        m_socketMultiplexer(nullptr),
        m_socket(nullptr) { }
#endif
};
