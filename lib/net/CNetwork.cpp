/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CNetwork.h"
#include "XNetwork.h"
#include "CLog.h"
#include <algorithm>

//
// CNetwork
//

CNetwork::Socket (PASCAL FAR *CNetwork::accept)(CNetwork::Socket s, CNetwork::Address FAR *addr, CNetwork::AddressLength FAR *addrlen);
int (PASCAL FAR *CNetwork::bind)(CNetwork::Socket s, const CNetwork::Address FAR *addr, CNetwork::AddressLength namelen);
int (PASCAL FAR *CNetwork::close)(CNetwork::Socket s);
int (PASCAL FAR *CNetwork::connect)(CNetwork::Socket s, const CNetwork::Address FAR *name, CNetwork::AddressLength namelen);
int (PASCAL FAR *CNetwork::ioctl)(CNetwork::Socket s, int cmd, void FAR *);
int (PASCAL FAR *CNetwork::getpeername)(CNetwork::Socket s, CNetwork::Address FAR *name, CNetwork::AddressLength FAR * namelen);
int (PASCAL FAR *CNetwork::getsockname)(CNetwork::Socket s, CNetwork::Address FAR *name, CNetwork::AddressLength FAR * namelen);
int (PASCAL FAR *CNetwork::getsockopt)(CNetwork::Socket s, int level, int optname, void FAR * optval, CNetwork::AddressLength FAR *optlen);
int (PASCAL FAR *CNetwork::listen)(CNetwork::Socket s, int backlog);
ssize_t (PASCAL FAR *CNetwork::read)(CNetwork::Socket s, void FAR * buf, size_t len);
ssize_t (PASCAL FAR *CNetwork::recv)(CNetwork::Socket s, void FAR * buf, size_t len, int flags);
ssize_t (PASCAL FAR *CNetwork::recvfrom)(CNetwork::Socket s, void FAR * buf, size_t len, int flags, CNetwork::Address FAR *from, CNetwork::AddressLength FAR * fromlen);
int (PASCAL FAR *CNetwork::poll)(CNetwork::PollEntry fds[], int nfds, int timeout);
ssize_t (PASCAL FAR *CNetwork::send)(CNetwork::Socket s, const void FAR * buf, size_t len, int flags);
ssize_t (PASCAL FAR *CNetwork::sendto)(CNetwork::Socket s, const void FAR * buf, size_t len, int flags, const CNetwork::Address FAR *to, CNetwork::AddressLength tolen);
int (PASCAL FAR *CNetwork::setsockopt)(CNetwork::Socket s, int level, int optname, const void FAR * optval, CNetwork::AddressLength optlen);
int (PASCAL FAR *CNetwork::shutdown)(CNetwork::Socket s, int how);
CNetwork::Socket (PASCAL FAR *CNetwork::socket)(int af, int type, int protocol);
ssize_t (PASCAL FAR *CNetwork::write)(CNetwork::Socket s, const void FAR * buf, size_t len);
int (PASCAL FAR *CNetwork::gethostname)(char FAR * name, int namelen);
int (PASCAL FAR *CNetwork::getsockerror)(void);

#if WINDOWS_LIKE

int (PASCAL FAR *CNetwork::select)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout);
int (PASCAL FAR *CNetwork::WSACleanup)(void);
int (PASCAL FAR *CNetwork::__WSAFDIsSet)(CNetwork::Socket, fd_set FAR *);
struct hostent FAR * (PASCAL FAR *CNetwork::gethostbyaddr_n)(const char FAR * addr, int len, int type);
struct hostent FAR * (PASCAL FAR *CNetwork::gethostbyname_n)(const char FAR * name);
struct servent FAR * (PASCAL FAR *CNetwork::getservbyport_n)(int port, const char FAR * proto);
struct servent FAR * (PASCAL FAR *CNetwork::getservbyname_n)(const char FAR * name, const char FAR * proto);
struct protoent FAR * (PASCAL FAR *CNetwork::getprotobynumber_n)(int proto);
struct protoent FAR * (PASCAL FAR *CNetwork::getprotobyname_n)(const char FAR * name);
const int				CNetwork::Error = SOCKET_ERROR;
const CNetwork::Socket	CNetwork::Null = INVALID_SOCKET;

