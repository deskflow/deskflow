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
