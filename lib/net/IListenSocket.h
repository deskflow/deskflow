#ifndef ILISTENSOCKET_H
#define ILISTENSOCKET_H

#include "ISocket.h"

class IDataSocket;

//! Listen socket interface
/*!
This interface defines the methods common to all network sockets that
listen for incoming connections.
*/
class IListenSocket : public ISocket {
public:
	//! @name manipulators
	//@{

	//! Accept connection
	/*!
	Wait for and accept a connection, returning a socket representing
	the full-duplex data stream.

	(cancellation point)
	*/
	virtual IDataSocket*	accept() = 0;

	//@}

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&) = 0;
	virtual void		close() = 0;
};

#endif
