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

#include "XSocket.h"
#include "CNetwork.h"

//
// XSocketAddress
//

XSocketAddress::XSocketAddress(EError error,
				const CString& hostname, UInt16 port) throw() :
	m_error(error),
	m_hostname(hostname),
	m_port(port)
{
	// do nothing
}

XSocketAddress::EError
XSocketAddress::getError() const throw()
{
	return m_error;
}

CString
XSocketAddress::getHostname() const throw()
{
	return m_hostname;
}

UInt16
XSocketAddress::getPort() const throw()
{
	return m_port;
}

CString
XSocketAddress::getWhat() const throw()
{
	static const char* s_errorID[] = {
		"XSocketAddressUnknown",
		"XSocketAddressNotFound",
		"XSocketAddressNoAddress",
		"XSocketAddressBadPort"
	};
	static const char* s_errorMsg[] = {
		"unknown error for: %{1}:%{2}",
		"address not found for: %{1}",
		"no address for: %{1}",
		"invalid port"				// m_port may not be set to the bad port
	};
	return format(s_errorID[m_error], s_errorMsg[m_error],
								m_hostname.c_str(), 
								CStringUtil::print("%d", m_port).c_str());
}


//
// XSocketErrno
//

XSocketErrno::XSocketErrno() :
	MXErrno(CNetwork::getsockerror())
{
	// do nothing
}

XSocketErrno::XSocketErrno(int err) :
	MXErrno(err)
{
	// do nothing
}

