#ifndef IDATASOCKET_H
#define IDATASOCKET_H

#include "ISocket.h"

class IInputStream;
class IOutputStream;

class IDataSocket : public ISocket {
public:
	// manipulators

	// connect the socket
	virtual void		connect(const CNetworkAddress&) = 0;

	// get the input and output streams for the socket.  closing
	// these streams closes the appropriate half of the socket.
	virtual IInputStream*	getInputStream() = 0;
	virtual IOutputStream*	getOutputStream() = 0;

	// accessors

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&) = 0;
	virtual void		close() = 0;
};

#endif
