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

#ifndef IDATASOCKET_H
#define IDATASOCKET_H

#include "ISocket.h"

class IInputStream;
class IOutputStream;

//! Data stream socket interface
/*!
This interface defines the methods common to all network sockets that
represent a full-duplex data stream.
*/
class IDataSocket : public ISocket {
public:
	//! @name manipulators
	//@{

	//! Connect socket
	/*!
	Attempt to connect to a remote endpoint.  This waits until the
	connection is established or fails.  If it fails it throws an
	XSocketConnect exception.

	(cancellation point)
	*/
	virtual void		connect(const CNetworkAddress&) = 0;

	//! Get input stream
	/*!
	Returns the input stream for reading from the socket.  Closing this
	stream will shutdown the socket for reading.
	*/
	virtual IInputStream*	getInputStream() = 0;
	//! Get output stream
	/*!
	Returns the output stream for writing to the socket.  Closing this
	stream will shutdown the socket for writing.
	*/
	virtual IOutputStream*	getOutputStream() = 0;

	//@}

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&) = 0;
	virtual void		close() = 0;
};

#endif
