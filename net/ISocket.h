#ifndef ISOCKET_H
#define ISOCKET_H

#include "IInterface.h"
#include "BasicTypes.h"
#include "XSocket.h"
#include "XIO.h"

class CNetworkAddress;
class IInputStream;
class IOutputStream;

class ISocket : public IInterface {
public:
	// manipulators

	// bind the socket to a particular address
	virtual void		bind(const CNetworkAddress&) = 0;

	// connect the socket
	virtual void		connect(const CNetworkAddress&) = 0;

	// close the socket.  this will flush the output stream if it
	// hasn't been closed yet.
	virtual void		close() = 0;

	// get the input and output streams for the socket.  closing
	// these streams closes the appropriate half of the socket.
	virtual IInputStream*	getInputStream() = 0;
	virtual IOutputStream*	getOutputStream() = 0;

	// accessors
};

#endif
