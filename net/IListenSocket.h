#ifndef ILISTENSOCKET_H
#define ILISTENSOCKET_H

#include "IInterface.h"
#include "XIO.h"
#include "XSocket.h"

class CNetworkAddress;
class ISocket;

class IListenSocket : public IInterface {
  public:
	// manipulators

	// bind the socket to a particular address
	virtual void		bind(const CNetworkAddress&) throw(XSocket) = 0;

	// wait for a connection
	virtual ISocket*	accept() throw(XSocket) = 0;
						
	// close the socket
	virtual void		close() throw(XIO) = 0;

	// accessors
};

#endif