const char*
XSocketErrno::getErrstr() const
{
#if WINDOWS_LIKE
	// built-in windows function for looking up error message strings
	// may not look up network error messages correctly.  we'll have
	// to do it ourself.
	static const struct { int m_code; const char* m_msg; } s_netErrorCodes[] = {
		/* 10004 */{WSAEINTR,		"The (blocking) call was canceled via WSACancelBlockingCall"},
		/* 10009 */{WSAEBADF,		"Bad file handle"},
		/* 10013 */{WSAEACCES,		"The requested address is a broadcast address, but the appropriate flag was not set"},
		/* 10014 */{WSAEFAULT,		"WSAEFAULT"},
		/* 10022 */{WSAEINVAL,		"WSAEINVAL"},
		/* 10024 */{WSAEMFILE,		"No more file descriptors available"},
		/* 10035 */{WSAEWOULDBLOCK,	"Socket is marked as non-blocking and no connections are present or the receive operation would block"},
		/* 10036 */{WSAEINPROGRESS,	"A blocking Windows Sockets operation is in progress"},
		/* 10037 */{WSAEALREADY,	"The asynchronous routine being canceled has already completed"},
		/* 10038 */{WSAENOTSOCK,	"At least on descriptor is not a socket"},
		/* 10039 */{WSAEDESTADDRREQ,	"A destination address is required"},
		/* 10040 */{WSAEMSGSIZE,	"The datagram was too large to fit into the specified buffer and was truncated"},
		/* 10041 */{WSAEPROTOTYPE,	"The specified protocol is the wrong type for this socket"},
		/* 10042 */{WSAENOPROTOOPT,	"The option is unknown or unsupported"},
		/* 10043 */{WSAEPROTONOSUPPORT,"The specified protocol is not supported"},
		/* 10044 */{WSAESOCKTNOSUPPORT,"The specified socket type is not supported by this address family"},
		/* 10045 */{WSAEOPNOTSUPP,	"The referenced socket is not a type that supports that operation"},
		/* 10046 */{WSAEPFNOSUPPORT,	"BSD: Protocol family not supported"},
		/* 10047 */{WSAEAFNOSUPPORT,	"The specified address family is not supported"},
		/* 10048 */{WSAEADDRINUSE,	"The specified address is already in use"},
		/* 10049 */{WSAEADDRNOTAVAIL,	"The specified address is not available from the local machine"},
		/* 10050 */{WSAENETDOWN,	"The Windows Sockets implementation has detected that the network subsystem has failed"},
		/* 10051 */{WSAENETUNREACH,	"The network can't be reached from this hos at this time"},
		/* 10052 */{WSAENETRESET,	"The connection must be reset because the Windows Sockets implementation dropped it"},
		/* 10053 */{WSAECONNABORTED,	"The virtual circuit was aborted due to timeout or other failure"},
		/* 10054 */{WSAECONNRESET,	"The virtual circuit was reset by the remote side"},
		/* 10055 */{WSAENOBUFS,		"No buffer space is available or a buffer deadlock has occured. The socket cannot be created"},
		/* 10056 */{WSAEISCONN,		"The socket is already connected"},
		/* 10057 */{WSAENOTCONN,	"The socket is not connected"},
		/* 10058 */{WSAESHUTDOWN,	"The socket has been shutdown"},
		/* 10059 */{WSAETOOMANYREFS,	"BSD: Too many references"},
		/* 10060 */{WSAETIMEDOUT,	"Attempt to connect timed out without establishing a connection"},
		/* 10061 */{WSAECONNREFUSED,	"The attempt to connect was forcefully rejected"},
		/* 10062 */{WSAELOOP,		"Undocumented WinSock error code used in BSD"},
		/* 10063 */{WSAENAMETOOLONG,	"Undocumented WinSock error code used in BSD"},
		/* 10064 */{WSAEHOSTDOWN,	"Undocumented WinSock error code used in BSD"},
		/* 10065 */{WSAEHOSTUNREACH,	"No route to host"},
		/* 10066 */{WSAENOTEMPTY,	"Undocumented WinSock error code"},
		/* 10067 */{WSAEPROCLIM,	"Undocumented WinSock error code"},
		/* 10068 */{WSAEUSERS,		"Undocumented WinSock error code"},
		/* 10069 */{WSAEDQUOT,		"Undocumented WinSock error code"},
		/* 10070 */{WSAESTALE,		"Undocumented WinSock error code"},
		/* 10071 */{WSAEREMOTE,		"Undocumented WinSock error code"},
		/* 10091 */{WSASYSNOTREADY,	"Underlying network subsytem is not ready for network communication"},
		/* 10092 */{WSAVERNOTSUPPORTED,	"The version of WinSock API support requested is not provided in this implementation"},
		/* 10093 */{WSANOTINITIALISED,	"WinSock subsystem not properly initialized"},
		/* 10101 */{WSAEDISCON,		"Virtual circuit has gracefully terminated connection"},
		/* 11001 */{WSAHOST_NOT_FOUND,	"Host not found"},
		/* 11002 */{WSATRY_AGAIN,	"Host not found"},
		/* 11003 */{WSANO_RECOVERY,	"Host name lookup error"},
		/* 11004 */{WSANO_DATA,		"No data for host"},
		/* end   */{0,			NULL}
	};

	const int err = getErrno();
	for (unsigned int i = 0; s_netErrorCodes[i].m_code != 0; ++i) {
		if (s_netErrorCodes[i].m_code == err) {
			return s_netErrorCodes[i].m_msg;
		}
	}
#endif

	// not a network error code.  fallback to system error message.
	return MXErrno::getErrstr();
}


//
// XSocketIOClose
//

CString
XSocketIOClose::getWhat() const throw()
{
	return format("XSocketIOClose", "close: %{1}",
								getErrstr());
}


//
// XSocketBind
//

CString
XSocketBind::getWhat() const throw()
{
	return format("XSocketBind", "cannot bind address: %{1}",
								getErrstr());
}


//
// XSocketConnect
//

CString
XSocketConnect::getWhat() const throw()
{
	return format("XSocketConnect", "cannot connect socket: %{1}",
								getErrstr());
}


//
// XSocketCreate
//

CString
XSocketCreate::getWhat() const throw()
{
	return format("XSocketCreate", "cannot create socket: %{1}",
								getErrstr());
}
