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

#if WINDOWS_LIKE

static CNetwork::Socket (PASCAL FAR *accept_winsock)(CNetwork::Socket s, CNetwork::Address FAR *addr, CNetwork::AddressLength FAR *addrlen);
static int (PASCAL FAR *bind_winsock)(CNetwork::Socket s, const CNetwork::Address FAR *addr, CNetwork::AddressLength namelen);
static int (PASCAL FAR *close_winsock)(CNetwork::Socket s);
static int (PASCAL FAR *connect_winsock)(CNetwork::Socket s, const CNetwork::Address FAR *name, CNetwork::AddressLength namelen);
static int (PASCAL FAR *gethostname_winsock)(char FAR * name, int namelen);
static int (PASCAL FAR *getpeername_winsock)(CNetwork::Socket s, CNetwork::Address FAR *name, CNetwork::AddressLength FAR * namelen);
static int (PASCAL FAR *getsockerror_winsock)(void);
static int (PASCAL FAR *getsockname_winsock)(CNetwork::Socket s, CNetwork::Address FAR *name, CNetwork::AddressLength FAR * namelen);
static int (PASCAL FAR *getsockopt_winsock)(CNetwork::Socket s, int level, int optname, void FAR * optval, CNetwork::AddressLength FAR *optlen);
static char FAR * (PASCAL FAR *inet_ntoa_winsock)(struct in_addr in);
static unsigned long (PASCAL FAR *inet_addr_winsock)(const char FAR * cp);
static int (PASCAL FAR *ioctl_winsock)(CNetwork::Socket s, int cmd, void FAR *);
static int (PASCAL FAR *listen_winsock)(CNetwork::Socket s, int backlog);
static ssize_t (PASCAL FAR *recv_winsock)(CNetwork::Socket s, void FAR * buf, size_t len, int flags);
static ssize_t (PASCAL FAR *recvfrom_winsock)(CNetwork::Socket s, void FAR * buf, size_t len, int flags, CNetwork::Address FAR *from, CNetwork::AddressLength FAR * fromlen);
static int (PASCAL FAR *select_winsock)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout);
static ssize_t (PASCAL FAR *send_winsock)(CNetwork::Socket s, const void FAR * buf, size_t len, int flags);
static ssize_t (PASCAL FAR *sendto_winsock)(CNetwork::Socket s, const void FAR * buf, size_t len, int flags, const CNetwork::Address FAR *to, CNetwork::AddressLength tolen);
static int (PASCAL FAR *setsockopt_winsock)(CNetwork::Socket s, int level, int optname, const void FAR * optval, CNetwork::AddressLength optlen);
static int (PASCAL FAR *shutdown_winsock)(CNetwork::Socket s, int how);
static CNetwork::Socket (PASCAL FAR *socket_winsock)(int af, int type, int protocol);
static struct hostent FAR * (PASCAL FAR *gethostbyaddr_winsock)(const char FAR * addr, int len, int type);
static struct hostent FAR * (PASCAL FAR *gethostbyname_winsock)(const char FAR * name);
static struct servent FAR * (PASCAL FAR *getservbyport_winsock)(int port, const char FAR * proto);
static struct servent FAR * (PASCAL FAR *getservbyname_winsock)(const char FAR * name, const char FAR * proto);
static struct protoent FAR * (PASCAL FAR *getprotobynumber_winsock)(int proto);
static struct protoent FAR * (PASCAL FAR *getprotobyname_winsock)(const char FAR * name);
static int (PASCAL FAR *WSACleanup_winsock)(void);
static int (PASCAL FAR *WSAFDIsSet_winsock)(CNetwork::Socket, fd_set FAR *);

const int				CNetwork::Error = SOCKET_ERROR;
const CNetwork::Socket	CNetwork::Null  = INVALID_SOCKET;

#undef FD_ISSET
#define FD_ISSET(fd, set) WSAFDIsSet_winsock((SOCKET)(fd), (fd_set FAR *)(set))

