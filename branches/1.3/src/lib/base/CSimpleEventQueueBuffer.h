/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CSIMPLEEVENTQUEUEBUFFER_H
#define CSIMPLEEVENTQUEUEBUFFER_H

#include "IEventQueueBuffer.h"
#include "IArchMultithread.h"
#include "stddeque.h"

//! In-memory event queue buffer
/*!
An event queue buffer provides a queue of events for an IEventQueue.
*/
class CSimpleEventQueueBuffer : public IEventQueueBuffer {
public:
	CSimpleEventQueueBuffer();	
	~CSimpleEventQueueBuffer();

	// IEventQueueBuffer overrides
	virtual void		waitForEvent(double timeout);
	virtual Type		getEvent(CEvent& event, UInt32& dataID);
	virtual bool		addEvent(UInt32 dataID);
	virtual bool		isEmpty() const;
	virtual CEventQueueTimer*
						newTimer(double duration, bool oneShot) const;
	virtual void		deleteTimer(CEventQueueTimer*) const;

private:
	typedef std::deque<UInt32> CEventDeque;

	CArchMutex			m_queueMutex;
	CArchCond			m_queueReadyCond;
	bool				m_queueReady;
	CEventDeque			m_queue;
};

#endif
