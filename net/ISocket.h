#ifndef ISOCKET_H
#define ISOCKET_H

#include "IInterface.h"

class CNetworkAddress;

//! Generic socket interface
/*!
This interface defines the methods common to all network sockets.
*/
class ISocket : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Bind socket to address
	/*!
	Binds the socket to a particular address.
	*/
	virtual void		bind(const CNetworkAddress&) = 0;

	//! Close socket
	/*!
	Closes the socket.  This should flush the output stream.
	*/
	virtual void		close() = 0;

	//@}
};

#endif
