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

#ifndef CBUFFEREDOUTPUTSTREAM_H
#define CBUFFEREDOUTPUTSTREAM_H

#include "IOutputStream.h"
#include "CStreamBuffer.h"
#include "CCondVar.h"

class CMutex;
class IJob;

//! Memory buffer output stream
/*!
This class provides an output stream that writes to a memory buffer.
It also provides a means for the owner to ensure thread safe access.
Typically, an owner object will make this object visible to clients
that need access to an IOutputStream while using the CBufferedOutputStream
methods to read the data written to the stream.
*/
class CBufferedOutputStream : public IOutputStream {
public:
	/*!
	The \c mutex must not be NULL and will be used to ensure thread
	safe access.  If \c adoptedCloseCB is not NULL it will be called
	when close() is called, allowing the creator to detect the close.
	*/
	CBufferedOutputStream(CMutex* mutex, IJob* adoptedCloseCB);
	~CBufferedOutputStream();

	//! @name manipulators
	//@{

	//! Read data without removing from buffer
	/*!
	Returns a buffer of \c n bytes (which must be <= getSize()).  The
	caller must not modify the buffer nor delete it.  The mutex must
	be locked before calling this.
	*/
	const void*			peek(UInt32 n);

	//! Discard data
	/*!
	Discards the next \c n bytes.  If \c n >= getSize() then the buffer
	is cleared.  The mutex must be locked before calling this.
	*/
	void				pop(UInt32 n);

	//@}
	//! @name accessors
	//@{

	//! Get size of buffer
	/*!
	Returns the number of bytes in the buffer.  The mutex must be locked
	before calling this.
	*/
	UInt32				getSize() const;

	//@}

	// IOutputStream overrides
	// these all lock the mutex for their duration
	virtual void		close();
	virtual UInt32		write(const void*, UInt32 n);
	virtual void		flush();

private:
	CMutex*				m_mutex;
	IJob*				m_closeCB;
	CCondVar<bool>		m_empty;
	CStreamBuffer		m_buffer;
	bool				m_closed;
};

#endif
