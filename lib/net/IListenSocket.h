/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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