#undef FD_ISSET
#define FD_ISSET(fd, set) CNetwork::__WSAFDIsSet((SOCKET)(fd), (fd_set FAR *)(set))

static HMODULE			s_networkModule = NULL;

static
FARPROC
netGetProcAddress(HMODULE module, LPCSTR name)
{
	FARPROC func = ::GetProcAddress(module, name);
	if (!func) {
		throw XNetworkFunctionUnavailable(name);
	}
	return func;
}

void
CNetwork::init()
{
	assert(WSACleanup      == NULL);
	assert(s_networkModule == NULL);

	// try winsock 2
	HMODULE module = (HMODULE)::LoadLibrary("ws2_32.dll");
	if (module == NULL) {
		LOG((CLOG_NOTE "ws2_32.dll not found"));
	}
	else {
		try {
			init2(module);
			return;
		}
		catch (XNetwork& e) {
			LOG((CLOG_NOTE "ws2_32.dll error: %s", e.what()));
		}
	}

	// try winsock 1
	module = (HMODULE)::LoadLibrary("wsock32.dll");
	if (module == NULL) {
		LOG((CLOG_NOTE "wsock32.dll not found"));
	}
	else {
		try {
			init2(module);
			return;
		}
		catch (XNetwork& e) {
			LOG((CLOG_NOTE "wsock32.dll error: %s", e.what()));
		}
	}

	// no networking
	throw XNetworkUnavailable();
}

void
CNetwork::cleanup()
{
	if (s_networkModule != NULL) {
		WSACleanup();
		::FreeLibrary(s_networkModule);

		WSACleanup      = NULL;
		s_networkModule = NULL;
	}
}

UInt32
CNetwork::swaphtonl(UInt32 v)
{
	static const union { UInt16 s; UInt8 b[2]; } s_endian = { 0x1234 };
	if (s_endian.b[0] == 0x34) {
		return	((v & 0xff000000lu) >> 24) |
				((v & 0x00ff0000lu) >>  8) |
				((v & 0x0000ff00lu) <<  8) |
				((v & 0x000000fflu) << 24);
	}
	else {
		return v;
	}
}

UInt16
CNetwork::swaphtons(UInt16 v)
{
	static const union { UInt16 s; UInt8 b[2]; } s_endian = { 0x1234 };
	if (s_endian.b[0] == 0x34) {
		return static_cast<UInt16>(	((v & 0xff00u) >> 8) |
									((v & 0x00ffu) << 8));
	}
	else {
		return v;
	}
}

UInt32
CNetwork::swapntohl(UInt32 v)
{
	return swaphtonl(v);
}

UInt16
CNetwork::swapntohs(UInt16 v)
{
	return swaphtons(v);
}

#define setfunc(var, name, type) 	var = (type)netGetProcAddress(module, #name)

