#ifndef ISOCKET_H
#define ISOCKET_H

#include "IInterface.h"

class CNetworkAddress;

class ISocket : public IInterface {
public:
	// manipulators

	// bind the socket to a particular address
	virtual void		bind(const CNetworkAddress&) = 0;

	// close the socket.  this will flush the output stream if it
	// hasn't been closed yet.
	virtual void		close() = 0;

	// accessors
};

#endif
