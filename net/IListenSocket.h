#ifndef ILISTENSOCKET_H
#define ILISTENSOCKET_H

#include "ISocket.h"

class IDataSocket;

class IListenSocket : public ISocket {
public:
	// manipulators

	// wait for a connection
	virtual IDataSocket*	accept() = 0;
						
	// accessors

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&) = 0;
	virtual void		close() = 0;
};

#endif
