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

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&);
	virtual void		close();

	// IListenSocket overrides
	virtual IDataSocket*	accept();

private:
	CNetwork::Socket	m_fd;
};

#endif
