#ifndef CNETWORKADDRESS_H
#define CNETWORKADDRESS_H

#include "BasicTypes.h"
#include "XSocket.h"
#include <sys/socket.h>

class CString;

class CNetworkAddress {
  public:
	CNetworkAddress(UInt16 port) throw(XSocketAddress);
	CNetworkAddress(const CString& hostname, UInt16 port) throw(XSocketAddress);
	~CNetworkAddress();

	// manipulators

	// accessors

	const struct sockaddr*	getAddress() const throw();
	int					getAddressLength() const throw();

  private:
	struct sockaddr		m_address;
};

#endif
