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

#ifndef CINPUTSTREAMFILTER_H
#define CINPUTSTREAMFILTER_H

#include "IInputStream.h"

//! A filtering input stream
/*!
This class wraps an input stream.  Subclasses provide indirect access
to the stream, typically performing some filtering.
*/
class CInputStreamFilter : public IInputStream {
public:
	/*!
	Create a wrapper around \c stream.  Iff \c adoptStream is true then
	this object takes ownership of the stream and will delete it in the
	d'tor.
	*/
	CInputStreamFilter(IInputStream* stream, bool adoptStream = true);
	~CInputStreamFilter();

	// IInputStream overrides
	virtual void		close() = 0;
	virtual UInt32		read(void*, UInt32 n, double timeout) = 0;
	virtual UInt32		getSize() const = 0;

protected:
	//! Get the stream
	/*!
	Returns the stream passed to the c'tor.
	*/
	IInputStream*		getStream() const;

private:
	IInputStream*		m_stream;
	bool				m_adopted;
};

#endif
