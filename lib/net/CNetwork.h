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

#if !defined(HAVE_SOCKLEN_T)
// Darwin is so unsure what to use for socklen_t it makes us choose
#	if defined(__APPLE__)
#		if !defined(_BSD_SOCKLEN_T_) 	
#			define _BSD_SOCKLEN_T_ int
#		endif
#	else
typedef int socklen_t;
#	endif
#endif

#if WINDOWS_LIKE
	// declare no functions in winsock2
#	define INCL_WINSOCK_API_PROTOTYPES 0
#	define INCL_WINSOCK_API_TYPEDEFS 0
#	include <winsock2.h>
typedef int ssize_t;
#	define SELECT_TYPE_ARG1 int
#	define SELECT_TYPE_ARG234 (fd_set *)
#	define SELECT_TYPE_ARG5 (struct timeval *)
#endif

#if UNIX_LIKE
#	if HAVE_SYS_TYPES_H
#		include <sys/types.h>
#	endif
#	if HAVE_SYS_SOCKET_H
#		include <sys/socket.h>
#	endif
#	if HAVE_POLL
#		include <sys/poll.h>
#	endif
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

#if !HAVE_POLL
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

	static Socket		accept(Socket s, Address* addr, AddressLength* addrlen);
	static int			bind(Socket s, const Address* addr, AddressLength namelen);
	static int			close(Socket s);
	static int			connect(Socket s, const Address* name, AddressLength namelen);
	static int			gethostname(char* name, int namelen);
	static int			getpeername(Socket s, Address* name, AddressLength* namelen);
	static int			getsockerror(void);
	static int			getsockname(Socket s, Address* name, AddressLength* namelen);
	static int			getsockopt(Socket s, int level, int optname, void* optval, AddressLength* optlen);
	static int			inet_aton(const char* cp, InternetAddress* addr);
	static CString		inet_ntoa(struct in_addr in);
	static int			ioctl(Socket s, int cmd, void* );
	static int			listen(Socket s, int backlog);
	static int			poll(PollEntry[], int nfds, int timeout);
	static ssize_t		read(Socket s, void* buf, size_t len);
	static ssize_t		recv(Socket s, void* buf, size_t len, int flags);
	static ssize_t		recvfrom(Socket s, void* buf, size_t len, int flags, Address* from, AddressLength* fromlen);
	static ssize_t		send(Socket s, const void* buf, size_t len, int flags);
	static ssize_t		sendto(Socket s, const void* buf, size_t len, int flags, const Address* to, AddressLength tolen);
	static int			setsockopt(Socket s, int level, int optname, const void* optval, AddressLength optlen);
	static int			shutdown(Socket s, int how);
	static Socket		socket(int af, int type, int protocol);
	static ssize_t		write(Socket s, const void* buf, size_t len);
	static int			gethostbyaddr(CHostInfo* hostinfo, const char* addr, int len, int type);
	static int			gethostbyname(CHostInfo* hostinfo, const char* name);
	static int			getservbyport(CServiceInfo* servinfo, int port, const char* proto);
	static int			getservbyname(CServiceInfo* servinfo, const char* name, const char* proto);
	static int			getprotobynumber(CProtocolInfo* protoinfo, int proto);
	static int			getprotobyname(CProtocolInfo* protoinfo, const char* name);

	// convenience functions (only available after init())

	//! Set socket to (non-)blocking operation
	static int			setblocking(Socket s, bool blocking);

	//! Turn Nagle algorithm on or off on socket
	/*!
	Set socket to send messages immediately (true) or to collect small
	messages into one packet (false).
	*/
	static int			setnodelay(Socket s, bool nodelay);

private:
#if WINDOWS_LIKE
	static void			init2(HMODULE);
#endif
};

#endif
