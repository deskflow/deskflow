#include "CNetworkAddress.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//
// CNetworkAddress
//

CNetworkAddress::CNetworkAddress(UInt16 port) throw(XSocketAddress)
{
	if (port == 0)
		throw XSocketAddress(XSocketAddress::kBadPort, CString(), port);

	struct sockaddr_in* inetAddress = reinterpret_cast<struct sockaddr_in*>(&m_address);
	inetAddress->sin_family      = AF_INET;
	inetAddress->sin_port        = htons(port);
	inetAddress->sin_addr.s_addr = INADDR_ANY;
	::memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
}

CNetworkAddress::CNetworkAddress(const CString& hostname, UInt16 port)
								throw(XSocketAddress)
{
	if (port == 0)
		throw XSocketAddress(XSocketAddress::kBadPort, hostname, port);

	struct hostent* hent = gethostbyname(hostname.c_str());
	if (hent == NULL) {
		switch (h_errno) {
		  case HOST_NOT_FOUND:
			throw XSocketAddress(XSocketAddress::kNotFound, hostname, port);

		  case NO_DATA:
			throw XSocketAddress(XSocketAddress::kNoAddress, hostname, port);

		  case NO_RECOVERY:
		  case TRY_AGAIN:
		  default:
			throw XSocketAddress(XSocketAddress::kUnknown, hostname, port);
		}
	}

	struct sockaddr_in* inetAddress = reinterpret_cast<struct sockaddr_in*>(&m_address);
	inetAddress->sin_family = hent->h_addrtype;
	inetAddress->sin_port   = htons(port);
	::memcpy(&inetAddress->sin_addr, hent->h_addr_list[0], hent->h_length);
	::memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
}

CNetworkAddress::~CNetworkAddress()
{
	// do nothing
}

const struct sockaddr*	CNetworkAddress::getAddress() const throw()
{
	return &m_address;
}
					
int						CNetworkAddress::getAddressLength() const throw()
{
	return sizeof(m_address);
}