void
CNetwork::init2(
	HMODULE module)
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
		throw XNetworkVersion(LOBYTE(data.wVersion), HIBYTE(data.wVersion));
	}
	if (err != 0) {
		throw XNetworkFailed();
	}

	// get function addresses
	setfunc(accept, accept, Socket (PASCAL FAR *)(Socket s, Address FAR *addr, AddressLength FAR *addrlen));
	setfunc(bind, bind, int (PASCAL FAR *)(Socket s, const Address FAR *addr, AddressLength namelen));
	setfunc(close, closesocket, int (PASCAL FAR *)(Socket s));
	setfunc(connect, connect, int (PASCAL FAR *)(Socket s, const Address FAR *name, AddressLength namelen));
	setfunc(ioctl, ioctlsocket, int (PASCAL FAR *)(Socket s, int cmd, void FAR *));
	setfunc(getpeername, getpeername, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockname, getsockname, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockopt, getsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, void FAR * optval, AddressLength FAR *optlen));
	setfunc(inet_addr_n, inet_addr, unsigned long (PASCAL FAR *)(const char FAR * cp));
	setfunc(inet_ntoa_n, inet_ntoa, char FAR * (PASCAL FAR *)(struct in_addr in));
	setfunc(listen, listen, int (PASCAL FAR *)(Socket s, int backlog));
	setfunc(recv, recv, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags));
	setfunc(recvfrom, recvfrom, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags, Address FAR *from, AddressLength FAR * fromlen));
	setfunc(send, send, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags));
	setfunc(sendto, sendto, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags, const Address FAR *to, AddressLength tolen));
	setfunc(setsockopt, setsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, const void FAR * optval, AddressLength optlen));
	setfunc(shutdown, shutdown, int (PASCAL FAR *)(Socket s, int how));
	setfunc(socket, socket, Socket (PASCAL FAR *)(int af, int type, int protocol));
	setfunc(gethostname, gethostname, int (PASCAL FAR *)(char FAR * name, int namelen));
	setfunc(gethostbyaddr_n, gethostbyaddr, struct hostent FAR * (PASCAL FAR *)(const char FAR * addr, int len, int type));
	setfunc(gethostbyname_n, gethostbyname, struct hostent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(getservbyport_n, getservbyport, struct servent FAR * (PASCAL FAR *)(int port, const char FAR * proto));
	setfunc(getservbyname_n, getservbyname, struct servent FAR * (PASCAL FAR *)(const char FAR * name, const char FAR * proto));
	setfunc(getprotobynumber_n, getprotobynumber, struct protoent FAR * (PASCAL FAR *)(int proto));
	setfunc(getprotobyname_n, getprotobyname, struct protoent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(getsockerror, WSAGetLastError, int (PASCAL FAR *)(void));
	setfunc(WSACleanup, WSACleanup, int (PASCAL FAR *)(void));
	setfunc(__WSAFDIsSet, __WSAFDIsSet, int (PASCAL FAR *)(CNetwork::Socket, fd_set FAR *));
	setfunc(select, select, int (PASCAL FAR *)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout));
	poll  = poll2;
	read  = read2;
	write = write2;

	s_networkModule = module;
}

ssize_t PASCAL FAR
CNetwork::read2(Socket s, void FAR* buf, size_t len)
{
	return recv(s, buf, len, 0);
}

ssize_t PASCAL FAR
CNetwork::write2(Socket s, const void FAR* buf, size_t len)
{
	return send(s, buf, len, 0);
}

int PASCAL FAR
CNetwork::inet_aton(const char FAR * cp, InternetAddress FAR * addr)
{
	assert(addr != NULL);

	// fake it with inet_addr
	unsigned long inetAddr = inet_addr_n(cp);
	if (inetAddr == INADDR_NONE) {
		return 0;
	}
	else {
		*(addr->s_addr) = inetAddr;
		return 1;
	}
}

CString PASCAL FAR
CNetwork::inet_ntoa(struct in_addr in)
{
	// winsock returns strings per-thread
	return CString(inet_ntoa_n(in));
}

