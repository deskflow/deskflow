#include "CNetwork.h"
#include "XNetwork.h"
#include "CLog.h"
#include <assert.h>

//
// CNetwork
//

CNetwork::Socket (PASCAL FAR *CNetwork::accept)(CNetwork::Socket s, CNetwork::Address FAR *addr, CNetwork::AddressLength FAR *addrlen);
int (PASCAL FAR *CNetwork::bind)(CNetwork::Socket s, const CNetwork::Address FAR *addr, CNetwork::AddressLength namelen);
int (PASCAL FAR *CNetwork::close)(CNetwork::Socket s);
int (PASCAL FAR *CNetwork::connect)(CNetwork::Socket s, const CNetwork::Address FAR *name, CNetwork::AddressLength namelen);
int (PASCAL FAR *CNetwork::ioctl)(CNetwork::Socket s, int cmd, ...);
int (PASCAL FAR *CNetwork::getpeername)(CNetwork::Socket s, CNetwork::Address FAR *name, CNetwork::AddressLength FAR * namelen);
int (PASCAL FAR *CNetwork::getsockname)(CNetwork::Socket s, CNetwork::Address FAR *name, CNetwork::AddressLength FAR * namelen);
int (PASCAL FAR *CNetwork::getsockopt)(CNetwork::Socket s, int level, int optname, void FAR * optval, CNetwork::AddressLength FAR *optlen);
UInt32 (PASCAL FAR *CNetwork::swaphtonl)(UInt32 hostlong);
UInt16 (PASCAL FAR *CNetwork::swaphtons)(UInt16 hostshort);
unsigned long (PASCAL FAR *CNetwork::inet_addr)(const char FAR * cp);
char FAR * (PASCAL FAR *CNetwork::inet_ntoa)(struct in_addr in);
int (PASCAL FAR *CNetwork::listen)(CNetwork::Socket s, int backlog);
UInt32 (PASCAL FAR *CNetwork::swapntohl)(UInt32 netlong);
UInt16 (PASCAL FAR *CNetwork::swapntohs)(UInt16 netshort);
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
struct hostent FAR * (PASCAL FAR *CNetwork::gethostbyaddr)(const char FAR * addr, int len, int type);
struct hostent FAR * (PASCAL FAR *CNetwork::gethostbyname)(const char FAR * name);
int (PASCAL FAR *CNetwork::gethostname)(char FAR * name, int namelen);
struct servent FAR * (PASCAL FAR *CNetwork::getservbyport)(int port, const char FAR * proto);
struct servent FAR * (PASCAL FAR *CNetwork::getservbyname)(const char FAR * name, const char FAR * proto);
struct protoent FAR * (PASCAL FAR *CNetwork::getprotobynumber)(int proto);
struct protoent FAR * (PASCAL FAR *CNetwork::getprotobyname)(const char FAR * name);
int (PASCAL FAR *CNetwork::getsockerror)(void);
int (PASCAL FAR *CNetwork::gethosterror)(void);

#if defined(CONFIG_PLATFORM_WIN32)

int (PASCAL FAR *CNetwork::WSACleanup)(void);
int (PASCAL FAR *CNetwork::__WSAFDIsSet)(CNetwork::Socket, fd_set FAR *);
const int				CNetwork::Error = SOCKET_ERROR;
const CNetwork::Socket	CNetwork::Null = INVALID_SOCKET;

#undef FD_ISSET
#define FD_ISSET(fd, set) CNetwork::__WSAFDIsSet((SOCKET)(fd), (fd_set FAR *)(set))

static HMODULE			s_networkModule = NULL;

static FARPROC			netGetProcAddress(HMODULE module, LPCSTR name)
{
	FARPROC func = ::GetProcAddress(module, name);
	if (!func)
		throw XNetworkFunctionUnavailable(name);
	return func;
}

void					CNetwork::init()
{
	assert(WSACleanup      == NULL);
	assert(s_networkModule == NULL);

	// try winsock 2
	HMODULE module = (HMODULE)::LoadLibrary("ws2_32.dll");
	if (module == NULL) {
		log((CLOG_DEBUG "ws2_32.dll not found"));
	}
	else {
		try {
			init2(module);
			return;
		}
		catch (XNetwork& e) {
			log((CLOG_DEBUG "ws2_32.dll error: %s", e.what()));
		}
	}

	// try winsock 1
	module = (HMODULE)::LoadLibrary("wsock32.dll");
	if (module == NULL) {
		log((CLOG_DEBUG "wsock32.dll not found"));
	}
	else {
		try {
			init2(module);
			return;
		}
		catch (XNetwork& e) {
			log((CLOG_DEBUG "wsock32.dll error: %s", e.what()));
		}
	}

	// no networking
	throw XNetworkUnavailable();
}

void					CNetwork::cleanup()
{
	if (s_networkModule != NULL) {
		WSACleanup();
		::FreeLibrary(s_networkModule);

		WSACleanup      = NULL;
		s_networkModule = NULL;
	}
}

#define setfunc(var, name, type) 	var = (type)netGetProcAddress(module, #name)

