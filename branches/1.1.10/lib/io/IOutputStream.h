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

#ifndef IOUTPUTSTREAM_H
#define IOUTPUTSTREAM_H

#include "IInterface.h"
#include "BasicTypes.h"

//! Output stream interface
/*!
Defines the interface for all output streams.
*/
class IOutputStream : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Close the stream
	/*!
	Closes the stream.  Attempting to write() after close() throws
	XIOClosed.
	*/
	virtual void		close() = 0;

	//! Write to stream
	/*!
	Write \c n bytes from \c buffer to the stream.  If this can't
	complete immeditely it will block.  If cancelled, an indeterminate
	amount of data may have been written.

	(cancellation point)
	*/
	virtual UInt32		write(const void* buffer, UInt32 n) = 0;

	//! Flush the stream
	/*!
	Waits until all buffered data has been written to the stream.

	(cancellation point)
	*/
	virtual void		flush() = 0;

	//@}
};

#endif
