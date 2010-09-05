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

#ifndef CNETWORKADDRESS_H
#define CNETWORKADDRESS_H

#include "CString.h"
#include "BasicTypes.h"
#include "IArchNetwork.h"

//! Network address type
/*!
This class represents a network address.
*/
class CNetworkAddress {
public:
	/*!
	Constructs the invalid address
	*/
	CNetworkAddress();

	/*!
	Construct the wildcard address with the given port.  \c port must
	not be zero.
	*/
	CNetworkAddress(int port);

	/*!
	Construct the network address for the given \c hostname and \c port.
	If \c hostname can be parsed as a numerical address then that's how
	it's used, otherwise the host name is looked up.  If the lookup fails
	then this throws XSocketAddress.  If \c hostname ends in ":[0-9]+" then
	that suffix is extracted and used as the port, overridding the port
	parameter.  Neither the extracted port or \c port may be zero.
	*/
	CNetworkAddress(const CString& hostname, int port);

	CNetworkAddress(const CNetworkAddress&);

	~CNetworkAddress();

	CNetworkAddress&	operator=(const CNetworkAddress&);

	//! @name accessors
	//@{

	//! Check address equality
	/*!
	Returns true if this address is equal to \p address.
	*/
	bool				operator==(const CNetworkAddress&) const;

	//! Check address inequality
	/*!
	Returns true if this address is not equal to \p address.
	*/
	bool				operator!=(const CNetworkAddress&) const;

	//! Check address validity
	/*!
	Returns true if this is not the invalid address.
	*/
	bool				isValid() const;

	//! Get address
	/*!
	Returns the address in the platform's native network address
	structure.
	*/
	const CArchNetAddress&	getAddress() const;

	//! Get port
	/*!
	Returns the port passed to the c'tor as a suffix to the hostname,
	if that existed, otherwise as passed directly to the c'tor.
	*/
	int					getPort() const;

	//! Get hostname
	/*!
	Returns the hostname passed to the c'tor sans the port suffix.
	*/
	CString				getHostname() const;

	//@}

private:
	CArchNetAddress		m_address;
	CString				m_hostname;
};

#endif
