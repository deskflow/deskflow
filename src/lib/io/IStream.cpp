/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "IStream.h"
#include "CEventQueue.h"

using namespace synergy;

//
// IStream
//

CEvent::Type			IStream::s_inputReadyEvent     = CEvent::kUnknown;
CEvent::Type			IStream::s_outputFlushedEvent  = CEvent::kUnknown;
CEvent::Type			IStream::s_outputErrorEvent    = CEvent::kUnknown;
CEvent::Type			IStream::s_inputShutdownEvent  = CEvent::kUnknown;
CEvent::Type			IStream::s_outputShutdownEvent = CEvent::kUnknown;

CEvent::Type
IStream::getInputReadyEvent()
{
	return m_eventQueue->registerTypeOnce(s_inputReadyEvent,
							"IStream::inputReady");
}

CEvent::Type
IStream::getOutputFlushedEvent()
{
	return m_eventQueue->registerTypeOnce(s_outputFlushedEvent,
							"IStream::outputFlushed");
}

CEvent::Type
IStream::getOutputErrorEvent()
{
	return m_eventQueue->registerTypeOnce(s_outputErrorEvent,
							"IStream::outputError");
}

CEvent::Type
IStream::getInputShutdownEvent()
{
	return m_eventQueue->registerTypeOnce(s_inputShutdownEvent,
							"IStream::inputShutdown");
}

CEvent::Type
IStream::getOutputShutdownEvent()
{
	return m_eventQueue->registerTypeOnce(s_outputShutdownEvent,
							"IStream::outputShutdown");
}

IEventQueue&
IStream::getEventQueue() const
{
	assert(m_eventQueue != NULL);
	return *m_eventQueue;
}
