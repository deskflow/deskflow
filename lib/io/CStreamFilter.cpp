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

#include "CStreamFilter.h"

//
// CStreamFilter
//

CStreamFilter::CStreamFilter(IStream* stream, bool adoptStream) :
	m_stream(stream),
	m_adopted(adoptStream)
{
	// do nothing
}

CStreamFilter::~CStreamFilter()
{
	if (m_adopted) {
		delete m_stream;
	}
}

void
CStreamFilter::close()
{
	getStream()->close();
}

UInt32
CStreamFilter::read(void* buffer, UInt32 n)
{
	return getStream()->read(buffer, n);
}

void
CStreamFilter::write(const void* buffer, UInt32 n)
{
	getStream()->write(buffer, n);
}

void
CStreamFilter::flush()
{
	getStream()->flush();
}

void
CStreamFilter::shutdownInput()
{
	getStream()->shutdownInput();
}

void
CStreamFilter::shutdownOutput()
{
	getStream()->shutdownOutput();
}

void
CStreamFilter::setEventFilter(IEventJob* filter)
{
	getStream()->setEventFilter(filter);
}

void*
CStreamFilter::getEventTarget() const
{
	return getStream()->getEventTarget();
}

bool
CStreamFilter::isReady() const
{
	return getStream()->isReady();
}

UInt32
CStreamFilter::getSize() const
{
	return getStream()->getSize();
}

IEventJob*
CStreamFilter::getEventFilter() const
{
	return getStream()->getEventFilter();
}

IStream*
CStreamFilter::getStream() const
{
	return m_stream;
}
