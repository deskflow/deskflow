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

#include "CNetworkAddress.h"
#include "XSocket.h"
#include "CArch.h"
#include "XArch.h"
#include <cstdlib>

//
// CNetworkAddress
//

CNetworkAddress::CNetworkAddress() :
	m_address(NULL)
{
	// note -- make no calls to CNetwork socket interface here;
	// we're often called prior to CNetwork::init().
}

CNetworkAddress::CNetworkAddress(int port)
{
	if (port == 0) {
		throw XSocketAddress(XSocketAddress::kBadPort, "", port);
	}

	m_address = ARCH->newAnyAddr(IArchNetwork::kINET);
	ARCH->setAddrPort(m_address, port);
}

CNetworkAddress::CNetworkAddress(const CNetworkAddress& addr) :
	m_address(ARCH->copyAddr(addr.m_address)),
	m_hostname(addr.m_hostname)
{
	// do nothing
}

CNetworkAddress::CNetworkAddress(const CString& hostname_, int port) :
	m_hostname(hostname_)
{
	if (port == 0) {
		throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, port);
	}

	// check for port suffix
	CString hostname(m_hostname);
	CString::size_type i = hostname.rfind(':');
	if (i != CString::npos && i + 1 < hostname.size()) {
		// found a colon.  see if it looks like an IPv6 address.
		bool colonNotation = false;
		bool dotNotation   = false;
		bool doubleColon   = false;
		for (CString::size_type j = 0; j < i; ++j) {
			if (hostname[j] == ':') {
				colonNotation = true;
				dotNotation   = false;
				if (hostname[j + 1] == ':') {
					doubleColon = true;
				}
			}
			else if (hostname[j] == '.' && colonNotation) {
				dotNotation = true;
			}
		}

		// port suffix is ambiguous with IPv6 notation if there's
		// a double colon and the end of the address is not in dot
		// notation.  in that case we assume it's not a port suffix.
		// the user can replace the double colon with zeros to
		// disambiguate.
		if ((!doubleColon || dotNotation) || !colonNotation) {
			char* end;
			long suffixPort = strtol(hostname.c_str() + i + 1, &end, 10);
			if (end == hostname.c_str() + i + 1 || *end != '\0' ||
				suffixPort <= 0 || suffixPort > 65535) {
				// bogus port
				throw XSocketAddress(XSocketAddress::kBadPort,
											m_hostname, port);
			}
			else {
				// good port
				port = static_cast<int>(suffixPort);
				hostname.erase(i);
			}
		}
	}

	// if hostname is empty then use wildcard address
	if (hostname.empty()) {
		m_address = ARCH->newAnyAddr(IArchNetwork::kINET);
		ARCH->setAddrPort(m_address, port);
	}
	else {
		// look up name
		try {
			m_address = ARCH->nameToAddr(hostname);
			ARCH->setAddrPort(m_address, port);
		}
		catch (XArchNetworkNameUnknown&) {
			throw XSocketAddress(XSocketAddress::kNotFound, hostname, port);
		}
		catch (XArchNetworkNameNoAddress&) {
			throw XSocketAddress(XSocketAddress::kNoAddress, hostname, port);
		}
		catch (XArchNetworkName&) {
			throw XSocketAddress(XSocketAddress::kUnknown, hostname, port);
		}
	}
}

CNetworkAddress::~CNetworkAddress()
{
	if (m_address != NULL) {
		ARCH->closeAddr(m_address);
	}
}

CNetworkAddress&
CNetworkAddress::operator=(const CNetworkAddress& addr)
{
	CArchNetAddress newAddr = NULL;
	if (addr.m_address != NULL) {
		newAddr = ARCH->copyAddr(addr.m_address);
	}
	if (m_address != NULL) {
		ARCH->closeAddr(m_address);
	}
	m_hostname = addr.m_hostname;
	m_address  = newAddr;
	return *this;
}

bool
CNetworkAddress::isValid() const
{
	return (m_address != NULL);
}

const CArchNetAddress&
CNetworkAddress::getAddress() const
{
	return m_address;
}

int
CNetworkAddress::getPort() const
{
	return (m_address == NULL) ? 0 : ARCH->getAddrPort(m_address);
}

CString
CNetworkAddress::getHostname() const
{
	return m_hostname;
}