int PASCAL FAR
CNetwork::gethostbyaddr(CHostInfo* hostinfo,
						const char FAR * addr, int len, int type)
{
	assert(hostinfo != NULL);

	// winsock returns structures per-thread
	struct hostent FAR* info = gethostbyaddr_n(addr, len, type);
	if (info == NULL) {
		return WSAGetLastError();
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::gethostbyname(CHostInfo* hostinfo,
						const char FAR * name)
{
	assert(hostinfo != NULL);

	// winsock returns structures per-thread
	struct hostent FAR* info = gethostbyname_n(name);
	if (info == NULL) {
		return WSAGetLastError();
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getservbyport(CServiceInfo* servinfo,
						int port, const char FAR * proto)
{
	assert(servinfo != NULL);

	// winsock returns structures per-thread
	struct servent FAR* info = getservbyport_n(port, proto);
	if (info == NULL) {
		return WSAGetLastError();
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getservbyname(CServiceInfo* servinfo,
						const char FAR * name, const char FAR * proto)
{
	assert(servinfo != NULL);

	// winsock returns structures per-thread
	struct servent FAR* info = getservbyname_n(name, proto);
	if (info == NULL) {
		return WSAGetLastError();
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getprotobynumber(CProtocolInfo* protoinfo,
						int proto)
{
	assert(protoinfo != NULL);

	// winsock returns structures per-thread
	struct protoinfo FAR* info = getprotobynumber_n(proto);
	if (info == NULL) {
		return WSAGetLastError();
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getprotobyname(CProtocolInfo* protoinfo,
						const char FAR * name)
{
	assert(protoinfo != NULL);

	// winsock returns structures per-thread
	struct protoinfo FAR* info = getprotobyname_n(name);
	if (info == NULL) {
		return WSAGetLastError();
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::setblocking(CNetwork::Socket s, bool blocking)
{
	int flag = blocking ? 0 : 1;
	return ioctl(s, FIONBIO, &flag);
}

int PASCAL FAR
CNetwork::setnodelay(CNetwork::Socket s, bool nodelay)
{
	BOOL flag = nodelay ? 1 : 0;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

#endif

#if UNIX_LIKE

#include "CMutex.h"
#include "CLock.h"

#if HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#	include <sys/stat.h>
#endif
#if HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if !defined(TCP_NODELAY)
#	include <netinet/tcp.h>
#endif

// FIXME -- use reentrant versions of non-reentrant functions

const int				CNetwork::Error = -1;
const CNetwork::Socket	CNetwork::Null = -1;

static CMutex*			s_networkMutex = NULL;

#define setfunc(var, name, type) 	var = (type)::name

UInt32
CNetwork::swaphtonl(UInt32 v)
{
	return htonl(v);
}

UInt16
CNetwork::swaphtons(UInt16 v)
{
	return htons(v);
}

UInt32
CNetwork::swapntohl(UInt32 v)
{
	return ntohl(v);
}

UInt16
CNetwork::swapntohs(UInt16 v)
{
	return ntohs(v);
}

void
CNetwork::init()
{
	assert(s_networkMutex == NULL);

	try {
		s_networkMutex = new CMutex;
	}
	catch (...) {
		throw XNetworkFailed();
	}

	setfunc(accept, accept, Socket (PASCAL FAR *)(Socket s, Address FAR *addr, AddressLength FAR *addrlen));
	setfunc(bind, bind, int (PASCAL FAR *)(Socket s, const Address FAR *addr, AddressLength namelen));
	setfunc(close, close, int (PASCAL FAR *)(Socket s));
	setfunc(connect, connect, int (PASCAL FAR *)(Socket s, const Address FAR *name, AddressLength namelen));
	setfunc(ioctl, ioctl, int (PASCAL FAR *)(Socket s, int cmd, void FAR *));
	setfunc(getpeername, getpeername, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockname, getsockname, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockopt, getsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, void FAR * optval, AddressLength FAR *optlen));
	setfunc(listen, listen, int (PASCAL FAR *)(Socket s, int backlog));
#if HAVE_POLL
	setfunc(poll, poll, int (PASCAL FAR *)(CNetwork::PollEntry fds[], int nfds, int timeout));
#else
	setfunc(poll, CNetwork::poll2, int (PASCAL FAR *)(CNetwork::PollEntry fds[], int nfds, int timeout));
#endif
	setfunc(read, read, ssize_t (PASCAL FAR *)(CNetwork::Socket s, void FAR * buf, size_t len));
	setfunc(recv, recv, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags));
	setfunc(recvfrom, recvfrom, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags, Address FAR *from, AddressLength FAR * fromlen));
	setfunc(send, send, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags));
	setfunc(sendto, sendto, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags, const Address FAR *to, AddressLength tolen));
	setfunc(setsockopt, setsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, const void FAR * optval, AddressLength optlen));
	setfunc(shutdown, shutdown, int (PASCAL FAR *)(Socket s, int how));
	setfunc(socket, socket, Socket (PASCAL FAR *)(int af, int type, int protocol));
	setfunc(write, write, ssize_t (PASCAL FAR *)(CNetwork::Socket s, const void FAR * buf, size_t len));
	gethostname  = gethostname2;
	getsockerror = getsockerror2;
}

void
CNetwork::cleanup()
{
	delete s_networkMutex;
	s_networkMutex = NULL;
}

int PASCAL FAR
CNetwork::gethostname2(char* name, int namelen)
{
	return ::gethostname(name, namelen);
}

int PASCAL FAR
CNetwork::getsockerror2(void)
{
	return errno;
}

int PASCAL FAR
CNetwork::inet_aton(const char FAR * cp, InternetAddress FAR * addr)
{
	return ::inet_aton(cp, addr);
}

CString PASCAL FAR
CNetwork::inet_ntoa(struct in_addr in)
{
	// single threaded access to inet_ntoa functions
	CLock lock(s_networkMutex);
	return CString(::inet_ntoa(in));
}

int PASCAL FAR
CNetwork::gethostbyaddr(CHostInfo* hostinfo,
						const char FAR * addr, int len, int type)
{
	assert(hostinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct hostent FAR* info = ::gethostbyaddr(addr, len, type);
	if (info == NULL) {
		return h_errno;
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::gethostbyname(CHostInfo* hostinfo,
						const char FAR * name)
{
	assert(hostinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct hostent FAR* info = ::gethostbyname(name);
	if (info == NULL) {
		return h_errno;
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getservbyport(CServiceInfo* servinfo,
						int port, const char FAR * proto)
{
	assert(servinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct servent FAR* info = ::getservbyport(port, proto);
	if (info == NULL) {
		return -1;
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getservbyname(CServiceInfo* servinfo,
						const char FAR * name, const char FAR * proto)
{
	assert(servinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct servent FAR* info = ::getservbyname(name, proto);
	if (info == NULL) {
		return -1;
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getprotobynumber(CProtocolInfo* protoinfo,
						int proto)
{
	assert(protoinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct protoent FAR* info = ::getprotobynumber(proto);
	if (info == NULL) {
		return -1;
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::getprotobyname(CProtocolInfo* protoinfo,
						const char FAR * name)
{
	assert(protoinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct protoent FAR* info = ::getprotobyname(name);
	if (info == NULL) {
		return -1;
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int PASCAL FAR
CNetwork::setblocking(CNetwork::Socket s, bool blocking)
{
	int mode = fcntl(s, F_GETFL, 0);
	if (mode == -1) {
		return -1;
	}
	if (blocking) {
		mode &= ~O_NDELAY;
	}
	else {
		mode |= O_NDELAY;
	}
	if (fcntl(s, F_SETFL, mode) < 0) {
		return -1;
	}
	return 0;
}

int PASCAL FAR
CNetwork::setnodelay(CNetwork::Socket s, bool nodelay)
{
	int flag = nodelay ? 1 : 0;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

#endif

#if WINDOWS_LIKE || !HAVE_POLL

#if HAVE_SYS_SELECT_H
#	include <sys/select.h>
#endif
#if HAVE_SYS_TIME_H
#	include <sys/time.h>
#endif
#if HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#	include <unistd.h>
#endif

int PASCAL FAR
CNetwork::poll2(PollEntry fd[], int nfds, int timeout)
{
	int i;

	// prepare sets for select
	int n = 0;
	fd_set readSet, writeSet, errSet;
	fd_set* readSetP  = NULL;
	fd_set* writeSetP = NULL;
	fd_set* errSetP   = NULL;
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_ZERO(&errSet);
	for (i = 0; i < nfds; ++i) {
		if (fd[i].events & kPOLLIN) {
			FD_SET(fd[i].fd, &readSet);
			readSetP = &readSet;
			if (fd[i].fd > n) {
				n = fd[i].fd;
			}
		}
		if (fd[i].events & kPOLLOUT) {
			FD_SET(fd[i].fd, &writeSet);
			writeSetP = &writeSet;
			if (fd[i].fd > n) {
				n = fd[i].fd;
			}
		}
		if (true) {
			FD_SET(fd[i].fd, &errSet);
			errSetP = &errSet;
			if (fd[i].fd > n) {
				n = fd[i].fd;
			}
		}
	}

	// prepare timeout for select
	struct timeval timeout2;
	struct timeval* timeout2P;
	if (timeout < 0) {
		timeout2P = NULL;
	}
	else {
		timeout2P = &timeout2;
		timeout2.tv_sec  = timeout / 1000;
		timeout2.tv_usec = 1000 * (timeout % 1000);
	}

	// do the select.  note that winsock ignores the first argument.
	n = select((SELECT_TYPE_ARG1)  n + 1,
				SELECT_TYPE_ARG234 readSetP,
				SELECT_TYPE_ARG234 writeSetP,
				SELECT_TYPE_ARG234 errSetP,
				SELECT_TYPE_ARG5   timeout2P);

	// handle results
	if (n == Error) {
		return Error;
	}
	if (n == 0) {
		return 0;
	}
	n = 0;
	for (i = 0; i < nfds; ++i) {
		fd[i].revents = 0;
		if (FD_ISSET(fd[i].fd, &readSet)) {
			fd[i].revents |= kPOLLIN;
		}
		if (FD_ISSET(fd[i].fd, &writeSet)) {
			fd[i].revents |= kPOLLOUT;
		}
		if (FD_ISSET(fd[i].fd, &errSet)) {
			fd[i].revents |= kPOLLERR;
		}
		if (fd[i].revents != 0) {
			++n;
		}
	}
	return n;
}

#endif


//
// CNetwork::CHostInfo
//

CNetwork::CHostInfo::CHostInfo(const struct hostent* hent)
{
	assert(hent != NULL);

	m_name          = hent->h_name;
	m_addressType   = hent->h_addrtype;
	m_addressLength = hent->h_length;
	for (char** scan = hent->h_aliases; *scan != NULL; ++scan) {
		m_aliases.push_back(*scan);
	}

	// concatenate addresses together
	UInt32 n = 0;
	for (char** scan = hent->h_addr_list; *scan != NULL; ++scan) {
		++n;
	}
	m_addressData.reserve(n);
	for (char** scan = hent->h_addr_list; *scan != NULL; ++scan) {
		m_addressData.append(*scan, m_addressLength);
	}

	// set pointers into concatenated data
	const char* data = m_addressData.data();
	for (char** scan = hent->h_addr_list; *scan != NULL; ++scan) {
		m_addresses.push_back(data);
		data += m_addressLength;
	}
}

void
CNetwork::CHostInfo::swap(CHostInfo& v)
{
	std::swap(m_name,          v.m_name);
	std::swap(m_aliases,       v.m_aliases);
	std::swap(m_addressType,   v.m_addressType);
	std::swap(m_addressLength, v.m_addressLength);
	std::swap(m_addresses,     v.m_addresses);
	std::swap(m_addressData,   v.m_addressData);
}


//
// CNetwork::CServiceInfo
//

CNetwork::CServiceInfo::CServiceInfo(const struct servent* sent)
{
	assert(sent != NULL);

	m_name     = sent->s_name;
	m_port     = sent->s_port;
	m_protocol = sent->s_proto;
	for (char** scan = sent->s_aliases; *scan != NULL; ++scan) {
		m_aliases.push_back(*scan);
	}
}

void
CNetwork::CServiceInfo::swap(CServiceInfo& v)
{
	std::swap(m_name,     v.m_name);
	std::swap(m_aliases,  v.m_aliases);
	std::swap(m_port,     v.m_port);
	std::swap(m_protocol, v.m_protocol);
}


//
// CNetwork::CProtocolInfo
//

CNetwork::CProtocolInfo::CProtocolInfo(const struct protoent* pent)
{
	assert(pent != NULL);

	m_name     = pent->p_name;
	m_protocol = pent->p_proto;
	for (char** scan = pent->p_aliases; *scan != NULL; ++scan) {
		m_aliases.push_back(*scan);
	}
}

void
CNetwork::CProtocolInfo::swap(CProtocolInfo& v)
{
	std::swap(m_name,     v.m_name);
	std::swap(m_aliases,  v.m_aliases);
	std::swap(m_protocol, v.m_protocol);
}
