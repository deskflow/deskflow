#include "XSocket.h"

//
// XSocketAddress
//

XSocketAddress::XSocketAddress(Error error,
				const CString& hostname, UInt16 port) throw() :
	m_error(error),
	m_hostname(hostname),
	m_port(port)
{
	// do nothing
}

XSocketAddress::Error
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
	return "no address";
/* FIXME
	return format("XSocketAddress", "no address: %1:%2",
								m_hostname.t_str(), 
								CString::sprintf("%d", m_port).t_str());
*/
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
