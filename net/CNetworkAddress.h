#ifndef CNETWORKADDRESS_H
#define CNETWORKADDRESS_H

#include "CNetwork.h"
#include "CString.h"
#include "BasicTypes.h"

class CNetworkAddress {
public:
	// invalid address
	CNetworkAddress();

	// wildcard address and given port.  port must not be zero.
	CNetworkAddress(UInt16 port);

	// given address and port.  if hostname can be parsed as numerical
	// address then that's how it's used, otherwise the hostname is
	// looked up.  if lookup fails then it throws XSocketAddress.  if
	// hostname ends in ":[0-9]+" then that suffix is extracted and
	// used as the port, overridding the port parameter.  neither
	// port may be zero.
	CNetworkAddress(const CString& hostname, UInt16 port);

	~CNetworkAddress();

	// manipulators

	// accessors

	// returns true if this is not the invalid address
	bool				isValid() const;

	// get the address
	const CNetwork::Address*	getAddress() const;
	CNetwork::AddressLength		getAddressLength() const;

	// get the hostname and port (as provided in the c'tor)
	CString				getHostname() const;
	UInt16				getPort() const;

private:
	CNetwork::Address	m_address;
	CString				m_hostname;
	UInt16				m_port;
};

#endif
