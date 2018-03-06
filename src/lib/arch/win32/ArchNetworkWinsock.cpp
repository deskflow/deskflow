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

#include "arch/win32/ArchNetworkWinsock.h"
#include "arch/win32/ArchMultithreadWindows.h"
#include "arch/win32/XArchWindows.h"
#include "arch/IArchMultithread.h"
#include "arch/Arch.h"

#include <malloc.h>

static const int s_family[] = {
    PF_UNSPEC,
    PF_INET,
    PF_INET6,
};
static const int s_type[] = {
    SOCK_DGRAM,
    SOCK_STREAM
};

static SOCKET (PASCAL FAR *accept_winsock)(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen);
static int (PASCAL FAR *bind_winsock)(SOCKET s, const struct sockaddr FAR *addr, int namelen);
static int (PASCAL FAR *close_winsock)(SOCKET s);
static int (PASCAL FAR *connect_winsock)(SOCKET s, const struct sockaddr FAR *name, int namelen);
static int (PASCAL FAR *gethostname_winsock)(char FAR * name, int namelen);
static int (PASCAL FAR *getsockerror_winsock)(void);
static int (PASCAL FAR *getsockopt_winsock)(SOCKET s, int level, int optname, void FAR * optval, int FAR *optlen);
static u_short (PASCAL FAR *htons_winsock)(u_short v);
static char FAR * (PASCAL FAR *inet_ntoa_winsock)(struct in_addr in);
static unsigned long (PASCAL FAR *inet_addr_winsock)(const char FAR * cp);
static int (PASCAL FAR *ioctl_winsock)(SOCKET s, int cmd, void FAR * data);
static int (PASCAL FAR *listen_winsock)(SOCKET s, int backlog);
static u_short (PASCAL FAR *ntohs_winsock)(u_short v);
static int (PASCAL FAR *recv_winsock)(SOCKET s, void FAR * buf, int len, int flags);
static int (PASCAL FAR *select_winsock)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout);
static int (PASCAL FAR *send_winsock)(SOCKET s, const void FAR * buf, int len, int flags);
static int (PASCAL FAR *setsockopt_winsock)(SOCKET s, int level, int optname, const void FAR * optval, int optlen);
static int (PASCAL FAR *shutdown_winsock)(SOCKET s, int how);
static SOCKET (PASCAL FAR *socket_winsock)(int af, int type, int protocol);
static struct hostent FAR * (PASCAL FAR *gethostbyaddr_winsock)(const char FAR * addr, int len, int type);
static struct hostent FAR * (PASCAL FAR *gethostbyname_winsock)(const char FAR * name);
static int (PASCAL FAR *WSACleanup_winsock)(void);
static int (PASCAL FAR *WSAFDIsSet_winsock)(SOCKET, fd_set FAR * fdset);
static WSAEVENT (PASCAL FAR *WSACreateEvent_winsock)(void);
static BOOL (PASCAL FAR *WSACloseEvent_winsock)(WSAEVENT);
static BOOL (PASCAL FAR *WSASetEvent_winsock)(WSAEVENT);
static BOOL (PASCAL FAR *WSAResetEvent_winsock)(WSAEVENT);
static int (PASCAL FAR *WSAEventSelect_winsock)(SOCKET, WSAEVENT, long);
static DWORD (PASCAL FAR *WSAWaitForMultipleEvents_winsock)(DWORD, const WSAEVENT FAR*, BOOL, DWORD, BOOL);
static int (PASCAL FAR *WSAEnumNetworkEvents_winsock)(SOCKET, WSAEVENT, LPWSANETWORKEVENTS);

#undef FD_ISSET
#define FD_ISSET(fd, set) WSAFDIsSet_winsock((SOCKET)(fd), (fd_set FAR *)(set))

#define setfunc(var, name, type) var = (type)netGetProcAddress(module, #name)

static HMODULE            s_networkModule = NULL;

static
FARPROC
netGetProcAddress(HMODULE module, LPCSTR name)
{
    FARPROC func = ::GetProcAddress(module, name);
    if (!func) {
        throw XArchNetworkSupport("");
    }
    return func;
}