void					CNetwork::init2(HMODULE module)
{
	assert(module != NULL);

	// get startup function address
	int (PASCAL FAR *startup)(WORD, LPWSADATA);
	setfunc(startup, WSAStartup, int(PASCAL FAR*)(WORD, LPWSADATA));

	// startup network library
	WORD version = MAKEWORD(1 /*major*/, 1 /*minor*/);
	WSADATA data;
	int err = startup(version, &data);
	if (data.wVersion != version)
		throw XNetworkVersion(LOBYTE(data.wVersion), HIBYTE(data.wVersion));
	if (err != 0)
		throw XNetworkFailed();

	// get function addresses
	setfunc(accept, accept, Socket (PASCAL FAR *)(Socket s, Address FAR *addr, AddressLength FAR *addrlen));
	setfunc(bind, bind, int (PASCAL FAR *)(Socket s, const Address FAR *addr, AddressLength namelen));
	setfunc(close, closesocket, int (PASCAL FAR *)(Socket s));
	setfunc(connect, connect, int (PASCAL FAR *)(Socket s, const Address FAR *name, AddressLength namelen));
	setfunc(ioctl, ioctlsocket, int (PASCAL FAR *)(Socket s, int cmd, ...));
	setfunc(getpeername, getpeername, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockname, getsockname, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockopt, getsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, void FAR * optval, AddressLength FAR *optlen));
	setfunc(swaphtonl, htonl, UInt32 (PASCAL FAR *)(UInt32 hostlong));
	setfunc(swaphtons, htons, UInt16 (PASCAL FAR *)(UInt16 hostshort));
	setfunc(inet_addr, inet_addr, unsigned long (PASCAL FAR *)(const char FAR * cp));
	setfunc(inet_ntoa, inet_ntoa, char FAR * (PASCAL FAR *)(struct in_addr in));
	setfunc(listen, listen, int (PASCAL FAR *)(Socket s, int backlog));
	setfunc(swapntohl, ntohl, UInt32 (PASCAL FAR *)(UInt32 netlong));
	setfunc(swapntohs, ntohs, UInt16 (PASCAL FAR *)(UInt16 netshort));
	setfunc(recv, recv, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags));
	setfunc(recvfrom, recvfrom, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags, Address FAR *from, AddressLength FAR * fromlen));
	setfunc(send, send, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags));
	setfunc(sendto, sendto, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags, const Address FAR *to, AddressLength tolen));
	setfunc(setsockopt, setsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, const void FAR * optval, AddressLength optlen));
	setfunc(shutdown, shutdown, int (PASCAL FAR *)(Socket s, int how));
	setfunc(socket, socket, Socket (PASCAL FAR *)(int af, int type, int protocol));
	setfunc(gethostbyaddr, gethostbyaddr, struct hostent FAR * (PASCAL FAR *)(const char FAR * addr, int len, int type));
	setfunc(gethostbyname, gethostbyname, struct hostent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(gethostname, gethostname, int (PASCAL FAR *)(char FAR * name, int namelen));
	setfunc(getservbyport, getservbyport, struct servent FAR * (PASCAL FAR *)(int port, const char FAR * proto));
	setfunc(getservbyname, getservbyname, struct servent FAR * (PASCAL FAR *)(const char FAR * name, const char FAR * proto));
	setfunc(getprotobynumber, getprotobynumber, struct protoent FAR * (PASCAL FAR *)(int proto));
	setfunc(getprotobyname, getprotobyname, struct protoent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(getsockerror, WSAGetLastError, int (PASCAL FAR *)(void));
	setfunc(gethosterror, WSAGetLastError, int (PASCAL FAR *)(void));
	setfunc(WSACleanup, WSACleanup, int (PASCAL FAR *)(void));
	setfunc(__WSAFDIsSet, __WSAFDIsSet, int (PASCAL FAR *)(CNetwork::Socket, fd_set FAR *));
	setfunc(select, select, int (PASCAL FAR *)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout));
	poll  = poll2;
	read  = read2;
	write = write2;

	s_networkModule = module;
}

int PASCAL FAR			CNetwork::poll2(PollEntry fd[], int nfds, int timeout)
{
	int i;

	// prepare sets for select
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
		}
		if (fd[i].events & kPOLLOUT) {
			FD_SET(fd[i].fd, &writeSet);
			writeSetP = &writeSet;
		}
		if (true) {
			FD_SET(fd[i].fd, &errSet);
			errSetP = &errSet;
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
	int n = select(0, readSetP, writeSetP, errSetP, timeout2P);

	// handle results
	if (n == Error)
		return Error;
	if (n == 0)
		return 0;
	for (i = 0; i < nfds; ++i) {
		fd[i].revents = 0;
		if (FD_ISSET(fd[i].fd, &readSet))
			fd[i].revents |= kPOLLIN;
		if (FD_ISSET(fd[i].fd, &writeSet))
			fd[i].revents |= kPOLLOUT;
		if (FD_ISSET(fd[i].fd, &errSet))
			fd[i].revents |= kPOLLERR;
	}
	return n;
}

