#ifndef CTCPLISTENSOCKET_H
#define CTCPLISTENSOCKET_H

#include "IListenSocket.h"
#include "CNetwork.h"

class CTCPListenSocket : public IListenSocket {
public:
	CTCPListenSocket();
	~CTCPListenSocket();

	// manipulators

	// accessors

	// IListenSocket overrides
	virtual void		bind(const CNetworkAddress&);
	virtual ISocket*	accept();
	virtual void		close();

private:
	CNetwork::Socket	m_fd;
};

#endif