ArchNetAddressImpl*
ArchNetAddressImpl::alloc(size_t size)
{
    size_t totalSize = size + ADDR_HDR_SIZE;
    ArchNetAddressImpl* addr = (ArchNetAddressImpl*)malloc(totalSize);
    addr->m_len = (int)size;
    return addr;
}


//
// ArchNetworkWinsock
//

ArchNetworkWinsock::ArchNetworkWinsock() :
    m_mutex(NULL)
{
}

ArchNetworkWinsock::~ArchNetworkWinsock()
{
    if (s_networkModule != NULL) {
        WSACleanup_winsock();
        ::FreeLibrary(s_networkModule);

        WSACleanup_winsock = NULL;
        s_networkModule    = NULL;
    }
    if (m_mutex != NULL) {
        ARCH->closeMutex(m_mutex);
    }

    EventList::iterator it;
    for (it = m_unblockEvents.begin(); it != m_unblockEvents.end(); it++) {
        delete *it;
    }
}

void
ArchNetworkWinsock::init()
{
    static const char* s_library[] = { "ws2_32.dll" };

    assert(WSACleanup_winsock == NULL);
    assert(s_networkModule    == NULL);

    // try each winsock library
    for (size_t i = 0; i < sizeof(s_library) / sizeof(s_library[0]); ++i) {
        try {
            initModule((HMODULE)::LoadLibrary(s_library[i]));
            m_mutex = ARCH->newMutex();
            return;
        }
        catch (XArchNetwork&) {
            // ignore
        }
    }

    // can't initialize any library
    throw XArchNetworkSupport("Cannot load winsock library");
}

void
ArchNetworkWinsock::initModule(HMODULE module)
{
    if (module == NULL) {
        throw XArchNetworkSupport("");
    }

    // get startup function address
    int (PASCAL FAR *startup)(WORD, LPWSADATA);
    setfunc(startup, WSAStartup, int(PASCAL FAR*)(WORD, LPWSADATA));

    // startup network library
    WORD version = MAKEWORD(2 /*major*/, 2 /*minor*/);
    WSADATA data;
    int err = startup(version, &data);
    if (data.wVersion != version) {
        throw XArchNetworkSupport(new XArchEvalWinsock(err));
    }
    if (err != 0) {
        // some other initialization error
        throwError(err);
    }

    // get function addresses
    setfunc(accept_winsock, accept, SOCKET (PASCAL FAR *)(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen));
    setfunc(bind_winsock, bind, int (PASCAL FAR *)(SOCKET s, const struct sockaddr FAR *addr, int namelen));
    setfunc(close_winsock, closesocket, int (PASCAL FAR *)(SOCKET s));
    setfunc(connect_winsock, connect, int (PASCAL FAR *)(SOCKET s, const struct sockaddr FAR *name, int namelen));
    setfunc(gethostname_winsock, gethostname, int (PASCAL FAR *)(char FAR * name, int namelen));
    setfunc(getsockerror_winsock, WSAGetLastError, int (PASCAL FAR *)(void));
    setfunc(getsockopt_winsock, getsockopt, int (PASCAL FAR *)(SOCKET s, int level, int optname, void FAR * optval, int FAR *optlen));
    setfunc(htons_winsock, htons, u_short (PASCAL FAR *)(u_short v));
    setfunc(inet_ntoa_winsock, inet_ntoa, char FAR * (PASCAL FAR *)(struct in_addr in));
    setfunc(inet_addr_winsock, inet_addr, unsigned long (PASCAL FAR *)(const char FAR * cp));
    setfunc(ioctl_winsock, ioctlsocket, int (PASCAL FAR *)(SOCKET s, int cmd, void FAR *));
    setfunc(listen_winsock, listen, int (PASCAL FAR *)(SOCKET s, int backlog));
    setfunc(ntohs_winsock, ntohs, u_short (PASCAL FAR *)(u_short v));
    setfunc(recv_winsock, recv, int (PASCAL FAR *)(SOCKET s, void FAR * buf, int len, int flags));
    setfunc(select_winsock, select, int (PASCAL FAR *)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout));
    setfunc(send_winsock, send, int (PASCAL FAR *)(SOCKET s, const void FAR * buf, int len, int flags));
    setfunc(setsockopt_winsock, setsockopt, int (PASCAL FAR *)(SOCKET s, int level, int optname, const void FAR * optval, int optlen));
    setfunc(shutdown_winsock, shutdown, int (PASCAL FAR *)(SOCKET s, int how));
    setfunc(socket_winsock, socket, SOCKET (PASCAL FAR *)(int af, int type, int protocol));
    setfunc(gethostbyaddr_winsock, gethostbyaddr, struct hostent FAR * (PASCAL FAR *)(const char FAR * addr, int len, int type));
    setfunc(gethostbyname_winsock, gethostbyname, struct hostent FAR * (PASCAL FAR *)(const char FAR * name));
    setfunc(WSACleanup_winsock, WSACleanup, int (PASCAL FAR *)(void));
    setfunc(WSAFDIsSet_winsock, __WSAFDIsSet, int (PASCAL FAR *)(SOCKET, fd_set FAR *));
    setfunc(WSACreateEvent_winsock, WSACreateEvent, WSAEVENT (PASCAL FAR *)(void));
    setfunc(WSACloseEvent_winsock, WSACloseEvent, BOOL (PASCAL FAR *)(WSAEVENT));
    setfunc(WSASetEvent_winsock, WSASetEvent, BOOL (PASCAL FAR *)(WSAEVENT));
    setfunc(WSAResetEvent_winsock, WSAResetEvent, BOOL (PASCAL FAR *)(WSAEVENT));
    setfunc(WSAEventSelect_winsock, WSAEventSelect, int (PASCAL FAR *)(SOCKET, WSAEVENT, long));
    setfunc(WSAWaitForMultipleEvents_winsock, WSAWaitForMultipleEvents, DWORD (PASCAL FAR *)(DWORD, const WSAEVENT FAR*, BOOL, DWORD, BOOL));
    setfunc(WSAEnumNetworkEvents_winsock, WSAEnumNetworkEvents, int (PASCAL FAR *)(SOCKET, WSAEVENT, LPWSANETWORKEVENTS));

    s_networkModule = module;
}

