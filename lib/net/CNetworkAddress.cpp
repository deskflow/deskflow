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
#include <cstdlib>
#include <cstring>

//
// CNetworkAddress
//

CNetworkAddress::CNetworkAddress() :
	m_port(0)
{
	// note -- make no calls to CNetwork socket interface here;
	// we're often called prior to CNetwork::init().

	struct sockaddr_in* inetAddress = reinterpret_cast<
										struct sockaddr_in*>(&m_address);
	inetAddress->sin_family      = AF_INET;
	inetAddress->sin_port        = CNetwork::swaphtons(m_port);
	inetAddress->sin_addr.s_addr = INADDR_ANY;
	memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
}

CNetworkAddress::CNetworkAddress(UInt16 port) :
	m_port(port)
{
	if (port == 0) {
		throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, m_port);
	}

	struct sockaddr_in* inetAddress = reinterpret_cast<
										struct sockaddr_in*>(&m_address);
	inetAddress->sin_family      = AF_INET;
	inetAddress->sin_port        = CNetwork::swaphtons(m_port);
	inetAddress->sin_addr.s_addr = INADDR_ANY;
	memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
}

CNetworkAddress::CNetworkAddress(const CString& hostname_, UInt16 port) :
	m_hostname(hostname_),
	m_port(port)
{
	CString hostname(m_hostname);

	if (port == 0) {
		throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, m_port);
	}

	// check for port suffix
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
											m_hostname, m_port);
			}
			else {
				// good port
				port = static_cast<UInt16>(suffixPort);
				hostname.erase(i);
			}
		}
	}

	// if hostname is empty then use wildcard address
	struct sockaddr_in* inetAddress = reinterpret_cast<
										struct sockaddr_in*>(&m_address);
	if (hostname.empty()) {
		inetAddress->sin_family      = AF_INET;
		inetAddress->sin_port        = CNetwork::swaphtons(port);
		inetAddress->sin_addr.s_addr = INADDR_ANY;
		memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
		return;
	}

	// convert dot notation to address
	if (CNetwork::inet_aton(hostname.c_str(), &inetAddress->sin_addr) != 0) {
		inetAddress->sin_family = AF_INET;
		inetAddress->sin_port   = CNetwork::swaphtons(port);
		memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
		return;
	}

	// look up name
	CNetwork::CHostInfo hostInfo;
	switch (CNetwork::gethostbyname(&hostInfo, hostname.c_str())) {
	case CNetwork::kHOST_OK:
		break;

	case CNetwork::kHOST_NOT_FOUND:
		throw XSocketAddress(XSocketAddress::kNotFound, hostname, port);

	case CNetwork::kNO_DATA:
		throw XSocketAddress(XSocketAddress::kNoAddress, hostname, port);

	case CNetwork::kNO_RECOVERY:
	case CNetwork::kTRY_AGAIN:
	default:
		throw XSocketAddress(XSocketAddress::kUnknown, hostname, port);
	}

	inetAddress->sin_family = hostInfo.m_addressType;
	inetAddress->sin_port   = CNetwork::swaphtons(port);
	memcpy(&inetAddress->sin_addr, hostInfo.m_addresses[0],
									hostInfo.m_addressLength);
	memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
}

CNetworkAddress::~CNetworkAddress()
{
	// do nothing
}

bool
CNetworkAddress::isValid() const
{
	return (m_port != 0);
}

const CNetwork::Address*
CNetworkAddress::getAddress() const
{
	return &m_address;
}
					
CNetwork::AddressLength
CNetworkAddress::getAddressLength() const
{
	return sizeof(m_address);
}

CString
CNetworkAddress::getHostname() const
{
	return m_hostname;
}

UInt16
CNetworkAddress::getPort() const
{
	return m_port;
}
