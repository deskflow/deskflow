#ifndef CNETWORKADDRESS_H
#define CNETWORKADDRESS_H

#include "BasicTypes.h"
#include "CNetwork.h"
#include "XSocket.h"

class CString;

class CNetworkAddress {
public:
	CNetworkAddress(UInt16 port);
	CNetworkAddress(const CString& hostname, UInt16 port);
	~CNetworkAddress();

	// manipulators

	// accessors

	const CNetwork::Address*	getAddress() const;
	CNetwork::AddressLength		getAddressLength() const;

private:
	CNetwork::Address	m_address;
};

#endif