ArchSocket
ArchNetworkWinsock::newSocket(EAddressFamily family, ESocketType type)
{
    // create socket
    SOCKET fd = socket_winsock(s_family[family], s_type[type], 0);
    if (fd == INVALID_SOCKET) {
        throwError(getsockerror_winsock());
    }
    try {
        setBlockingOnSocket(fd, false);
        BOOL flag = 0;
        int size     = sizeof(flag);
        if (setsockopt_winsock(fd, IPPROTO_IPV6, IPV6_V6ONLY, &flag, size) == SOCKET_ERROR) {
            throwError(getsockerror_winsock());
        }
    }
    catch (...) {
        close_winsock(fd);
        throw;
    }

    // allocate socket object
    ArchSocketImpl* socket = new ArchSocketImpl;
    socket->m_socket        = fd;
    socket->m_refCount      = 1;
    socket->m_event         = WSACreateEvent_winsock();
    socket->m_pollWrite     = true;
    return socket;
}

ArchSocket
ArchNetworkWinsock::copySocket(ArchSocket s)
{
    assert(s != NULL);

    // ref the socket and return it
    ARCH->lockMutex(m_mutex);
    ++s->m_refCount;
    ARCH->unlockMutex(m_mutex);
    return s;
}

void
ArchNetworkWinsock::closeSocket(ArchSocket s)
{
    assert(s != NULL);

    // unref the socket and note if it should be released
    ARCH->lockMutex(m_mutex);
    const bool doClose = (--s->m_refCount == 0);
    ARCH->unlockMutex(m_mutex);

    // close the socket if necessary
    if (doClose) {
        if (close_winsock(s->m_socket) == SOCKET_ERROR) {
            // close failed.  restore the last ref and throw.
            int err = getsockerror_winsock();
            ARCH->lockMutex(m_mutex);
            ++s->m_refCount;
            ARCH->unlockMutex(m_mutex);
            throwError(err);
        }
        WSACloseEvent_winsock(s->m_event);
        delete s;
    }
}

void
ArchNetworkWinsock::closeSocketForRead(ArchSocket s)
{
    assert(s != NULL);

    if (shutdown_winsock(s->m_socket, SD_RECEIVE) == SOCKET_ERROR) {
        if (getsockerror_winsock() != WSAENOTCONN) {
            throwError(getsockerror_winsock());
        }
    }
}

