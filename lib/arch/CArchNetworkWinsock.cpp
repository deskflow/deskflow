/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include "CArchNetworkWinsock.h"
#include "CArch.h"
#include "XArchWindows.h"

static const int s_family[] = {
	PF_UNSPEC,
	PF_INET
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

#undef FD_ISSET
#define FD_ISSET(fd, set) WSAFDIsSet_winsock((SOCKET)(fd), (fd_set FAR *)(set))

#define setfunc(var, name, type) var = (type)netGetProcAddress(module, #name)

static HMODULE			s_networkModule = NULL;

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

//
// CArchNetworkWinsock
//

CArchNetworkWinsock::CArchNetworkWinsock()
{
	static const char* s_library[] = { "ws2_32.dll", "wsock32.dll" };

	assert(WSACleanup_winsock == NULL);
	assert(s_networkModule    == NULL);

	// try each winsock library
	for (size_t i = 0; i < sizeof(s_library) / sizeof(s_library[0]); ++i) {
		try {
			init((HMODULE)::LoadLibrary(s_library[i]));
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

CArchNetworkWinsock::~CArchNetworkWinsock()
{
	if (s_networkModule != NULL) {
		WSACleanup_winsock();
		::FreeLibrary(s_networkModule);

		WSACleanup_winsock = NULL;
		s_networkModule    = NULL;
	}
	ARCH->closeMutex(m_mutex);
}

void
CArchNetworkWinsock::init(HMODULE module)
{
	assert(module != NULL);

	// get startup function address
	int (PASCAL FAR *startup)(WORD, LPWSADATA);
	setfunc(startup, WSAStartup, int(PASCAL FAR*)(WORD, LPWSADATA));

	// startup network library
	WORD version = MAKEWORD(1 /*major*/, 1 /*minor*/);
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

	s_networkModule = module;
}

CArchSocket
CArchNetworkWinsock::newSocket(EAddressFamily family, ESocketType type)
{
	// allocate socket object
	CArchSocketImpl* socket = new CArchSocketImpl;

	// create socket
	SOCKET fd = socket_winsock(s_family[family], s_type[type], 0);
	if (fd == INVALID_SOCKET) {
		throwError(getsockerror_winsock());
	}

	socket->m_socket    = fd;
	socket->m_connected = false;
	socket->m_refCount  = 1;
	return socket;
}

CArchSocket
CArchNetworkWinsock::copySocket(CArchSocket s)
{
	assert(s != NULL);

	// ref the socket and return it
	ARCH->lockMutex(m_mutex);
	++s->m_refCount;
	ARCH->unlockMutex(m_mutex);
	return s;
}

void
CArchNetworkWinsock::closeSocket(CArchSocket s)
{
	assert(s != NULL);

	// unref the socket and note if it should be released
	ARCH->lockMutex(m_mutex);
	const bool doClose = (--s->m_refCount == 0);
	ARCH->unlockMutex(m_mutex);

	// close the socket if necessary
	if (doClose) {
		do {
			if (close_winsock(s->m_socket) == SOCKET_ERROR) {
				// close failed
				int err = getsockerror_winsock();
				if (err == EINTR) {
					// interrupted system call
					ARCH->testCancelThread();
					continue;
				}

				// restore the last ref and throw
				ARCH->lockMutex(m_mutex);
				++s->m_refCount;
				ARCH->unlockMutex(m_mutex);
				throwError(err);
			}
		} while (false);
		delete s;
	}
}

void
CArchNetworkWinsock::closeSocketForRead(CArchSocket s)
{
	assert(s != NULL);

	if (shutdown_winsock(s->m_socket, SD_RECEIVE) == SOCKET_ERROR) {
		if (getsockerror_winsock() != WSAENOTCONN) {
			throwError(getsockerror_winsock());
		}
	}
}

void
CArchNetworkWinsock::closeSocketForWrite(CArchSocket s)
{
	assert(s != NULL);

	if (shutdown_winsock(s->m_socket, SD_SEND) == SOCKET_ERROR) {
		if (getsockerror_winsock() != WSAENOTCONN) {
			throwError(getsockerror_winsock());
		}
	}
}

void
CArchNetworkWinsock::bindSocket(CArchSocket s, CArchNetAddress addr)
{
	assert(s    != NULL);
	assert(addr != NULL);

	if (bind_winsock(s->m_socket, &addr->m_addr, addr->m_len) == SOCKET_ERROR) {
		throwError(getsockerror_winsock());
	}
}

void
CArchNetworkWinsock::listenOnSocket(CArchSocket s)
{
	assert(s != NULL);

	// hardcoding backlog
	if (listen_winsock(s->m_socket, 3) == SOCKET_ERROR) {
		throwError(getsockerror_winsock());
	}
}

CArchSocket
CArchNetworkWinsock::acceptSocket(CArchSocket s, CArchNetAddress* addr)
{
	assert(s != NULL);

	// if user passed NULL in addr then use scratch space
	CArchNetAddress dummy;
	if (addr == NULL) {
		addr = &dummy;
	}

	// create new socket and address
	CArchSocketImpl* socket = new CArchSocketImpl;
	*addr                   = new CArchNetAddressImpl;

	// accept on socket
	SOCKET fd;
	do {
		fd = accept_winsock(s->m_socket, &(*addr)->m_addr, &(*addr)->m_len);
		if (fd == INVALID_SOCKET) {
			int err = getsockerror_winsock();
			if (err == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			if (err == WSAECONNABORTED) {
				// connection was aborted;  try again
				ARCH->testCancelThread();
				continue;
			}
			delete socket;
			delete *addr;
			*addr = NULL;
			throwError(err);
		}
	} while (false);

	// initialize socket
	socket->m_socket    = fd;
	socket->m_connected = true;
	socket->m_refCount  = 1;

	// discard address if not requested
	if (addr == &dummy) {
		ARCH->closeAddr(dummy);
	}

	return socket;
}

void
CArchNetworkWinsock::connectSocket(CArchSocket s, CArchNetAddress addr)
{
	assert(s    != NULL);
	assert(addr != NULL);

	do {
		if (connect_winsock(s->m_socket, &addr->m_addr,
										addr->m_len) == SOCKET_ERROR) {
			if (getsockerror_winsock() == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}

			if (getsockerror_winsock() == WSAEISCONN) {
				// already connected
				break;
			}

			if (getsockerror_winsock() == WSAEWOULDBLOCK) {
				// connecting
				throw XArchNetworkConnecting(new XArchEvalWinsock(
													getsockerror_winsock()));
			}

			throwError(getsockerror_winsock());
		}
	} while (false);

	ARCH->lockMutex(m_mutex);
	s->m_connected = true;
	ARCH->unlockMutex(m_mutex);
}

int
CArchNetworkWinsock::pollSocket(CPollEntry pe[], int num, double timeout)
{
	int i, n;

	do {
		// prepare sets for select
		n = 0;
		fd_set readSet, writeSet, errSet;
		fd_set* readSetP  = NULL;
		fd_set* writeSetP = NULL;
		fd_set* errSetP   = NULL;
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_ZERO(&errSet);
		for (i = 0; i < num; ++i) {
			// reset return flags
			pe[i].m_revents = 0;

			// set invalid flag if socket is bogus then go to next socket
			if (pe[i].m_socket == NULL) {
				pe[i].m_revents |= kPOLLNVAL;
				continue;
			}

			if (pe[i].m_events & kPOLLIN) {
				FD_SET(pe[i].m_socket->m_socket, &readSet);
				readSetP = &readSet;
				n = 1;
			}
			if (pe[i].m_events & kPOLLOUT) {
				FD_SET(pe[i].m_socket->m_socket, &writeSet);
				writeSetP = &writeSet;
				n = 1;
			}
			if (true) {
				FD_SET(pe[i].m_socket->m_socket, &errSet);
				errSetP = &errSet;
				n = 1;
			}
		}

		// if there are no sockets then don't block forever
		if (n == 0 && timeout < 0.0) {
			timeout = 0.0;
		}

		// prepare timeout for select
		struct timeval timeout2;
		struct timeval* timeout2P;
		if (timeout < 0) {
			timeout2P = NULL;
		}
		else {
			timeout2P = &timeout2;
			timeout2.tv_sec  = static_cast<int>(timeout);
			timeout2.tv_usec = static_cast<int>(1.0e+6 *
											(timeout - timeout2.tv_sec));
		}

		// do the select
		n = select_winsock(0, readSetP, writeSetP, errSetP, timeout2P);

		// handle results
		if (n == SOCKET_ERROR) {
			if (getsockerror_winsock() == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			throwError(getsockerror_winsock());
		}
		n = 0;
		for (i = 0; i < num; ++i) {
			if (pe[i].m_socket != NULL) {
				if (FD_ISSET(pe[i].m_socket->m_socket, &readSet)) {
					pe[i].m_revents |= kPOLLIN;
				}
				if (FD_ISSET(pe[i].m_socket->m_socket, &writeSet)) {
					pe[i].m_revents |= kPOLLOUT;
				}
				if (FD_ISSET(pe[i].m_socket->m_socket, &errSet)) {
					pe[i].m_revents |= kPOLLERR;
				}
			}
			if (pe[i].m_revents != 0) {
				++n;
			}
		}
	} while (false);

	return n;
}

size_t
CArchNetworkWinsock::readSocket(CArchSocket s, void* buf, size_t len)
{
	assert(s != NULL);

	int n;
	do {
		n = recv_winsock(s->m_socket, buf, len, 0);
		if (n == SOCKET_ERROR) {
			if (getsockerror_winsock() == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			throwError(getsockerror_winsock());
		}
	} while (false);
	ARCH->testCancelThread();
	return static_cast<size_t>(n);
}

size_t
CArchNetworkWinsock::writeSocket(CArchSocket s, const void* buf, size_t len)
{
	assert(s != NULL);

	int n;
	do {
		n = send_winsock(s->m_socket, buf, len, 0);
		if (n == SOCKET_ERROR) {
			if (getsockerror_winsock() == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			throwError(getsockerror_winsock());
		}
	} while (false);
	ARCH->testCancelThread();
	return static_cast<size_t>(n);
}

void
CArchNetworkWinsock::throwErrorOnSocket(CArchSocket s)
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

bool
CArchNetworkWinsock::setBlockingOnSocket(CArchSocket s, bool blocking)
{
	assert(s != NULL);

	int flag = blocking ? 0 : 1;
	if (ioctl_winsock(s->m_socket, FIONBIO, &flag) == SOCKET_ERROR) {
		throwError(getsockerror_winsock());
	}
	// FIXME -- can't get the current blocking state of socket?
	return true;
}

bool
CArchNetworkWinsock::setNoDelayOnSocket(CArchSocket s, bool noDelay)
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

std::string
CArchNetworkWinsock::getHostName()
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

CArchNetAddress
CArchNetworkWinsock::newAnyAddr(EAddressFamily family)
{
	// allocate address
	CArchNetAddressImpl* addr = new CArchNetAddressImpl;

	// fill it in
	switch (family) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		ipAddr->sin_family         = AF_INET;
		ipAddr->sin_port           = 0;
		ipAddr->sin_addr.s_addr    = INADDR_ANY;
		addr->m_len                = sizeof(struct sockaddr_in);
		break;
	}

	default:
		delete addr;
		assert(0 && "invalid family");
	}

	return addr;
}

CArchNetAddress
CArchNetworkWinsock::copyAddr(CArchNetAddress addr)
{
	assert(addr != NULL);

	// allocate and copy address
	return new CArchNetAddressImpl(*addr);
}

CArchNetAddress
CArchNetworkWinsock::nameToAddr(const std::string& name)
{
	// allocate address
	CArchNetAddressImpl* addr = new CArchNetAddressImpl;

	// try to convert assuming an IPv4 dot notation address
	struct sockaddr_in inaddr;
	memset(&inaddr, 0, sizeof(inaddr));
	inaddr.sin_addr.s_addr = inet_addr_winsock(name.c_str());
	if (inaddr.sin_addr.s_addr != INADDR_NONE) {
		// it's a dot notation address
		addr->m_len       = sizeof(struct sockaddr_in);
		inaddr.sin_family = AF_INET;
		inaddr.sin_port   = 0;
		memcpy(&addr->m_addr, &inaddr, addr->m_len);
	}

	else {
		// address lookup
		struct hostent* info = gethostbyname_winsock(name.c_str());
		if (info == NULL) {
			delete addr;
			throwNameError(getsockerror_winsock());
		}

		// copy over address (only IPv4 currently supported)
		addr->m_len       = sizeof(struct sockaddr_in);
		inaddr.sin_family = info->h_addrtype;
		inaddr.sin_port   = 0;
		memcpy(&inaddr.sin_addr, info->h_addr_list[0], info->h_length);
		memcpy(&addr->m_addr, &inaddr, addr->m_len);
	}

	return addr;
}

void
CArchNetworkWinsock::closeAddr(CArchNetAddress addr)
{
	assert(addr != NULL);

	delete addr;
}

std::string
CArchNetworkWinsock::addrToName(CArchNetAddress addr)
{
	assert(addr != NULL);

	// name lookup
	struct hostent* info = gethostbyaddr_winsock(
							reinterpret_cast<const char FAR*>(&addr->m_addr),
							addr->m_len, addr->m_addr.sa_family);
	if (info == NULL) {
		throwNameError(getsockerror_winsock());
	}

	// return (primary) name
	return info->h_name;
}

std::string
CArchNetworkWinsock::addrToString(CArchNetAddress addr)
{
	assert(addr != NULL);

	switch (getAddrFamily(addr)) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		return inet_ntoa_winsock(ipAddr->sin_addr);
	}

	default:
		assert(0 && "unknown address family");
		return "";
	}
}

IArchNetwork::EAddressFamily
CArchNetworkWinsock::getAddrFamily(CArchNetAddress addr)
{
	assert(addr != NULL);

	switch (addr->m_addr.sa_family) {
	case AF_INET:
		return kINET;

	default:
		return kUNKNOWN;
	}
}

void
CArchNetworkWinsock::setAddrPort(CArchNetAddress addr, int port)
{
	assert(addr != NULL);

	switch (getAddrFamily(addr)) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		ipAddr->sin_port = htons_winsock(static_cast<u_short>(port));
		break;
	}

	default:
		assert(0 && "unknown address family");
		break;
	}
}

int
CArchNetworkWinsock::getAddrPort(CArchNetAddress addr)
{
	assert(addr != NULL);

	switch (getAddrFamily(addr)) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		return ntohs_winsock(ipAddr->sin_port);
	}

	default:
		assert(0 && "unknown address family");
		return 0;
	}
}

bool
CArchNetworkWinsock::isAnyAddr(CArchNetAddress addr)
{
	assert(addr != NULL);

	switch (getAddrFamily(addr)) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		return (ipAddr->sin_addr.s_addr == INADDR_ANY &&
				addr->m_len == sizeof(struct sockaddr_in));
	}

	default:
		assert(0 && "unknown address family");
		return true;
	}
}

void
CArchNetworkWinsock::throwError(int err)
{
	switch (err) {
	case WSAEWOULDBLOCK:
		throw XArchNetworkWouldBlock(new XArchEvalWinsock(err));

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

	case WSAENETRESET:
	case WSAEDISCON:
	case WSAECONNABORTED:
	case WSAECONNRESET:
		throw XArchNetworkDisconnected(new XArchEvalWinsock(err));

	case WSAECONNREFUSED:
		throw XArchNetworkConnectionRefused(new XArchEvalWinsock(err));

	case WSAEINPROGRESS:
	case WSAEALREADY:
		throw XArchNetworkConnecting(new XArchEvalWinsock(err));

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
CArchNetworkWinsock::throwNameError(int err)
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
