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
	//! @name accessors
	//@{

	//! Get connected event type
	/*!
	Returns the socket connected event type.  A socket sends this
	event when a remote connection has been established.
	*/
	static CEvent::Type	getConnectedEvent();

	//! Get connection failed event type
	/*!
	Returns the socket connection failed event type.  A socket sends
	this event when an attempt to connect to a remote port has failed.
	*/
	static CEvent::Type	getConnectionFailedEvent();

	//! Get input event type
	/*!
	Returns the socket input event type.  A socket sends this
	event when data is available to read from the input stream.
	*/
	static CEvent::Type	getInputEvent();

	//! Get shutdown input event type
	/*!
	Returns the socket shutdown input event type.  A socket sends this
	event when the remote side of the connection has shutdown for
	writing and there is no more data to read from the socket.
	*/
	static CEvent::Type	getShutdownInputEvent();

	//! Get shutdown input event type
	/*!
	Returns the socket shutdown input event type.  A socket sends this
	event when the remote side of the connection has shutdown for
	writing and there is no more data to read from the socket.
	*/
	static CEvent::Type	getShutdownOutputEvent();

	//@}

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&) = 0;
	virtual void		close() = 0;
	virtual void		setEventTarget(void*) = 0;

private:
	static CEvent::Type	s_connectedEvent;
	static CEvent::Type	s_failedEvent;
	static CEvent::Type	s_inputEvent;
	static CEvent::Type	s_shutdownInputEvent;
	static CEvent::Type	s_shutdownOutputEvent;
};

#endif