// have poll() pick up our select() pointer
#define select select_winsock

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
	assert(WSACleanup_winsock == NULL);
	assert(s_networkModule    == NULL);

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
		WSACleanup_winsock();
		::FreeLibrary(s_networkModule);

		WSACleanup_winsock = NULL;
		s_networkModule    = NULL;
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
	setfunc(accept_winsock, accept, Socket (PASCAL FAR *)(Socket s, Address FAR *addr, AddressLength FAR *addrlen));
	setfunc(bind_winsock, bind, int (PASCAL FAR *)(Socket s, const Address FAR *addr, AddressLength namelen));
	setfunc(close_winsock, closesocket, int (PASCAL FAR *)(Socket s));
	setfunc(connect_winsock, connect, int (PASCAL FAR *)(Socket s, const Address FAR *name, AddressLength namelen));
	setfunc(gethostname_winsock, gethostname, int (PASCAL FAR *)(char FAR * name, int namelen));
	setfunc(getpeername_winsock, getpeername, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockerror_winsock, WSAGetLastError, int (PASCAL FAR *)(void));
	setfunc(getsockname_winsock, getsockname, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockopt_winsock, getsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, void FAR * optval, AddressLength FAR *optlen));
	setfunc(inet_addr_winsock, inet_addr, unsigned long (PASCAL FAR *)(const char FAR * cp));
	setfunc(inet_ntoa_winsock, inet_ntoa, char FAR * (PASCAL FAR *)(struct in_addr in));
	setfunc(ioctl_winsock, ioctlsocket, int (PASCAL FAR *)(Socket s, int cmd, void FAR *));
	setfunc(listen_winsock, listen, int (PASCAL FAR *)(Socket s, int backlog));
	setfunc(recv_winsock, recv, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags));
	setfunc(recvfrom_winsock, recvfrom, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags, Address FAR *from, AddressLength FAR * fromlen));
	setfunc(select_winsock, select, int (PASCAL FAR *)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout));
	setfunc(send_winsock, send, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags));
	setfunc(sendto_winsock, sendto, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags, const Address FAR *to, AddressLength tolen));
	setfunc(setsockopt_winsock, setsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, const void FAR * optval, AddressLength optlen));
	setfunc(shutdown_winsock, shutdown, int (PASCAL FAR *)(Socket s, int how));
	setfunc(socket,_winsock socket, Socket (PASCAL FAR *)(int af, int type, int protocol));
	setfunc(gethostbyaddr_winsock, gethostbyaddr, struct hostent FAR * (PASCAL FAR *)(const char FAR * addr, int len, int type));
	setfunc(gethostbyname_winsock, gethostbyname, struct hostent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(getservbyport_winsock, getservbyport, struct servent FAR * (PASCAL FAR *)(int port, const char FAR * proto));
	setfunc(getservbyname_winsock, getservbyname, struct servent FAR * (PASCAL FAR *)(const char FAR * name, const char FAR * proto));
	setfunc(getprotobynumber_winsock, getprotobynumber, struct protoent FAR * (PASCAL FAR *)(int proto));
	setfunc(getprotobyname_winsock, getprotobyname, struct protoent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(WSACleanup_winsock, WSACleanup, int (PASCAL FAR *)(void));
	setfunc(WSAFDIsSet_winsock, __WSAFDIsSet, int (PASCAL FAR *)(CNetwork::Socket, fd_set FAR *));

	s_networkModule = module;
}

CNetwork::Socket
CNetwork::accept(Socket s, Address* addr, AddressLength* addrlen)
{
	return accept_winsock(s, addr, addrlen);
}

int
CNetwork::bind(Socket s, const Address* addr, AddressLength namelen)
{
	return bind_winsock(s, addr, namelen);
}

int
CNetwork::close(Socket s)
{
	return close_winsock(s);
}

int
CNetwork::connect(Socket s, const Address* name, AddressLength namelen)
{
	return connect_winsock(s);
}

int
CNetwork::gethostname(char* name, int namelen)
{
	return gethostname_winsock(name, namelen);
}

int
CNetwork::getpeername(Socket s, Address* name, AddressLength* namelen)
{
	return getpeername_winsock(s, name, namelen);
}

int
CNetwork::getsockerror(void)
{
	return getsockerrror_winsock();
}

int
CNetwork::getsockname(Socket s, Address* name, AddressLength* namelen)
{
	return getsockname_winsock(s, name, namelen);
}

int
CNetwork::getsockopt(Socket s, int level, int optname, void* optval, AddressLength* optlen)
{
	return getsockopt_winsock(s, level, optname, optval, optlen);
}

int
CNetwork::inet_aton(const char* cp, InternetAddress* addr)
{
	assert(addr != NULL);

	// fake it with inet_addr, which is per-thread on winsock
	unsigned long inetAddr = inet_addr_winsock(cp);
	if (inetAddr == INADDR_NONE) {
		return 0;
	}
	else {
		addr->s_addr = inetAddr;
		return 1;
	}
}

CString
CNetwork::inet_ntoa(struct in_addr in)
{
	// winsock returns strings per-thread
	return CString(inet_ntoa_winsock(in));
}

int
CNetwork::ioctl(Socket s, int cmd, void* arg)
{
	return ioctl_winsock(s, cmd, arg);
}

int
CNetwork::listen(Socket s, int backlog)
{
	return listen_winsock(s, backlog);
}

ssize_t
CNetwork::read(Socket s, void* buf, size_t len)
{
	return recv_winsock(s, buf, len, 0);
}

ssize_t
CNetwork::recv(Socket s, void* buf, size_t len, int flags)
{
	return recv_winsock(s, buf, len, flags);
}

ssize_t
CNetwork::recvfrom(Socket s, void* buf, size_t len, int flags, Address* from, AddressLength* fromlen)
{
	return recvfrom_winsock(s, buf, len, flags, from, fromlen);
}

ssize_t
CNetwork::send(Socket s, const void* buf, size_t len, int flags)
{
	return send_winsock(s, buf, len, flags);
}

ssize_t
CNetwork::sendto(Socket s, const void* buf, size_t len, int flags, const Address* to, AddressLength tolen)
{
	return sendto_winsock(s, buf, len, flags, to, tolen);
}

int
CNetwork::setsockopt(Socket s, int level, int optname, const void* optval, AddressLength optlen)
{
	return setsockopt_winsock(s, level, optname, optval, optlen);
}

int
CNetwork::shutdown(Socket s, int how)
{
	return shutdown_winsock(s, how);
}

CNetwork::Socket
CNetwork::socket(int af, int type, int protocol)
{
	return socket_winsock(af, type, protocol);
}

ssize_t
CNetwork::write(Socket s, const void* buf, size_t len)
{
	return send_winsock(s, buf, len, 0);
}

int
CNetwork::gethostbyaddr(CHostInfo* hostinfo, const char* addr, int len, int type)
{
	assert(hostinfo != NULL);

	// winsock returns structures per-thread
	struct hostent FAR* info = gethostbyaddr_winsock(addr, len, type);
	if (info == NULL) {
		return getsockerror();
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::gethostbyname(CHostInfo* hostinfo, const char* name)
{
	assert(hostinfo != NULL);

	// winsock returns structures per-thread
	struct hostent FAR* info = gethostbyname_winsock(name);
	if (info == NULL) {
		return getsockerror();
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getservbyport(CServiceInfo* servinfo, int port, const char* proto)
{
	assert(servinfo != NULL);

	// winsock returns structures per-thread
	struct servent FAR* info = getservbyport_winsock(port, proto);
	if (info == NULL) {
		return getsockerror();
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getservbyname(CServiceInfo* servinfo, const char* name, const char* proto)
{
	assert(servinfo != NULL);

	// winsock returns structures per-thread
	struct servent FAR* info = getservbyname_winsock(name, proto);
	if (info == NULL) {
		return getsockerror();
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getprotobynumber(CProtocolInfo* protoinfo, int proto)
{
	assert(protoinfo != NULL);

	// winsock returns structures per-thread
	struct protoent FAR* info = getprotobynumber_winsock(proto);
	if (info == NULL) {
		return getsockerror();
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getprotobyname(CProtocolInfo* protoinfo, const char* name)
{
	assert(protoinfo != NULL);

	// winsock returns structures per-thread
	struct protoent FAR* info = getprotobyname_winsock(name);
	if (info == NULL) {
		return getsockerror();
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::setblocking(Socket s, bool blocking)
{
	int flag = blocking ? 0 : 1;
	return ioctl(s, FIONBIO, &flag);
}

int
CNetwork::setnodelay(Socket s, bool nodelay)
{
	BOOL flag = nodelay ? 1 : 0;
	return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

#endif

#if UNIX_LIKE

#include "CMutex.h"
#include "CLock.h"

#if HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#if !defined(TCP_NODELAY)
#	include <netinet/tcp.h>
#endif

const int				CNetwork::Error = -1;
const CNetwork::Socket	CNetwork::Null  = -1;

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
}

void
CNetwork::cleanup()
{
	delete s_networkMutex;
	s_networkMutex = NULL;
}

CNetwork::Socket
CNetwork::accept(Socket s, Address* addr, AddressLength* addrlen)
{
	return ::accept(s, addr, addrlen);
}

int
CNetwork::bind(Socket s, const Address* addr, AddressLength namelen)
{
	return ::bind(s, addr, namelen);
}

int
CNetwork::close(Socket s)
{
	return ::close(s);
}

int
CNetwork::connect(Socket s, const Address* name, AddressLength namelen)
{
	return ::connect(s, name, namelen);
}

int
CNetwork::gethostname(char* name, int namelen)
{
	return ::gethostname(name, namelen);
}

int
CNetwork::getpeername(Socket s, Address* name, AddressLength* namelen)
{
	return ::getpeername(s, name, namelen);
}

int
CNetwork::getsockerror(void)
{
	return errno;
}

int
CNetwork::getsockname(Socket s, Address* name, AddressLength* namelen)
{
	return ::getsockname(s, name, namelen);
}

int
CNetwork::getsockopt(Socket s, int level, int optname, void* optval, AddressLength* optlen)
{
	return ::getsockopt(s, level, optname, optval, optlen);
}

int
CNetwork::inet_aton(const char* cp, InternetAddress* addr)
{
	return ::inet_aton(cp, addr);
}

CString
CNetwork::inet_ntoa(struct in_addr in)
{
	// single threaded access to inet_ntoa functions
	CLock lock(s_networkMutex);
	return CString(::inet_ntoa(in));
}

int
CNetwork::ioctl(Socket s, int cmd, void* arg)
{
	return ::ioctl(s, cmd, arg);
}

int
CNetwork::listen(Socket s, int backlog)
{
	return ::listen(s, backlog);
}

ssize_t
CNetwork::read(Socket s, void* buf, size_t len)
{
	return ::read(s, buf, len);
}

ssize_t
CNetwork::recv(Socket s, void* buf, size_t len, int flags)
{
	return ::recv(s, buf, len, flags);
}

ssize_t
CNetwork::recvfrom(Socket s, void* buf, size_t len, int flags, Address* from, AddressLength* fromlen)
{
	return ::recvfrom(s, buf, len, flags, from, fromlen);
}

ssize_t
CNetwork::send(Socket s, const void* buf, size_t len, int flags)
{
	return ::send(s, buf, len, flags);
}

ssize_t
CNetwork::sendto(Socket s, const void* buf, size_t len, int flags, const Address* to, AddressLength tolen)
{
	return ::sendto(s, buf, len, flags, to, tolen);
}

int
CNetwork::setsockopt(Socket s, int level, int optname, const void* optval, AddressLength optlen)
{
	return ::setsockopt(s, level, optname, optval, optlen);
}

int
CNetwork::shutdown(Socket s, int how)
{
	return ::shutdown(s, how);
}

CNetwork::Socket
CNetwork::socket(int af, int type, int protocol)
{
	return ::socket(af, type, protocol);
}

ssize_t
CNetwork::write(Socket s, const void* buf, size_t len)
{
	return ::write(s, buf, len);
}

int
CNetwork::gethostbyaddr(CHostInfo* hostinfo, const char* addr, int len, int type)
{
	assert(hostinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct hostent* info = ::gethostbyaddr(addr, len, type);
	if (info == NULL) {
		return h_errno;
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::gethostbyname(CHostInfo* hostinfo, const char* name)
{
	assert(hostinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct hostent* info = ::gethostbyname(name);
	if (info == NULL) {
		return h_errno;
	}
	else {
		CHostInfo tmp(info);
		hostinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getservbyport(CServiceInfo* servinfo, int port, const char* proto)
{
	assert(servinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct servent* info = ::getservbyport(port, proto);
	if (info == NULL) {
		return -1;
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getservbyname(CServiceInfo* servinfo, const char* name, const char* proto)
{
	assert(servinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct servent* info = ::getservbyname(name, proto);
	if (info == NULL) {
		return -1;
	}
	else {
		CServiceInfo tmp(info);
		servinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getprotobynumber(CProtocolInfo* protoinfo, int proto)
{
	assert(protoinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct protoent* info = ::getprotobynumber(proto);
	if (info == NULL) {
		return -1;
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int
CNetwork::getprotobyname(CProtocolInfo* protoinfo, const char* name)
{
	assert(protoinfo != NULL);

	// single threaded access to netdb functions
	CLock lock(s_networkMutex);
	struct protoent* info = ::getprotobyname(name);
	if (info == NULL) {
		return -1;
	}
	else {
		CProtocolInfo tmp(info);
		protoinfo->swap(tmp);
		return 0;
	}
}

int
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

int
CNetwork::setnodelay(CNetwork::Socket s, bool nodelay)
{
	int flag = nodelay ? 1 : 0;
	return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

#endif

#if HAVE_POLL

int
CNetwork::poll(PollEntry fd[], int nfds, int timeout)
{
	return ::poll(fd, nfds, timeout);
}

#else // !HAVE_POLL

#if HAVE_SYS_SELECT_H && !WINDOWS_LIKE
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

int
CNetwork::poll(PollEntry fd[], int nfds, int timeout)
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
		int fdi = static_cast<int>(fd[i].fd);
		if (fd[i].events & kPOLLIN) {
			FD_SET(fd[i].fd, &readSet);
			readSetP = &readSet;
			if (fdi > n) {
				n = fdi;
			}
		}
		if (fd[i].events & kPOLLOUT) {
			FD_SET(fd[i].fd, &writeSet);
			writeSetP = &writeSet;
			if (fdi > n) {
				n = fdi;
			}
		}
		if (true) {
			FD_SET(fd[i].fd, &errSet);
			errSetP = &errSet;
			if (fdi > n) {
				n = fdi;
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

#endif // !HAVE_POLL


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
