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

#include "CSimpleEventQueue.h"
#include "CArch.h"

class CEventQueueTimer { };

//
// CSimpleEventQueue
//

CSimpleEventQueue::CSimpleEventQueue()
{
	m_queueMutex     = ARCH->newMutex();
	m_queueReadyCond = ARCH->newCondVar();
	m_queueReady     = false;
}

CSimpleEventQueue::~CSimpleEventQueue()
{
	ARCH->closeCondVar(m_queueReadyCond);
	ARCH->closeMutex(m_queueMutex);
}

void
CSimpleEventQueue::waitForEvent(double timeout)
{
	CArchMutexLock lock(m_queueMutex);
	while (!m_queueReady) {
		ARCH->waitCondVar(m_queueReadyCond, m_queueMutex, -1.0);
	}
}

bool
CSimpleEventQueue::doGetEvent(CEvent& event)
{
	CArchMutexLock lock(m_queueMutex);
	if (!m_queueReady) {
		return false;
	}
	event = removeEvent(m_queue.back());
	m_queue.pop_back();
	m_queueReady = !m_queue.empty();
	return true;
}

bool
CSimpleEventQueue::doAddEvent(UInt32 dataID)
{
	CArchMutexLock lock(m_queueMutex);
	m_queue.push_front(dataID);
	if (!m_queueReady) {
		m_queueReady = true;
		ARCH->broadcastCondVar(m_queueReadyCond);
	}
	return true;
}

bool
CSimpleEventQueue::doIsEmpty() const
{
	CArchMutexLock lock(m_queueMutex);
	return !m_queueReady;
}

CEventQueueTimer*
CSimpleEventQueue::doNewTimer(double, bool) const
{
	return new CEventQueueTimer;
}

void
CSimpleEventQueue::doDeleteTimer(CEventQueueTimer* timer) const
{
	delete timer;
}