ssize_t PASCAL FAR		CNetwork::read2(Socket s, void FAR * buf, size_t len)
{
	return recv(s, buf, len, 0);
}

ssize_t PASCAL FAR		CNetwork::write2(Socket s,
								const void FAR * buf, size_t len)
{
	return send(s, buf, len, 0);
}

#endif

#if defined(CONFIG_PLATFORM_UNIX)

#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

// FIXME -- use reentrant versions of non-reentrant functions

#define setfunc(var, name, type) 	var = (type)::name

static UInt32			myhtonl(UInt32 v)
{
	return htonl(v);
}

static UInt16			myhtons(UInt16 v)
{
	return htons(v);
}

static UInt32			myntohl(UInt32 v)
{
	return ntohl(v);
}

static UInt16			myntohs(UInt16 v)
{
	return ntohs(v);
}

static int				myerrno()
{
	return errno;
}

static int				myherrno()
{
	return h_errno;
}

static int				mygethostname(char* name, int namelen)
{
	return gethostname(name, namelen);
}

const int				CNetwork::Error = -1;
const CNetwork::Socket	CNetwork::Null = -1;

void					CNetwork::init()
{
	setfunc(accept, accept, Socket (PASCAL FAR *)(Socket s, Address FAR *addr, AddressLength FAR *addrlen));
	setfunc(bind, bind, int (PASCAL FAR *)(Socket s, const Address FAR *addr, AddressLength namelen));
	setfunc(close, close, int (PASCAL FAR *)(Socket s));
	setfunc(connect, connect, int (PASCAL FAR *)(Socket s, const Address FAR *name, AddressLength namelen));
	setfunc(ioctl, ioctl, int (PASCAL FAR *)(Socket s, int cmd, ...));
	setfunc(getpeername, getpeername, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockname, getsockname, int (PASCAL FAR *)(Socket s, Address FAR *name, AddressLength FAR * namelen));
	setfunc(getsockopt, getsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, void FAR * optval, AddressLength FAR *optlen));
	setfunc(swaphtonl, myhtonl, UInt32 (PASCAL FAR *)(UInt32 hostlong));
	setfunc(swaphtons, myhtons, UInt16 (PASCAL FAR *)(UInt16 hostshort));
	setfunc(inet_addr, inet_addr, unsigned long (PASCAL FAR *)(const char FAR * cp));
	setfunc(inet_ntoa, inet_ntoa, char FAR * (PASCAL FAR *)(struct in_addr in));
	setfunc(listen, listen, int (PASCAL FAR *)(Socket s, int backlog));
	setfunc(swapntohl, myntohl, UInt32 (PASCAL FAR *)(UInt32 netlong));
	setfunc(swapntohs, myntohs, UInt16 (PASCAL FAR *)(UInt16 netshort));
	setfunc(poll, poll, int (PASCAL FAR *)(CNetwork::PollEntry fds[], int nfds, int timeout));
	setfunc(read, read, ssize_t (PASCAL FAR *)(CNetwork::Socket s, void FAR * buf, size_t len));
	setfunc(recv, recv, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags));
	setfunc(recvfrom, recvfrom, ssize_t (PASCAL FAR *)(Socket s, void FAR * buf, size_t len, int flags, Address FAR *from, AddressLength FAR * fromlen));
	setfunc(send, send, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags));
	setfunc(sendto, sendto, ssize_t (PASCAL FAR *)(Socket s, const void FAR * buf, size_t len, int flags, const Address FAR *to, AddressLength tolen));
	setfunc(setsockopt, setsockopt, int (PASCAL FAR *)(Socket s, int level, int optname, const void FAR * optval, AddressLength optlen));
	setfunc(shutdown, shutdown, int (PASCAL FAR *)(Socket s, int how));
	setfunc(socket, socket, Socket (PASCAL FAR *)(int af, int type, int protocol));
	setfunc(write, write, ssize_t (PASCAL FAR *)(CNetwork::Socket s, const void FAR * buf, size_t len));
	setfunc(gethostbyaddr, gethostbyaddr, struct hostent FAR * (PASCAL FAR *)(const char FAR * addr, int len, int type));
	setfunc(gethostbyname, gethostbyname, struct hostent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(gethostname, mygethostname, int (PASCAL FAR *)(char FAR * name, int namelen));
	setfunc(getservbyport, getservbyport, struct servent FAR * (PASCAL FAR *)(int port, const char FAR * proto));
	setfunc(getservbyname, getservbyname, struct servent FAR * (PASCAL FAR *)(const char FAR * name, const char FAR * proto));
	setfunc(getprotobynumber, getprotobynumber, struct protoent FAR * (PASCAL FAR *)(int proto));
	setfunc(getprotobyname, getprotobyname, struct protoent FAR * (PASCAL FAR *)(const char FAR * name));
	setfunc(getsockerror, myerrno, int (PASCAL FAR *)(void));
	setfunc(gethosterror, myherrno, int (PASCAL FAR *)(void));
}

void					CNetwork::cleanup()
{
	// do nothing
}

#endif
