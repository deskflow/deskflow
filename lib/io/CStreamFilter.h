/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#ifndef CSTREAMFILTER_H
#define CSTREAMFILTER_H

#include "IStream.h"

//! A stream filter
/*!
This class wraps a stream.  Subclasses provide indirect access
to the wrapped stream, typically performing some filtering.
*/
class CStreamFilter : public IStream {
public:
	/*!
	Create a wrapper around \c stream.  Iff \c adoptStream is true then
	this object takes ownership of the stream and will delete it in the
	d'tor.
	*/
	CStreamFilter(IStream* stream, bool adoptStream = true);
	~CStreamFilter();

	// IStream overrides
	// These all just forward to the underlying stream.  Override as
	// necessary.
	virtual void		close();
	virtual UInt32		read(void* buffer, UInt32 n);
	virtual void		write(const void* buffer, UInt32 n);
	virtual void		flush();
	virtual void		shutdownInput();
	virtual void		shutdownOutput();
	virtual void		setEventFilter(IEventJob* filter);
	virtual void*		getEventTarget() const;
	virtual bool		isReady() const;
	virtual UInt32		getSize() const;
	virtual IEventJob*	getEventFilter() const;

protected:
	//! Get the stream
	/*!
	Returns the stream passed to the c'tor.
	*/
	IStream*			getStream() const;

private:
	IStream*			m_stream;
	bool				m_adopted;
};

#endif
