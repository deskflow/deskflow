#ifndef CNETWORK_H
#define CNETWORK_H

#include "BasicTypes.h"

#if WINDOWS_LIKE
	// declare no functions in winsock2
#	define INCL_WINSOCK_API_PROTOTYPES 0
#	define INCL_WINSOCK_API_TYPEDEFS 0
#	include <winsock2.h>
typedef int ssize_t;
#	if !defined(SOL_TCP)
# 		define SOL_TCP IPPROTO_TCP
#	endif
#else
#	undef FAR
#	undef PASCAL
#	define FAR
#	define PASCAL
#endif

#if UNIX_LIKE
# include <sys/types.h>
# include <sys/poll.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <errno.h>
# if !defined(TCP_NODELAY) || !defined(SOL_TCP)
#  include <netinet/tcp.h>
# endif
#endif

class CNetwork {
public:
#if WINDOWS_LIKE
	typedef SOCKET Socket;
	typedef struct sockaddr Address;
	typedef int AddressLength;
	typedef BOOL TCPNoDelayType;
	class PollEntry {
	public:
		Socket			fd;
		short			events;
		short			revents;
	};
	enum {
		kPOLLIN = 1,
		kPOLLOUT = 2,
		kPOLLERR = 4,
		kPOLLNVAL = 8
	};
#elif UNIX_LIKE
	typedef int Socket;
	typedef struct sockaddr Address;
	typedef socklen_t AddressLength;
	typedef struct pollfd PollEntry;
	typedef int TCPNoDelayType;
	enum {
		kPOLLIN = POLLIN,
		kPOLLOUT = POLLOUT,
		kPOLLERR = POLLERR,
		kPOLLNVAL = POLLNVAL
	};
#endif

	// manipulators

	static void			init();
	static void			cleanup();

	// byte swapping functions
	static UInt32		swaphtonl(UInt32 hostlong);
	static UInt16		swaphtons(UInt16 hostshort);
	static UInt32		swapntohl(UInt32 netlong);
	static UInt16		swapntohs(UInt16 netshort);

	// constants

	static const int	Error;
	static const Socket	Null;

	// getsockerror() constants
	enum {
#if WINDOWS_LIKE
		kEADDRINUSE				= WSAEADDRINUSE,
		kECONNECTING			= WSAEWOULDBLOCK,
#elif UNIX_LIKE
		kEADDRINUSE				= EADDRINUSE,
		kECONNECTING			= EINPROGRESS,
#endif
		kNone = 0
	};

	// gethosterror() constants
	enum {
#if WINDOWS_LIKE
		kHOST_NOT_FOUND			= WSAHOST_NOT_FOUND,
		kNO_DATA				= WSANO_DATA,
		kNO_RECOVERY			= WSANO_RECOVERY,
		kTRY_AGAIN				= WSATRY_AGAIN,
#elif UNIX_LIKE
		kHOST_NOT_FOUND			= HOST_NOT_FOUND,
		kNO_DATA				= NO_DATA,
		kNO_RECOVERY			= NO_RECOVERY,
		kTRY_AGAIN				= TRY_AGAIN,
#endif
		kHNone = 0
	};

	// socket interface (only available after init())

	static Socket (PASCAL FAR *accept)(Socket s, Address FAR *addr, AddressLength FAR *addrlen);
	static int (PASCAL FAR *bind)(Socket s, const Address FAR *addr, AddressLength namelen);
	static int (PASCAL FAR *close)(Socket s);
	static int (PASCAL FAR *connect)(Socket s, const Address FAR *name, AddressLength namelen);
	static int (PASCAL FAR *ioctl)(Socket s, int cmd, ...);
	static int (PASCAL FAR *getpeername)(Socket s, Address FAR *name, AddressLength FAR * namelen);
	static int (PASCAL FAR *getsockname)(Socket s, Address FAR *name, AddressLength FAR * namelen);
	static int (PASCAL FAR *getsockopt)(Socket s, int level, int optname, void FAR * optval, AddressLength FAR *optlen);
	static unsigned long (PASCAL FAR *inet_addr)(const char FAR * cp);
	static char FAR * (PASCAL FAR *inet_ntoa)(struct in_addr in);
	static int (PASCAL FAR *listen)(Socket s, int backlog);
	static ssize_t (PASCAL FAR *read)(Socket s, void FAR * buf, size_t len);
	static ssize_t (PASCAL FAR *recv)(Socket s, void FAR * buf, size_t len, int flags);
	static ssize_t (PASCAL FAR *recvfrom)(Socket s, void FAR * buf, size_t len, int flags, Address FAR *from, AddressLength FAR * fromlen);
	static int (PASCAL FAR *poll)(PollEntry[], int nfds, int timeout);
	static ssize_t (PASCAL FAR *send)(Socket s, const void FAR * buf, size_t len, int flags);
	static ssize_t (PASCAL FAR *sendto)(Socket s, const void FAR * buf, size_t len, int flags, const Address FAR *to, AddressLength tolen);
	static int (PASCAL FAR *setsockopt)(Socket s, int level, int optname, const void FAR * optval, AddressLength optlen);
	static int (PASCAL FAR *shutdown)(Socket s, int how);
	static Socket (PASCAL FAR *socket)(int af, int type, int protocol);
	static ssize_t (PASCAL FAR *write)(Socket s, const void FAR * buf, size_t len);
	static struct hostent FAR * (PASCAL FAR *gethostbyaddr)(const char FAR * addr, int len, int type);
	static struct hostent FAR * (PASCAL FAR *gethostbyname)(const char FAR * name);
	static int (PASCAL FAR *gethostname)(char FAR * name, int namelen);
	static struct servent FAR * (PASCAL FAR *getservbyport)(int port, const char FAR * proto);
	static struct servent FAR * (PASCAL FAR *getservbyname)(const char FAR * name, const char FAR * proto);
	static struct protoent FAR * (PASCAL FAR *getprotobynumber)(int proto);
	static struct protoent FAR * (PASCAL FAR *getprotobyname)(const char FAR * name);
	static int (PASCAL FAR *getsockerror)(void);
	static int (PASCAL FAR *gethosterror)(void);

	// convenience functions (only available after init())

	static int (PASCAL FAR *setblocking)(CNetwork::Socket s, bool blocking);

#if WINDOWS_LIKE
private:
	static void			init2(HMODULE);
	static int PASCAL FAR poll2(PollEntry[], int nfds, int timeout);
	static ssize_t PASCAL FAR read2(Socket s, void FAR * buf, size_t len);
	static ssize_t PASCAL FAR write2(Socket s, const void FAR * buf, size_t len);
	static int PASCAL FAR setblocking2(CNetwork::Socket s, bool blocking);
	static int (PASCAL FAR *WSACleanup)(void);
	static int (PASCAL FAR *__WSAFDIsSet)(CNetwork::Socket, fd_set FAR *);
	static int (PASCAL FAR *select)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout);
#endif
};

#endif
