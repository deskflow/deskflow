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

#ifndef CNETWORK_H
#define CNETWORK_H

#include "BasicTypes.h"
#include "CString.h"
#include "stdvector.h"

#if HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#	include <sys/socket.h>
#endif
#if HAVE_POLL
#	include <sys/poll.h>
#endif

#if WINDOWS_LIKE
	// declare no functions in winsock2
#	define INCL_WINSOCK_API_PROTOTYPES 0
#	define INCL_WINSOCK_API_TYPEDEFS 0
#	include <winsock2.h>
typedef int ssize_t;
#else
#	undef FAR
#	undef PASCAL
#	define FAR
#	define PASCAL
#endif

#if UNIX_LIKE
#	include <netinet/in.h>
#	include <netdb.h>
#	include <errno.h>
#endif

//! Networking functions
class CNetwork {
public:
	// platform dependent types
#if WINDOWS_LIKE
	typedef SOCKET Socket;
	typedef struct sockaddr Address;
	typedef int AddressLength;
	typedef short AddressType;
	typedef struct in_addr InternetAddress;
#elif UNIX_LIKE
	typedef int Socket;
	typedef struct sockaddr Address;
	typedef socklen_t AddressLength;
	typedef int AddressType;
	typedef struct in_addr InternetAddress;
#endif

#if WINDOWS_LIKE || !HAVE_POLL
	class PollEntry {
	public:
		Socket			fd;
		short			events;
		short			revents;
	};
	enum {
		kPOLLIN   = 1,
		kPOLLOUT  = 2,
		kPOLLERR  = 4,
		kPOLLNVAL = 8
	};
#else
	typedef struct pollfd PollEntry;
	enum {
		kPOLLIN   = POLLIN,
		kPOLLOUT  = POLLOUT,
		kPOLLERR  = POLLERR,
		kPOLLNVAL = POLLNVAL
	};
#endif

	//! Host name information
	class CHostInfo {
	public:
		CHostInfo() { }
		CHostInfo(const struct hostent*);

		void			swap(CHostInfo&);

	public:
		typedef std::vector<CString> AliasList;
		typedef std::vector<const char*> AddressList;

		CString			m_name;
		AliasList		m_aliases;
		AddressType		m_addressType;
		int				m_addressLength;
		AddressList		m_addresses;

	private:
		std::string		m_addressData;
	};

	//! Network service information
	class CServiceInfo {
	public:
		CServiceInfo() { }
		CServiceInfo(const struct servent*);

		void			swap(CServiceInfo&);

	public:
		typedef std::vector<CString> AliasList;

		CString			m_name;
		AliasList		m_aliases;
		int				m_port;
		CString			m_protocol;
	};

	//! Network protocol information
	class CProtocolInfo {
	public:
		CProtocolInfo() { }
		CProtocolInfo(const struct protoent*);

		void			swap(CProtocolInfo&);

	public:
		typedef std::vector<CString> AliasList;

		CString			m_name;
		AliasList		m_aliases;
		int				m_protocol;
	};

	//! @name manipulators
	//@{

	//! Initialize network subsystem
	/*!
	This \b must be called before any other calls to the network subsystem.
	*/
	static void			init();

	//! Clean up network subsystem
	/*!
	This should be called when the network subsystem is no longer needed
	and no longer in use.
	*/
	static void			cleanup();

	// byte swapping functions
	//! Swap bytes to network order
	static UInt32		swaphtonl(UInt32 hostlong);
	//! Swap bytes to network order
	static UInt16		swaphtons(UInt16 hostshort);
	//! Swap bytes to host order
	static UInt32		swapntohl(UInt32 netlong);
	//! Swap bytes to host order
	static UInt16		swapntohs(UInt16 netshort);

	//@}
	//! @name constants
	//@{

	//! The error type
	static const int	Error;
	//! The non-socket
	static const Socket	Null;

	// getsockerror() constants
	enum {
#if WINDOWS_LIKE
		kEADDRINUSE				= WSAEADDRINUSE,
		kECONNECTING			= WSAEWOULDBLOCK,
		kEINTR					= WSAEINTR,
#elif UNIX_LIKE
		kEADDRINUSE				= EADDRINUSE,
		kECONNECTING			= EINPROGRESS,
		kEINTR					= EINTR,
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
		kHOST_OK				= 0
	};

	//@}

	// socket interface (only available after init())

