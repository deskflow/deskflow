#include "XSocket.h"

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
	MXErrno()
{
	// do nothing
}

XSocketErrno::XSocketErrno(int err) :
	MXErrno(err)
{
	// do nothing
}


//
// XSocketBind
//

CString
XSocketBind::getWhat() const throw()
{
	return format("XSocketBind", "cannot bind address");
}


//
// XSocketConnect
//

CString
XSocketConnect::getWhat() const throw()
{
	return format("XSocketConnect", "cannot connect socket");
}


//
// XSocketCreate
//

CString
XSocketCreate::getWhat() const throw()
{
	return format("XSocketCreate", "cannot create socket");
}