void
ArchNetworkWinsock::closeSocketForWrite(ArchSocket s)
{
    assert(s != NULL);

    if (shutdown_winsock(s->m_socket, SD_SEND) == SOCKET_ERROR) {
        if (getsockerror_winsock() != WSAENOTCONN) {
            throwError(getsockerror_winsock());
        }
    }
}

void
ArchNetworkWinsock::bindSocket(ArchSocket s, ArchNetAddress addr)
{
    assert(s    != NULL);
    assert(addr != NULL);

    if (bind_winsock(s->m_socket, TYPED_ADDR(struct sockaddr, addr), addr->m_len) == SOCKET_ERROR) {
        throwError(getsockerror_winsock());
    }
}

void
ArchNetworkWinsock::listenOnSocket(ArchSocket s)
{
    assert(s != NULL);

    // hardcoding backlog
    if (listen_winsock(s->m_socket, 3) == SOCKET_ERROR) {
        throwError(getsockerror_winsock());
    }
}

ArchSocket
ArchNetworkWinsock::acceptSocket(ArchSocket s, ArchNetAddress* const addr)
{
    assert(s != NULL);

    // create new socket and temporary address
    ArchSocketImpl* socket = new ArchSocketImpl;
    ArchNetAddress tmp = ArchNetAddressImpl::alloc(sizeof(struct sockaddr_in6));

    // accept on socket
    SOCKET fd = accept_winsock(s->m_socket, TYPED_ADDR(struct sockaddr, tmp), &tmp->m_len);
    if (fd == INVALID_SOCKET) {
        int err = getsockerror_winsock();
        delete socket;
        free(tmp);
        if (addr) {
            *addr = NULL;
        }
        if (err == WSAEWOULDBLOCK) {
            return NULL;
        }
        throwError(err);
    }

    try {
        setBlockingOnSocket(fd, false);
    }
    catch (...) {
        close_winsock(fd);
        delete socket;
        free(tmp);
        if (addr) {
            *addr = NULL;
        }
        throw;
    }

    // initialize socket
    socket->m_socket    = fd;
    socket->m_refCount  = 1;
    socket->m_event     = WSACreateEvent_winsock();
    socket->m_pollWrite = true;

    // copy address if requested
    if (addr != NULL) {
        *addr = ARCH->copyAddr(tmp);
    }

    free(tmp);
    return socket;
}

bool
ArchNetworkWinsock::connectSocket(ArchSocket s, ArchNetAddress addr)
{
    assert(s    != NULL);
    assert(addr != NULL);

    if (connect_winsock(s->m_socket, TYPED_ADDR(struct sockaddr, addr),
                            addr->m_len) == SOCKET_ERROR) {
        if (getsockerror_winsock() == WSAEISCONN) {
            return true;
        }
        if (getsockerror_winsock() == WSAEWOULDBLOCK) {
            return false;
        }
        throwError(getsockerror_winsock());
    }
    return true;
}

