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

#include "CSimpleEventQueueBuffer.h"
#include "CStopwatch.h"
#include "CArch.h"

class CEventQueueTimer { };

//
// CSimpleEventQueueBuffer
//

CSimpleEventQueueBuffer::CSimpleEventQueueBuffer()
{
	m_queueMutex     = ARCH->newMutex();
	m_queueReadyCond = ARCH->newCondVar();
	m_queueReady     = false;
}

CSimpleEventQueueBuffer::~CSimpleEventQueueBuffer()
{
	ARCH->closeCondVar(m_queueReadyCond);
	ARCH->closeMutex(m_queueMutex);
}

void
CSimpleEventQueueBuffer::waitForEvent(double timeout)
{
	CArchMutexLock lock(m_queueMutex);
	CStopwatch timer(true);
	while (!m_queueReady) {
		double timeLeft = timeout;
		if (timeLeft >= 0.0) {
			timeLeft -= timer.getTime();
			if (timeLeft < 0.0) {
				return;
			}
		}
		ARCH->waitCondVar(m_queueReadyCond, m_queueMutex, timeLeft);
	}
}

IEventQueueBuffer::Type
CSimpleEventQueueBuffer::getEvent(CEvent&, UInt32& dataID)
{
	CArchMutexLock lock(m_queueMutex);
	if (!m_queueReady) {
		return kNone;
	}
	dataID = m_queue.back();
	m_queue.pop_back();
	m_queueReady = !m_queue.empty();
	return kUser;
}

bool
CSimpleEventQueueBuffer::addEvent(UInt32 dataID)
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
CSimpleEventQueueBuffer::isEmpty() const
{
	CArchMutexLock lock(m_queueMutex);
	return !m_queueReady;
}

CEventQueueTimer*
CSimpleEventQueueBuffer::newTimer(double, bool) const
{
	return new CEventQueueTimer;
}

void
CSimpleEventQueueBuffer::deleteTimer(CEventQueueTimer* timer) const
{
	delete timer;
}
