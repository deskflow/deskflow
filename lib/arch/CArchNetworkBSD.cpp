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

#include "CArchNetworkBSD.h"
#include "CArch.h"
#include "XArchUnix.h"
#if HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#if !defined(TCP_NODELAY)
#	include <netinet/tcp.h>
#endif
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#if HAVE_POLL
#	include <sys/poll.h>
#else
#	if HAVE_SYS_SELECT_H
#		include <sys/select.h>
#	endif
#	if HAVE_SYS_TIME_H
#		include <sys/time.h>
#	endif
#endif

static const int s_family[] = {
	PF_UNSPEC,
	PF_INET
};
static const int s_type[] = {
	SOCK_DGRAM,
	SOCK_STREAM
};

//
// CArchNetworkBSD
//

CArchNetworkBSD::CArchNetworkBSD()
{
	// create mutex to make some calls thread safe
	m_mutex = ARCH->newMutex();
}

CArchNetworkBSD::~CArchNetworkBSD()
{
	ARCH->closeMutex(m_mutex);
}

CArchSocket
CArchNetworkBSD::newSocket(EAddressFamily family, ESocketType type)
{
	// allocate socket object
	CArchSocketImpl* newSocket = new CArchSocketImpl;

	// create socket
	int fd = socket(s_family[family], s_type[type], 0);
	if (fd == -1) {
		throwError(errno);
	}

	newSocket->m_fd        = fd;
	newSocket->m_connected = false;
	newSocket->m_refCount  = 1;
	return newSocket;
}

CArchSocket
CArchNetworkBSD::copySocket(CArchSocket s)
{
	assert(s != NULL);

	// ref the socket and return it
	ARCH->lockMutex(m_mutex);
	++s->m_refCount;
	ARCH->unlockMutex(m_mutex);
	return s;
}