int
ArchNetworkWinsock::pollSocket(PollEntry pe[], int num, double timeout)
{
    int i;
    DWORD n;

    // prepare sockets and wait list
    bool canWrite = false;
    WSAEVENT* events = (WSAEVENT*)alloca((num + 1) * sizeof(WSAEVENT));
    for (i = 0, n = 0; i < num; ++i) {
        // reset return flags
        pe[i].m_revents = 0;

        // set invalid flag if socket is bogus then go to next socket
        if (pe[i].m_socket == NULL) {
            pe[i].m_revents |= kPOLLNVAL;
            continue;
        }

        // select desired events
        long socketEvents = 0;
        if ((pe[i].m_events & kPOLLIN) != 0) {
            socketEvents |= FD_READ | FD_ACCEPT | FD_CLOSE;
        }
        if ((pe[i].m_events & kPOLLOUT) != 0) {
            socketEvents |= FD_WRITE | FD_CONNECT | FD_CLOSE;

            // if m_pollWrite is false then we assume the socket is
            // writable.  winsock doesn't signal writability except
            // when the state changes from unwritable.
            if (!pe[i].m_socket->m_pollWrite) {
                canWrite         = true;
                pe[i].m_revents |= kPOLLOUT;
            }
        }

        // if no events then ignore socket
        if (socketEvents == 0) {
            continue;
        }

        // select socket for desired events
        WSAEventSelect_winsock(pe[i].m_socket->m_socket,
                            pe[i].m_socket->m_event, socketEvents);

        // add socket event to wait list
        events[n++] = pe[i].m_socket->m_event;
    }

    // if no sockets then return immediately
    if (n == 0) {
        return 0;
    }

    // add the unblock event
    ArchMultithreadWindows* mt = ArchMultithreadWindows::getInstance();
    ArchThread thread     = mt->newCurrentThread();
    WSAEVENT* unblockEvent = (WSAEVENT*)mt->getNetworkDataForThread(thread);
    ARCH->closeThread(thread);
    if (unblockEvent == NULL) {
        unblockEvent  = new WSAEVENT;
        m_unblockEvents.push_back(unblockEvent);
        *unblockEvent = WSACreateEvent_winsock();
        mt->setNetworkDataForCurrentThread(unblockEvent);
    }
    events[n++] = *unblockEvent;

    // prepare timeout
    DWORD t = (timeout < 0.0) ? INFINITE : (DWORD)(1000.0 * timeout);
    if (canWrite) {
        // if we know we can write then don't block
        t = 0;
    }

    // wait
    DWORD result = WSAWaitForMultipleEvents_winsock(n, events, FALSE, t, FALSE);

    // reset the unblock event
    WSAResetEvent_winsock(*unblockEvent);

    // handle results
    if (result == WSA_WAIT_FAILED) {
        if (getsockerror_winsock() == WSAEINTR) {
            // interrupted system call
            ARCH->testCancelThread();
            return 0;
        }
        throwError(getsockerror_winsock());
    }
    if (result == WSA_WAIT_TIMEOUT && !canWrite) {
        return 0;
    }
    if (result == WSA_WAIT_EVENT_0 + n - 1) {
        // the unblock event was signalled
        return 0;
    }
    for (i = 0, n = 0; i < num; ++i) {
        // skip events we didn't check
        if (pe[i].m_socket == NULL ||
            (pe[i].m_events & (kPOLLIN | kPOLLOUT)) == 0) {
            continue;
        }

        // get events
        WSANETWORKEVENTS info;
        if (WSAEnumNetworkEvents_winsock(pe[i].m_socket->m_socket,
                            pe[i].m_socket->m_event, &info) == SOCKET_ERROR) {
            continue;
        }
        if ((info.lNetworkEvents & FD_READ) != 0) {
            pe[i].m_revents |= kPOLLIN;
        }
        if ((info.lNetworkEvents & FD_ACCEPT) != 0) {
            pe[i].m_revents |= kPOLLIN;
        }
        if ((info.lNetworkEvents & FD_WRITE) != 0) {
            pe[i].m_revents |= kPOLLOUT;

            // socket is now writable so don't bothing polling for
            // writable until it becomes unwritable.
            pe[i].m_socket->m_pollWrite = false;
        }
        if ((info.lNetworkEvents & FD_CONNECT) != 0) {
            if (info.iErrorCode[FD_CONNECT_BIT] != 0) {
                pe[i].m_revents |= kPOLLERR;
            }
            else {
                pe[i].m_revents |= kPOLLOUT;
                pe[i].m_socket->m_pollWrite = false;
            }
        }
        if ((info.lNetworkEvents & FD_CLOSE) != 0) {
            if (info.iErrorCode[FD_CLOSE_BIT] != 0) {
                pe[i].m_revents |= kPOLLERR;
            }
            else {
                if ((pe[i].m_events & kPOLLIN) != 0) {
                    pe[i].m_revents |= kPOLLIN;
                }
                if ((pe[i].m_events & kPOLLOUT) != 0) {
                    pe[i].m_revents |= kPOLLOUT;
                }
            }
        }
        if (pe[i].m_revents != 0) {
            ++n;
        }
    }

    return (int)n;
}

