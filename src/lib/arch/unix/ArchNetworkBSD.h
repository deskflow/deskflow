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

#pragma once

#include "arch/IArchNetwork.h"
#include "arch/IArchMultithread.h"

#if HAVE_SYS_TYPES_H
#    include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#    include <sys/socket.h>
#else
struct sockaddr_storage {
	unsigned char       ss_len;         /* address length */
	unsigned char     ss_family;      /* [XSI] address family */
	char                    __ss_pad1[_SS_PAD1SIZE];
	long long       __ss_align;     /* force structure storage alignment */
	char                    __ss_pad2[_SS_PAD2SIZE];
};
#endif

#if !HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

#if HAVE_POLL
#    include <poll.h>
#else
#    if HAVE_SYS_SELECT_H
#        include <sys/select.h>
#    endif
#    if HAVE_SYS_TIME_H
#        include <sys/time.h>
#    endif
#endif

// old systems may use char* for [gs]etsockopt()'s optval argument.
// this should be void on modern systems but char is forwards
// compatible so we always use it.
typedef char optval_t;

#define ARCH_NETWORK ArchNetworkBSD
#define TYPED_ADDR(type_, addr_) (reinterpret_cast<type_*>(&addr_->m_addr))

class ArchSocketImpl {
public:
    int                    m_fd;
    int                    m_refCount;
};

class ArchNetAddressImpl {
public:
    ArchNetAddressImpl() : m_len(sizeof(m_addr)) { }

public:
    struct sockaddr_storage        m_addr;
    socklen_t                      m_len;
};

//! Berkeley (BSD) sockets implementation of IArchNetwork
class ArchNetworkBSD : public IArchNetwork {
public:
    ArchNetworkBSD();
    ArchNetworkBSD(ArchNetworkBSD const &) =delete;
    ArchNetworkBSD(ArchNetworkBSD &&) =delete;
    virtual ~ArchNetworkBSD();

    ArchNetworkBSD& operator=(ArchNetworkBSD const &) =delete;
    ArchNetworkBSD& operator=(ArchNetworkBSD &&) =delete;

    virtual void init();

    // IArchNetwork overrides
    virtual ArchSocket     newSocket(EAddressFamily, ESocketType);
    virtual ArchSocket     copySocket(ArchSocket s);
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
    virtual std::vector<ArchNetAddress> nameToAddr(const std::string&);
    virtual void            closeAddr(ArchNetAddress);
    virtual std::string        addrToName(ArchNetAddress);
    virtual std::string        addrToString(ArchNetAddress);
    virtual EAddressFamily    getAddrFamily(ArchNetAddress);
    virtual void            setAddrPort(ArchNetAddress, int port);
    virtual int                getAddrPort(ArchNetAddress);
    virtual bool            isAnyAddr(ArchNetAddress);
    virtual bool            isEqualAddr(ArchNetAddress, ArchNetAddress);

    struct Connectors 
    {
#if HAVE_POLL
        int (*poll_impl)(struct pollfd *, nfds_t, int);
#endif // HAVE_POLL
        Connectors() {
#if HAVE_POLL
            poll_impl = poll;
#endif // HAVE_POLL
        }
    };
    static Connectors s_connectors;

private:
    const int*            getUnblockPipe();
    const int*            getUnblockPipeForThread(ArchThread);
    void                setBlockingOnSocket(int fd, bool blocking);
    void                throwError(int);
    void                throwNameError(int);

private:
    ArchMutex            m_mutex {};
};
