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
#include "CStringUtil.h"

//
// XSocketAddress
//

XSocketAddress::XSocketAddress(EError error,
				const CString& hostname, int port) throw() :
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

int
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
// XSocketIOClose
//

CString
XSocketIOClose::getWhat() const throw()
{
	return format("XSocketIOClose", "close: %{1}", what());
}


//
// XSocketBind
//

CString
XSocketBind::getWhat() const throw()
{
	return format("XSocketBind", "cannot bind address: %{1}", what());
}


//
// XSocketConnect
//

CString
XSocketConnect::getWhat() const throw()
{
	return format("XSocketConnect", "cannot connect socket: %{1}", what());
}


//
// XSocketCreate
//

CString
XSocketCreate::getWhat() const throw()
{
	return format("XSocketCreate", "cannot create socket: %{1}", what());
}