void
ArchNetworkWinsock::unblockPollSocket(ArchThread thread)
{
    // set the unblock event
    ArchMultithreadWindows* mt = ArchMultithreadWindows::getInstance();
    WSAEVENT* unblockEvent = (WSAEVENT*)mt->getNetworkDataForThread(thread);
    if (unblockEvent != NULL) {
        WSASetEvent_winsock(*unblockEvent);
    }
}

size_t
ArchNetworkWinsock::readSocket(ArchSocket s, void* buf, size_t len)
{
    assert(s != NULL);

    int n = recv_winsock(s->m_socket, buf, (int)len, 0);
    if (n == SOCKET_ERROR) {
        int err = getsockerror_winsock();
        if (err == WSAEINTR || err == WSAEWOULDBLOCK) {
            return 0;
        }
        throwError(err);
    }
    return static_cast<size_t>(n);
}

size_t
ArchNetworkWinsock::writeSocket(ArchSocket s, const void* buf, size_t len)
{
    assert(s != NULL);

    int n = send_winsock(s->m_socket, buf, (int)len, 0);
    if (n == SOCKET_ERROR) {
        int err = getsockerror_winsock();
        if (err == WSAEINTR) {
            return 0;
        }
        if (err == WSAEWOULDBLOCK) {
            s->m_pollWrite = true;
            return 0;
        }
        throwError(err);
    }
    return static_cast<size_t>(n);
}

void
ArchNetworkWinsock::throwErrorOnSocket(ArchSocket s)
{
    assert(s != NULL);

    // get the error from the socket layer
    int err  = 0;
    int size = sizeof(err);
    if (getsockopt_winsock(s->m_socket, SOL_SOCKET,
                                    SO_ERROR, &err, &size) == SOCKET_ERROR) {
        err = getsockerror_winsock();
    }

    // throw if there's an error
    if (err != 0) {
        throwError(err);
    }
}

void
ArchNetworkWinsock::setBlockingOnSocket(SOCKET s, bool blocking)
{
    assert(s != 0);

    int flag = blocking ? 0 : 1;
    if (ioctl_winsock(s, FIONBIO, &flag) == SOCKET_ERROR) {
        throwError(getsockerror_winsock());
    }
}

bool
ArchNetworkWinsock::setNoDelayOnSocket(ArchSocket s, bool noDelay)
{
    assert(s != NULL);

    // get old state
    BOOL oflag;
    int size = sizeof(oflag);
    if (getsockopt_winsock(s->m_socket, IPPROTO_TCP,
                                TCP_NODELAY, &oflag, &size) == SOCKET_ERROR) {
        throwError(getsockerror_winsock());
    }

    // set new state
    BOOL flag = noDelay ? 1 : 0;
    size     = sizeof(flag);
    if (setsockopt_winsock(s->m_socket, IPPROTO_TCP,
                                TCP_NODELAY, &flag, size) == SOCKET_ERROR) {
        throwError(getsockerror_winsock());
    }

    return (oflag != 0);
}

bool
ArchNetworkWinsock::setReuseAddrOnSocket(ArchSocket s, bool reuse)
{
    assert(s != NULL);

    // get old state
    BOOL oflag;
    int size = sizeof(oflag);
    if (getsockopt_winsock(s->m_socket, SOL_SOCKET,
                                SO_REUSEADDR, &oflag, &size) == SOCKET_ERROR) {
        throwError(getsockerror_winsock());
    }

    // set new state
    BOOL flag = reuse ? 1 : 0;
    size     = sizeof(flag);
    if (setsockopt_winsock(s->m_socket, SOL_SOCKET,
                                SO_REUSEADDR, &flag, size) == SOCKET_ERROR) {
        throwError(getsockerror_winsock());
    }

    return (oflag != 0);
}

std::string
ArchNetworkWinsock::getHostName()
{
    char name[256];
    if (gethostname_winsock(name, sizeof(name)) == -1) {
        name[0] = '\0';
    }
    else {
        name[sizeof(name) - 1] = '\0';
    }
    return name;
}

