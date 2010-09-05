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

#ifndef CBUFFEREDINPUTSTREAM_H
#define CBUFFEREDINPUTSTREAM_H

#include "IInputStream.h"
#include "CStreamBuffer.h"
#include "CCondVar.h"

class CMutex;
class IJob;

//! Memory buffer input stream
/*!
This class provides an input stream that reads from a memory buffer.
It also provides a means for the owner to ensure thread safe access.
Typically, an owner object will make this object visible to clients
that need access to an IInputStream while using the CBufferedInputStream
methods to write data to the stream.
*/
class CBufferedInputStream : public IInputStream {
public:
	/*!
	The \c mutex must not be NULL and will be used to ensure thread
	safe access.  If \c adoptedCloseCB is not NULL it will be called
	when close() is called, allowing the creator to detect the close.
	*/
	CBufferedInputStream(CMutex* mutex, IJob* adoptedCloseCB);
	~CBufferedInputStream();

	//! @name manipulators
	//@{

	//! Write data to stream
	/*!
	Write \c n bytes from \c buffer to the stream.  The mutex must
	be locked before calling this.
	*/
	void				write(const void* buffer, UInt32 n);

	//! Hangup stream
	/*!
	Causes read() to always return immediately.  If there is no
	more data to read then it returns 0.  Further writes are discarded.
	The mutex must be locked before calling this.
	*/
	void				hangup();

	//! Read from stream
	/*!
	This is the same as read() but the mutex must be locked before
	calling this.
	*/
	UInt32				readNoLock(void*, UInt32 n, double timeout);

	//@}
	//! @name accessors
	//@{

	//! Get remaining size of stream
	/*!
	This is the same as getSize() but the mutex must be locked before
	calling this.
	*/
	UInt32				getSizeNoLock() const;

	//@}

	// IInputStream overrides
	// these all lock the mutex for their duration
	virtual void		close();
	virtual UInt32		read(void*, UInt32 n, double timeout);
	virtual UInt32		getSize() const;

private:
	CMutex*				m_mutex;
	CCondVar<bool>		m_empty;
	IJob*				m_closeCB;
	CStreamBuffer		m_buffer;
	bool				m_closed;
	bool				m_hungup;
};

#endif
