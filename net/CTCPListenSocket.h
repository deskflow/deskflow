#ifndef CTCPLISTENSOCKET_H
#define CTCPLISTENSOCKET_H

#include "IListenSocket.h"

class CTCPListenSocket : public IListenSocket {
  public:
	CTCPListenSocket();
	~CTCPListenSocket();

	// manipulators

	// accessors

	// IListenSocket overrides
	virtual void		bind(const CNetworkAddress&) throw(XSocket);
	virtual ISocket*	accept() throw(XSocket);
	virtual void		close() throw(XIO);

  private:
	int					m_fd;
};

#endif