	static Socket (PASCAL FAR *accept)(Socket s, Address FAR *addr, AddressLength FAR *addrlen);
	static int (PASCAL FAR *bind)(Socket s, const Address FAR *addr, AddressLength namelen);
	static int (PASCAL FAR *close)(Socket s);
	static int (PASCAL FAR *connect)(Socket s, const Address FAR *name, AddressLength namelen);
	static int (PASCAL FAR *ioctl)(Socket s, int cmd, void FAR *);
	static int (PASCAL FAR *getpeername)(Socket s, Address FAR *name, AddressLength FAR * namelen);
	static int (PASCAL FAR *getsockname)(Socket s, Address FAR *name, AddressLength FAR * namelen);
	static int (PASCAL FAR *getsockopt)(Socket s, int level, int optname, void FAR * optval, AddressLength FAR *optlen);
	static int PASCAL FAR inet_aton(const char FAR * cp, InternetAddress FAR * addr);
	static CString PASCAL FAR inet_ntoa(struct in_addr in);
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
	static int (PASCAL FAR *gethostname)(char FAR * name, int namelen);
	static int PASCAL FAR gethostbyaddr(CHostInfo* hostinfo, const char FAR * addr, int len, int type);
	static int PASCAL FAR gethostbyname(CHostInfo* hostinfo, const char FAR * name);
	static int PASCAL FAR getservbyport(CServiceInfo* servinfo, int port, const char FAR * proto);
	static int PASCAL FAR getservbyname(CServiceInfo* servinfo, const char FAR * name, const char FAR * proto);
	static int PASCAL FAR getprotobynumber(CProtocolInfo* protoinfo, int proto);
	static int PASCAL FAR getprotobyname(CProtocolInfo* protoinfo, const char FAR * name);
	static int (PASCAL FAR *getsockerror)(void);

	// convenience functions (only available after init())

	//! Set socket to (non-)blocking operation
	static int PASCAL FAR setblocking(CNetwork::Socket s, bool blocking);

	//! Turn Nagle algorithm on or off on socket
	/*!
	Set socket to send messages immediately (true) or to collect small
	messages into one packet (false).
	*/
	static int PASCAL FAR setnodelay(CNetwork::Socket s, bool nodelay);

private:
#if WINDOWS_LIKE
#define SELECT_TYPE_ARG1 int
#define SELECT_TYPE_ARG234 (fd_set *)
#define SELECT_TYPE_ARG5 (struct timeval *)
	static void			init2(HMODULE);
	static ssize_t PASCAL FAR read2(Socket s, void FAR * buf, size_t len);
	static ssize_t PASCAL FAR write2(Socket s, const void FAR * buf, size_t len);
	static int (PASCAL FAR *WSACleanup)(void);
	static int (PASCAL FAR *__WSAFDIsSet)(CNetwork::Socket, fd_set FAR *);
	static int (PASCAL FAR *select)(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout);
	static char FAR * (PASCAL FAR *inet_ntoa_n)(struct in_addr in);
	static unsigned long (PASCAL FAR *inet_addr_n)(const char FAR * cp);
	static struct hostent FAR * (PASCAL FAR *gethostbyaddr_n)(const char FAR * addr, int len, int type);
	static struct hostent FAR * (PASCAL FAR *gethostbyname_n)(const char FAR * name);
	static struct servent FAR * (PASCAL FAR *getservbyport_n)(int port, const char FAR * proto);
	static struct servent FAR * (PASCAL FAR *getservbyname_n)(const char FAR * name, const char FAR * proto);
	static struct protoent FAR * (PASCAL FAR *getprotobynumber_n)(int proto);
	static struct protoent FAR * (PASCAL FAR *getprotobyname_n)(const char FAR * name);
#endif

#if UNIX_LIKE
	static int PASCAL FAR gethostname2(char FAR * name, int namelen);
	static int PASCAL FAR getsockerror2(void);
#endif

#if WINDOWS_LIKE || UNIX_LIKE
/* FIXME -- reentrant netdb stuff
create classes for hostent, servent, protoent.
each class can clean itself up automatically.
inside CNetwork we'll convert from netdb structs to classes.
clients will pass a class pointer which CNetwork will assign to (or swap into).
won't need free...() functions to clean up structs.
each class should know how to copy from respective netdb struct.
will need to fix CNetworkAddress to use classes.
*/
	static void copyhostent(struct hostent FAR * dst, const struct hostent FAR * src);
	static void copyservent(struct servent FAR * dst, const struct servent FAR * src);
	static void copyprotoent(struct protoent FAR * dst, const struct protoent FAR * src);

	static void freehostent(struct hostent FAR * ent);
	static void freeservent(struct servent FAR * ent);
	static void freeprotoent(struct protoent FAR * ent);
#endif

#if WINDOWS_LIKE || !HAVE_POLL
	static int PASCAL FAR poll2(PollEntry[], int nfds, int timeout);
#endif
};

#endif