ArchNetAddress
ArchNetworkWinsock::newAnyAddr(EAddressFamily family)
{
    ArchNetAddressImpl* addr = NULL;
    switch (family) {
    case kINET: {
        addr = ArchNetAddressImpl::alloc(sizeof(struct sockaddr_in));
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
        ipAddr->sin_family         = AF_INET;
        ipAddr->sin_port           = 0;
        ipAddr->sin_addr.s_addr    = INADDR_ANY;
        break;
    }

    case kINET6: {
        addr = ArchNetAddressImpl::alloc(sizeof(struct sockaddr_in6));
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
        ipAddr->sin6_family         = AF_INET6;
        ipAddr->sin6_port           = 0;
        memcpy(&ipAddr->sin6_addr, &in6addr_any, sizeof(in6addr_any));
        break;
    }

    default:
        assert(0 && "invalid family");
    }
    return addr;
}

ArchNetAddress
ArchNetworkWinsock::copyAddr(ArchNetAddress addr)
{
    assert(addr != NULL);

    ArchNetAddressImpl* copy = ArchNetAddressImpl::alloc(addr->m_len);
    memcpy(TYPED_ADDR(void, copy), TYPED_ADDR(void, addr), addr->m_len);
    return copy;
}

ArchNetAddress
ArchNetworkWinsock::nameToAddr(const std::string& name)
{
    // allocate address

    ArchNetAddressImpl* addr = new ArchNetAddressImpl;

    struct addrinfo hints;
    struct addrinfo *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    int ret = -1;

    ARCH->lockMutex(m_mutex);
    if ((ret = getaddrinfo(name.c_str(), NULL, &hints, &p)) != 0) {
        ARCH->unlockMutex(m_mutex);
        delete addr;
        throwNameError(ret);
    }

    if (p->ai_family == AF_INET) {
        addr->m_len = (socklen_t)sizeof(struct sockaddr_in);
    } else {
        addr->m_len = (socklen_t)sizeof(struct sockaddr_in6);
    }

    memcpy(&addr->m_addr, p->ai_addr, addr->m_len);
    freeaddrinfo(p);
    ARCH->unlockMutex(m_mutex);
    return addr;
}

void
ArchNetworkWinsock::closeAddr(ArchNetAddress addr)
{
    assert(addr != NULL);

    free(addr);
}

std::string
ArchNetworkWinsock::addrToName(ArchNetAddress addr)
{
    assert(addr != NULL);

    char host[1024];
    char service[20];
    int ret = getnameinfo(TYPED_ADDR(struct sockaddr, addr), addr->m_len, host, sizeof(host), service, sizeof(service), 0);

    if (ret  != NULL) {
        throwNameError(ret);
    }

    // return (primary) name
    std::string name = host;
    return name;
}

std::string
ArchNetworkWinsock::addrToString(ArchNetAddress addr)
{
    assert(addr != NULL);

    switch (getAddrFamily(addr)) {
    case kINET: {
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
        return inet_ntoa_winsock(ipAddr->sin_addr);
    }

    case kINET6: {
        char strAddr[INET6_ADDRSTRLEN];
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
        inet_ntop(AF_INET6, &ipAddr->sin6_addr, strAddr, INET6_ADDRSTRLEN);
        return strAddr;
    }

    default:
        assert(0 && "unknown address family");
        return "";
    }
}

IArchNetwork::EAddressFamily
ArchNetworkWinsock::getAddrFamily(ArchNetAddress addr)
{
    assert(addr != NULL);

    switch (addr->m_addr.ss_family) {
    case AF_INET:
        return kINET;

    case AF_INET6:
        return kINET6;

    default:
        return kUNKNOWN;
    }
}

void
ArchNetworkWinsock::setAddrPort(ArchNetAddress addr, int port)
{
    assert(addr != NULL);

    switch (getAddrFamily(addr)) {
    case kINET: {
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
        ipAddr->sin_port = htons_winsock(static_cast<u_short>(port));
        break;
    }

    case kINET6: {
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
        ipAddr->sin6_port = htons_winsock(static_cast<u_short>(port));
        break;
    }

    default:
        assert(0 && "unknown address family");
        break;
    }
}

int
ArchNetworkWinsock::getAddrPort(ArchNetAddress addr)
{
    assert(addr != NULL);

    switch (getAddrFamily(addr)) {
    case kINET: {
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
        return ntohs_winsock(ipAddr->sin_port);
    }

    case kINET6: {
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
        return ntohs_winsock(ipAddr->sin6_port);
    }

    default:
        assert(0 && "unknown address family");
        return 0;
    }
}

