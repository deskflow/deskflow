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

#ifndef CSIMPLEEVENTQUEUE_H
#define CSIMPLEEVENTQUEUE_H

#include "CEventQueue.h"
#include "IArchMultithread.h"
#include "stddeque.h"

//! Event queue for added events only
/*!
An event queue that provides no system events, just events added by
addEvent().
*/
class CSimpleEventQueue : public CEventQueue {
public:
	CSimpleEventQueue();
	virtual ~CSimpleEventQueue();

	//! @name manipulators
	//@{

	//@}
	//! @name accessors
	//@{

	//@}

protected:
	// CEventQueue overrides
	virtual void		waitForEvent(double timeout);
	virtual bool		doGetEvent(CEvent& event);
	virtual bool		doAddEvent(UInt32 dataID);
	virtual bool		doIsEmpty() const;
	virtual CEventQueueTimer*
						doNewTimer(double duration, bool oneShot) const;
	virtual void		doDeleteTimer(CEventQueueTimer*) const;

private:
	typedef std::deque<UInt32> CEventDeque;

	CArchMutex			m_queueMutex;
	CArchCond			m_queueReadyCond;
	bool				m_queueReady;
	CEventDeque			m_queue;
};

#endif
