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
#include "IEventQueue.h"
#include "TMethodEventJob.h"

//
// CStreamFilter
//

CStreamFilter::CStreamFilter(IStream* stream, bool adoptStream) :
	m_stream(stream),
	m_adopted(adoptStream)
{
	// replace handlers for m_stream
	EVENTQUEUE->removeHandlers(m_stream->getEventTarget());
	EVENTQUEUE->adoptHandler(CEvent::kUnknown, m_stream->getEventTarget(),
							new TMethodEventJob<CStreamFilter>(this,
								&CStreamFilter::handleUpstreamEvent));
}

CStreamFilter::~CStreamFilter()
{
	EVENTQUEUE->removeHandler(CEvent::kUnknown, m_stream->getEventTarget());
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

void*
CStreamFilter::getEventTarget() const
{
	return const_cast<void*>(reinterpret_cast<const void*>(this));
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

IStream*
CStreamFilter::getStream() const
{
	return m_stream;
}

void
CStreamFilter::filterEvent(const CEvent& event)
{
	EVENTQUEUE->dispatchEvent(CEvent(event.getType(),
						getEventTarget(), event.getData()));
}

void
CStreamFilter::handleUpstreamEvent(const CEvent& event, void*)
{
	filterEvent(event);
}