bool
ArchNetworkWinsock::isAnyAddr(ArchNetAddress addr)
{
    assert(addr != NULL);

    switch (getAddrFamily(addr)) {
    case kINET: {
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
        return (addr->m_len == sizeof(struct sockaddr_in) &&
                ipAddr->sin_addr.s_addr == INADDR_ANY);
    }

    case kINET6: {
        auto* ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
        return (addr->m_len == sizeof(struct sockaddr_in) &&
                memcmp(&ipAddr->sin6_addr, &in6addr_any, sizeof(in6addr_any))== 0);
    }

    default:
        assert(0 && "unknown address family");
        return true;
    }
}

bool
ArchNetworkWinsock::isEqualAddr(ArchNetAddress a, ArchNetAddress b)
{
    return (a == b || (a->m_len == b->m_len &&
            memcmp(&a->m_addr, &b->m_addr, a->m_len) == 0));
}

void
ArchNetworkWinsock::throwError(int err)
{
    switch (err) {
    case WSAEACCES:
        throw XArchNetworkAccess(new XArchEvalWinsock(err));

    case WSAEMFILE:
    case WSAENOBUFS:
    case WSAENETDOWN:
        throw XArchNetworkResource(new XArchEvalWinsock(err));

    case WSAEPROTOTYPE:
    case WSAEPROTONOSUPPORT:
    case WSAEAFNOSUPPORT:
    case WSAEPFNOSUPPORT:
    case WSAESOCKTNOSUPPORT:
    case WSAEINVAL:
    case WSAENOPROTOOPT:
    case WSAEOPNOTSUPP:
    case WSAESHUTDOWN:
    case WSANOTINITIALISED:
    case WSAVERNOTSUPPORTED:
    case WSASYSNOTREADY:
        throw XArchNetworkSupport(new XArchEvalWinsock(err));

    case WSAEADDRNOTAVAIL:
        throw XArchNetworkNoAddress(new XArchEvalWinsock(err));

    case WSAEADDRINUSE:
        throw XArchNetworkAddressInUse(new XArchEvalWinsock(err));

    case WSAEHOSTUNREACH:
    case WSAENETUNREACH:
        throw XArchNetworkNoRoute(new XArchEvalWinsock(err));

    case WSAENOTCONN:
        throw XArchNetworkNotConnected(new XArchEvalWinsock(err));

    case WSAEDISCON:
        throw XArchNetworkShutdown(new XArchEvalWinsock(err));

    case WSAENETRESET:
    case WSAECONNABORTED:
    case WSAECONNRESET:
        throw XArchNetworkDisconnected(new XArchEvalWinsock(err));

    case WSAECONNREFUSED:
        throw XArchNetworkConnectionRefused(new XArchEvalWinsock(err));

    case WSAEHOSTDOWN:
    case WSAETIMEDOUT:
        throw XArchNetworkTimedOut(new XArchEvalWinsock(err));

    case WSAHOST_NOT_FOUND:
        throw XArchNetworkNameUnknown(new XArchEvalWinsock(err));

    case WSANO_DATA:
        throw XArchNetworkNameNoAddress(new XArchEvalWinsock(err));

    case WSANO_RECOVERY:
        throw XArchNetworkNameFailure(new XArchEvalWinsock(err));

    case WSATRY_AGAIN:
        throw XArchNetworkNameUnavailable(new XArchEvalWinsock(err));

    default:
        throw XArchNetwork(new XArchEvalWinsock(err));
    }
}

void
ArchNetworkWinsock::throwNameError(int err)
{
    switch (err) {
    case WSAHOST_NOT_FOUND:
        throw XArchNetworkNameUnknown(new XArchEvalWinsock(err));

    case WSANO_DATA:
        throw XArchNetworkNameNoAddress(new XArchEvalWinsock(err));

    case WSANO_RECOVERY:
        throw XArchNetworkNameFailure(new XArchEvalWinsock(err));

    case WSATRY_AGAIN:
        throw XArchNetworkNameUnavailable(new XArchEvalWinsock(err));

    default:
        throw XArchNetworkName(new XArchEvalWinsock(err));
    }
}
