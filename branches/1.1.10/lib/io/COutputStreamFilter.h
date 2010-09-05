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

#ifndef COUTPUTSTREAMFILTER_H
#define COUTPUTSTREAMFILTER_H

#include "IOutputStream.h"

//! A filtering output stream
/*!
This class wraps an output stream.  Subclasses provide indirect access
to the stream, typically performing some filtering.
*/
class COutputStreamFilter : public IOutputStream {
public:
	/*!
	Create a wrapper around \c stream.  Iff \c adoptStream is true then
	this object takes ownership of the stream and will delete it in the
	d'tor.
	*/
	COutputStreamFilter(IOutputStream* stream, bool adoptStream = true);
	~COutputStreamFilter();

	// IOutputStream overrides
	virtual void		close() = 0;
	virtual UInt32		write(const void*, UInt32 count) = 0;
	virtual void		flush() = 0;

protected:
	//! Get the stream
	/*!
	Returns the stream passed to the c'tor.
	*/
	IOutputStream*		getStream() const;

private:
	IOutputStream*		m_stream;
	bool				m_adopted;
};

#endif