void
CArchNetworkBSD::closeSocket(CArchSocket s)
{
	assert(s != NULL);

	// unref the socket and note if it should be released
	ARCH->lockMutex(m_mutex);
	const bool doClose = (--s->m_refCount == 0);
	ARCH->unlockMutex(m_mutex);

	// close the socket if necessary
	if (doClose) {
		do {
			if (close(s->m_fd) == -1) {
				// close failed
				int err = errno;
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
CArchNetworkBSD::closeSocketForRead(CArchSocket s)
{
	assert(s != NULL);

	if (shutdown(s->m_fd, 0) == -1) {
		if (errno != ENOTCONN) {
			throwError(errno);
		}
	}
}

void
CArchNetworkBSD::closeSocketForWrite(CArchSocket s)
{
	assert(s != NULL);

	if (shutdown(s->m_fd, 1) == -1) {
		if (errno != ENOTCONN) {
			throwError(errno);
		}
	}
}

void
CArchNetworkBSD::bindSocket(CArchSocket s, CArchNetAddress addr)
{
	assert(s    != NULL);
	assert(addr != NULL);

	if (bind(s->m_fd, &addr->m_addr, addr->m_len) == -1) {
		throwError(errno);
	}
}

void
CArchNetworkBSD::listenOnSocket(CArchSocket s)
{
	assert(s != NULL);

	// hardcoding backlog
	if (listen(s->m_fd, 3) == -1) {
		throwError(errno);
	}
}

CArchSocket
CArchNetworkBSD::acceptSocket(CArchSocket s, CArchNetAddress* addr)
{
	assert(s != NULL);

	// if user passed NULL in addr then use scratch space
	CArchNetAddress dummy;
	if (addr == NULL) {
		addr = &dummy;
	}

	// create new socket and address
	CArchSocketImpl* newSocket = new CArchSocketImpl;
	*addr                      = new CArchNetAddressImpl;

	// accept on socket
	int fd;
	do {
		fd = accept(s->m_fd, &(*addr)->m_addr, &(*addr)->m_len);
		if (fd == -1) {
			int err = errno;
			if (err == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			if (err == ECONNABORTED) {
				// connection was aborted;  try again
				ARCH->testCancelThread();
				continue;
			}
			delete newSocket;
			delete *addr;
			*addr = NULL;
			throwError(err);
		}
	} while (false);

	// initialize socket
	newSocket->m_fd        = fd;
	newSocket->m_connected = true;
	newSocket->m_refCount  = 1;

	// discard address if not requested
	if (addr == &dummy) {
		ARCH->closeAddr(dummy);
	}

	return newSocket;
}

void
CArchNetworkBSD::connectSocket(CArchSocket s, CArchNetAddress addr)
{
	assert(s    != NULL);
	assert(addr != NULL);

	do {
		if (connect(s->m_fd, &addr->m_addr, addr->m_len) == -1) {
			if (errno == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}

			if (errno == EISCONN) {
				// already connected
				break;
			}

			if (errno == EAGAIN) {
				// connecting
				throw XArchNetworkConnecting(new XArchEvalUnix(errno));
			}

			throwError(errno);
		}
	} while (false);

	ARCH->lockMutex(m_mutex);
	s->m_connected = true;
	ARCH->unlockMutex(m_mutex);
}

#if HAVE_POLL

int
CArchNetworkBSD::pollSocket(CPollEntry pe[], int num, double timeout)
{
	assert(pe != NULL || num == 0);

	// return if nothing to do
	if (num == 0) {
		if (timeout > 0.0) {
			ARCH->sleep(timeout);
		}
		return 0;
	}

	// allocate space for translated query
	struct pollfd* pfd = new struct pollfd[num];

	// translate query
	for (int i = 0; i < num; ++i) {
		pfd[i].fd     = (pe[i].m_socket == NULL) ? -1 : pe[i].m_socket->m_fd;
		pfd[i].events = 0;
		if ((pe[i].m_events & kPOLLIN) != 0) {
			pfd[i].events |= POLLIN;
		}
		if ((pe[i].m_events & kPOLLOUT) != 0) {
			pfd[i].events |= POLLOUT;
		}
	}

	// do the poll
	int n;
	do {
		n = poll(pfd, num, static_cast<int>(1000.0 * timeout));
		if (n == -1) {
			if (errno == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			delete[] pfd;
			throwError(errno);
		}
	} while (false);

	// translate back
	for (int i = 0; i < num; ++i) {
		pe[i].m_revents = 0;
		if ((pfd[i].revents & POLLIN) != 0) {
			pe[i].m_revents |= kPOLLIN;
		}
		if ((pfd[i].revents & POLLOUT) != 0) {
			pe[i].m_revents |= kPOLLOUT;
		}
		if ((pfd[i].revents & POLLERR) != 0) {
			pe[i].m_revents |= kPOLLERR;
		}
		if ((pfd[i].revents & POLLNVAL) != 0) {
			pe[i].m_revents |= kPOLLNVAL;
		}
	}

	// done with translated query
	delete[] pfd;

	return n;
}

#else

int
CArchNetworkBSD::pollSocket(CPollEntry pe[], int num, double timeout)
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

			int fdi = pe[i].m_socket->m_fd;
			if (pe[i].m_events & kPOLLIN) {
				FD_SET(pe[i].m_socket->m_fd, &readSet);
				readSetP = &readSet;
				if (fdi > n) {
					n = fdi;
				}
			}
			if (pe[i].m_events & kPOLLOUT) {
				FD_SET(pe[i].m_socket->m_fd, &writeSet);
				writeSetP = &writeSet;
				if (fdi > n) {
					n = fdi;
				}
			}
			if (true) {
				FD_SET(pe[i].m_socket->m_fd, &errSet);
				errSetP = &errSet;
				if (fdi > n) {
					n = fdi;
				}
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
		n = select((SELECT_TYPE_ARG1)  n + 1,
					SELECT_TYPE_ARG234 readSetP,
					SELECT_TYPE_ARG234 writeSetP,
					SELECT_TYPE_ARG234 errSetP,
					SELECT_TYPE_ARG5   timeout2P);

		// handle results
		if (n == -1) {
			if (errno == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			throwError(errno);
		}
		n = 0;
		for (i = 0; i < num; ++i) {
			if (pe[i].m_socket != NULL) {
				if (FD_ISSET(pe[i].m_socket->m_fd, &readSet)) {
					pe[i].m_revents |= kPOLLIN;
				}
				if (FD_ISSET(pe[i].m_socket->m_fd, &writeSet)) {
					pe[i].m_revents |= kPOLLOUT;
				}
				if (FD_ISSET(pe[i].m_socket->m_fd, &errSet)) {
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

#endif

size_t
CArchNetworkBSD::readSocket(CArchSocket s, void* buf, size_t len)
{
	assert(s != NULL);

	ssize_t n;
	do {
		n = read(s->m_fd, buf, len);
		if (n == -1) {
			if (errno == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			throwError(errno);
		}
	} while (false);
	ARCH->testCancelThread();
	return n;
}

size_t
CArchNetworkBSD::writeSocket(CArchSocket s, const void* buf, size_t len)
{
	assert(s != NULL);

	ssize_t n;
	do {
		n = write(s->m_fd, buf, len);
		if (n == -1) {
			if (errno == EINTR) {
				// interrupted system call
				ARCH->testCancelThread();
				continue;
			}
			throwError(errno);
		}
	} while (false);
	ARCH->testCancelThread();
	return n;
}

void
CArchNetworkBSD::throwErrorOnSocket(CArchSocket s)
{
	assert(s != NULL);

	// get the error from the socket layer
	int err        = 0;
	socklen_t size = sizeof(err);
	if (getsockopt(s->m_fd, SOL_SOCKET, SO_ERROR, &err, &size) == -1) {
		err = errno;
	}

	// throw if there's an error
	if (err != 0) {
		throwError(err);
	}
}

bool
CArchNetworkBSD::setBlockingOnSocket(CArchSocket s, bool blocking)
{
	assert(s != NULL);

	int mode = fcntl(s->m_fd, F_GETFL, 0);
	if (mode == -1) {
		throwError(errno);
	}
	bool old = ((mode & O_NDELAY) == 0);
	if (blocking) {
		mode &= ~O_NDELAY;
	}
	else {
		mode |= O_NDELAY;
	}
	if (fcntl(s->m_fd, F_SETFL, mode) == -1) {
		throwError(errno);
	}
	return old;
}

bool
CArchNetworkBSD::setNoDelayOnSocket(CArchSocket s, bool noDelay)
{
	assert(s != NULL);

	// get old state
	int oflag;
	socklen_t size = sizeof(oflag);
	if (getsockopt(s->m_fd, IPPROTO_TCP, TCP_NODELAY, &oflag, &size) == -1) {
		throwError(errno);
	}

	int flag = noDelay ? 1 : 0;
	size     = sizeof(flag);
	if (setsockopt(s->m_fd, IPPROTO_TCP, TCP_NODELAY, &flag, size) == -1) {
		throwError(errno);
	}

	return (oflag != 0);
}

std::string
CArchNetworkBSD::getHostName()
{
	char name[256];
	if (gethostname(name, sizeof(name)) == -1) {
		name[0] = '\0';
	}
	else {
		name[sizeof(name) - 1] = '\0';
	}
	return name;
}

CArchNetAddress
CArchNetworkBSD::newAnyAddr(EAddressFamily family)
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
CArchNetworkBSD::copyAddr(CArchNetAddress addr)
{
	assert(addr != NULL);

	// allocate and copy address
	return new CArchNetAddressImpl(*addr);
}

CArchNetAddress
CArchNetworkBSD::nameToAddr(const std::string& name)
{
	// allocate address
	CArchNetAddressImpl* addr = new CArchNetAddressImpl;

	// try to convert assuming an IPv4 dot notation address
	struct sockaddr_in inaddr;
	memset(&inaddr, 0, sizeof(inaddr));
	if (inet_aton(name.c_str(), &inaddr.sin_addr) != 0) {
		// it's a dot notation address
		addr->m_len       = sizeof(struct sockaddr_in);
		inaddr.sin_family = AF_INET;
		inaddr.sin_port   = 0;
		memcpy(&addr->m_addr, &inaddr, addr->m_len);
	}

	else {
		// mutexed address lookup (ugh)
		ARCH->lockMutex(m_mutex);
		struct hostent* info = gethostbyname(name.c_str());
		if (info == NULL) {
			ARCH->unlockMutex(m_mutex);
			delete addr;
			throwNameError(h_errno);
		}

		// copy over address (only IPv4 currently supported)
		addr->m_len       = sizeof(struct sockaddr_in);
		inaddr.sin_family = info->h_addrtype;
		inaddr.sin_port   = 0;
		memcpy(&inaddr.sin_addr, info->h_addr_list[0], info->h_length);
		memcpy(&addr->m_addr, &inaddr, addr->m_len);

		// done with static buffer
		ARCH->unlockMutex(m_mutex);
	}

	return addr;
}

void
CArchNetworkBSD::closeAddr(CArchNetAddress addr)
{
	assert(addr != NULL);

	delete addr;
}

std::string
CArchNetworkBSD::addrToName(CArchNetAddress addr)
{
	assert(addr != NULL);

	// mutexed name lookup (ugh)
	ARCH->lockMutex(m_mutex);
	struct hostent* info = gethostbyaddr(
							reinterpret_cast<const char*>(&addr->m_addr),
							addr->m_len, addr->m_addr.sa_family);
	if (info == NULL) {
		ARCH->unlockMutex(m_mutex);
		throwNameError(h_errno);
	}

	// save (primary) name
	std::string name = info->h_name;

	// done with static buffer
	ARCH->unlockMutex(m_mutex);

	return name;
}

std::string
CArchNetworkBSD::addrToString(CArchNetAddress addr)
{
	assert(addr != NULL);

	switch (getAddrFamily(addr)) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		ARCH->lockMutex(m_mutex);
		std::string s = inet_ntoa(ipAddr->sin_addr);
		ARCH->unlockMutex(m_mutex);
		return s;
	}

	default:
		assert(0 && "unknown address family");
		return "";
	}
}

IArchNetwork::EAddressFamily
CArchNetworkBSD::getAddrFamily(CArchNetAddress addr)
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
CArchNetworkBSD::setAddrPort(CArchNetAddress addr, int port)
{
	assert(addr != NULL);

	switch (getAddrFamily(addr)) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		ipAddr->sin_port = htons(port);
		break;
	}

	default:
		assert(0 && "unknown address family");
		break;
	}
}

int
CArchNetworkBSD::getAddrPort(CArchNetAddress addr)
{
	assert(addr != NULL);

	switch (getAddrFamily(addr)) {
	case kINET: {
		struct sockaddr_in* ipAddr =
			reinterpret_cast<struct sockaddr_in*>(&addr->m_addr);
		return ntohs(ipAddr->sin_port);
	}

	default:
		assert(0 && "unknown address family");
		return 0;
	}
}

bool
CArchNetworkBSD::isAnyAddr(CArchNetAddress addr)
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
CArchNetworkBSD::throwError(int err)
{
	switch (err) {
	case EAGAIN:
		throw XArchNetworkWouldBlock(new XArchEvalUnix(err));

	case EACCES:
	case EPERM:
		throw XArchNetworkAccess(new XArchEvalUnix(err));

	case ENFILE:
	case EMFILE:
	case ENODEV:
	case ENOBUFS:
	case ENOMEM:
	case ENETDOWN:
#if defined(ENOSR)
	case ENOSR:
#endif
		throw XArchNetworkResource(new XArchEvalUnix(err));

	case EPROTOTYPE:
	case EPROTONOSUPPORT:
	case EAFNOSUPPORT:
	case EPFNOSUPPORT:
	case ESOCKTNOSUPPORT:
	case EINVAL:
	case ENOPROTOOPT:
	case EOPNOTSUPP:
	case ESHUTDOWN:
#if defined(ENOPKG)
	case ENOPKG:
#endif
		throw XArchNetworkSupport(new XArchEvalUnix(err));

	case EIO:
		throw XArchNetworkIO(new XArchEvalUnix(err));

	case EADDRNOTAVAIL:
		throw XArchNetworkNoAddress(new XArchEvalUnix(err));

	case EADDRINUSE:
		throw XArchNetworkAddressInUse(new XArchEvalUnix(err));

	case EHOSTUNREACH:
	case ENETUNREACH:
		throw XArchNetworkNoRoute(new XArchEvalUnix(err));

	case ENOTCONN:
		throw XArchNetworkNotConnected(new XArchEvalUnix(err));

	case EPIPE:
	case ECONNABORTED:
	case ECONNRESET:
		throw XArchNetworkDisconnected(new XArchEvalUnix(err));

	case ECONNREFUSED:
		throw XArchNetworkConnectionRefused(new XArchEvalUnix(err));

	case EINPROGRESS:
	case EALREADY:
		throw XArchNetworkConnecting(new XArchEvalUnix(err));

	case EHOSTDOWN:
	case ETIMEDOUT:
		throw XArchNetworkTimedOut(new XArchEvalUnix(err));

	default:
		throw XArchNetwork(new XArchEvalUnix(err));
	}
}

void
CArchNetworkBSD::throwNameError(int err)
{
	static const char* s_msg[] = {
		"The specified host is unknown",
		"The requested name is valid but does not have an IP address",
		"A non-recoverable name server error occurred",
		"A temporary error occurred on an authoritative name server",
		"An unknown name server error occurred"
	};

	switch (err) {
	case HOST_NOT_FOUND:
		throw XArchNetworkNameUnknown(s_msg[0]);

	case NO_DATA:
		throw XArchNetworkNameNoAddress(s_msg[1]);

	case NO_RECOVERY:
		throw XArchNetworkNameFailure(s_msg[2]);

	case TRY_AGAIN:
		throw XArchNetworkNameUnavailable(s_msg[3]);

	default:
		throw XArchNetworkName(s_msg[4]);
	}
}
