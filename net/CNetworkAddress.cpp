#include "CNetworkAddress.h"

//
// CNetworkAddress
//

CNetworkAddress::CNetworkAddress(UInt16 port)
{
	if (port == 0)
		throw XSocketAddress(XSocketAddress::kBadPort, CString(), port);

	struct sockaddr_in* inetAddress = reinterpret_cast<struct sockaddr_in*>(&m_address);
	inetAddress->sin_family      = AF_INET;
	inetAddress->sin_port        = CNetwork::swaphtons(port);
	inetAddress->sin_addr.s_addr = INADDR_ANY;
	memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
}

CNetworkAddress::CNetworkAddress(const CString& hostname, UInt16 port)
{
	if (port == 0)
		throw XSocketAddress(XSocketAddress::kBadPort, hostname, port);

	struct hostent* hent = CNetwork::gethostbyname(hostname.c_str());
	if (hent == NULL) {
		switch (CNetwork::gethosterror()) {
		case CNetwork::kHOST_NOT_FOUND:
			throw XSocketAddress(XSocketAddress::kNotFound, hostname, port);

		case CNetwork::kNO_DATA:
			throw XSocketAddress(XSocketAddress::kNoAddress, hostname, port);

		case CNetwork::kNO_RECOVERY:
		case CNetwork::kTRY_AGAIN:
		default:
			throw XSocketAddress(XSocketAddress::kUnknown, hostname, port);
		}
	}

	struct sockaddr_in* inetAddress = reinterpret_cast<struct sockaddr_in*>(&m_address);
	inetAddress->sin_family = hent->h_addrtype;
	inetAddress->sin_port   = CNetwork::swaphtons(port);
	memcpy(&inetAddress->sin_addr, hent->h_addr_list[0], hent->h_length);
	memset(inetAddress->sin_zero, 0, sizeof(inetAddress->sin_zero));
}

CNetworkAddress::~CNetworkAddress()
{
	// do nothing
}

const CNetwork::Address*	CNetworkAddress::getAddress() const
{
	return &m_address;
}
					
CNetwork::AddressLength		CNetworkAddress::getAddressLength() const
{
	return sizeof(m_address);
}
