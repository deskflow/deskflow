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

#include <ws2tcpip.h>
// declare no functions in winsock2
#ifndef INCL_WINSOCK_API_PROTOTYPES
#define INCL_WINSOCK_API_PROTOTYPES 0
#endif
#define INCL_WINSOCK_API_TYPEDEFS 0

#include "arch/IArchNetwork.h"
#include "arch/IArchMultithread.h"

#include <WinSock2.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <list>

#pragma comment(lib, "ws2_32.lib")

#define ARCH_NETWORK ArchNetworkWinsock

class ArchSocketImpl {
public:
    SOCKET                m_socket;
    int                    m_refCount;
    WSAEVENT            m_event;
    bool                m_pollWrite;
};

class ArchNetAddressImpl {
public:
    static ArchNetAddressImpl* alloc(size_t);

public:
    int                            m_len;
    struct sockaddr_storage        m_addr;
};
#define ADDR_HDR_SIZE    offsetof(ArchNetAddressImpl, m_addr)
#define TYPED_ADDR(type_, addr_) (reinterpret_cast<type_*>(&addr_->m_addr))

//! Win32 implementation of IArchNetwork
class ArchNetworkWinsock : public IArchNetwork {
public:
    ArchNetworkWinsock();
    virtual ~ArchNetworkWinsock();

    virtual void init();

    // IArchNetwork overrides
    virtual ArchSocket    newSocket(EAddressFamily, ESocketType);
    virtual ArchSocket    copySocket(ArchSocket s);
    virtual void        closeSocket(ArchSocket s);
    virtual void        closeSocketForRead(ArchSocket s);
    virtual void        closeSocketForWrite(ArchSocket s);
    virtual void        bindSocket(ArchSocket s, ArchNetAddress addr);
    virtual void        listenOnSocket(ArchSocket s);
    virtual ArchSocket    acceptSocket(ArchSocket s, ArchNetAddress* addr);
    virtual bool        connectSocket(ArchSocket s, ArchNetAddress name);
    virtual int            pollSocket(PollEntry[], int num, double timeout);
    virtual void        unblockPollSocket(ArchThread thread);
    virtual size_t        readSocket(ArchSocket s, void* buf, size_t len);
    virtual size_t        writeSocket(ArchSocket s,
                            const void* buf, size_t len);
    virtual void        throwErrorOnSocket(ArchSocket);
    virtual bool        setNoDelayOnSocket(ArchSocket, bool noDelay);
    virtual bool        setReuseAddrOnSocket(ArchSocket, bool reuse);
    virtual std::string        getHostName();
    virtual ArchNetAddress    newAnyAddr(EAddressFamily);
    virtual ArchNetAddress    copyAddr(ArchNetAddress);
    virtual ArchNetAddress    nameToAddr(const std::string&);
    virtual void            closeAddr(ArchNetAddress);
    virtual std::string        addrToName(ArchNetAddress);
    virtual std::string        addrToString(ArchNetAddress);
    virtual EAddressFamily    getAddrFamily(ArchNetAddress);
    virtual void            setAddrPort(ArchNetAddress, int port);
    virtual int                getAddrPort(ArchNetAddress);
    virtual bool            isAnyAddr(ArchNetAddress);
    virtual bool            isEqualAddr(ArchNetAddress, ArchNetAddress);

private:
    void                initModule(HMODULE);

    void                setBlockingOnSocket(SOCKET, bool blocking);

    void                throwError(int);
    void                throwNameError(int);

private:
    typedef std::list<WSAEVENT> EventList;

    ArchMutex            m_mutex;
    EventList            m_unblockEvents;
};
