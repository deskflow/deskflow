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

#ifndef IINPUTSTREAM_H
#define IINPUTSTREAM_H

#include "IInterface.h"
#include "BasicTypes.h"

//! Input stream interface
/*!
Defines the interface for all input streams.
*/
class IInputStream : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Close the stream
	/*!
	Closes the stream.  Attempting to read() after close() throws
	XIOClosed and getSize() always returns zero.
	*/
	virtual void		close() = 0;

	//! Read from stream
	/*!
	Read up to \c n bytes into buffer, returning the number read.
	Blocks for up to \c timeout seconds if no data is available but does
	not wait if any data is available, even if less than \c n bytes.
	If \c timeout < 0 then it blocks indefinitely until data is available.  
	If \c buffer is NULL then the data is discarded.  Returns (UInt32)-1 if
	it times out and 0 if no data is available and the other end of the
	stream has hungup.

	(cancellation point)
	*/
	virtual UInt32		read(void* buffer, UInt32 n, double timeout) = 0;

	//@}
	//! @name accessors
	//@{

	//! Get remaining size of stream
	/*!
	Returns a conservative estimate of the available bytes to read
	(i.e. a number not greater than the actual number of bytes).
	Some streams may not be able to determine this and will always
	return zero.
	*/
	virtual UInt32		getSize() const = 0;

	//@}
};

#endif
