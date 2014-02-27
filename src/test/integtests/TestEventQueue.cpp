/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#include "CTestEventQueue.h"
#include "CLog.h"
#include "TMethodEventJob.h"
#include "CSimpleEventQueueBuffer.h"
#include <stdexcept>

void
CTestEventQueue::raiseQuitEvent() 
{
	addEvent(CEvent(CEvent::kQuit));
}

void
CTestEventQueue::initQuitTimeout(double timeout)
{
	assert(m_quitTimeoutTimer == nullptr);
	m_quitTimeoutTimer = newOneShotTimer(timeout, NULL);
	adoptHandler(CEvent::kTimer, m_quitTimeoutTimer,
		new TMethodEventJob<CTestEventQueue>(
		this, &CTestEventQueue::handleQuitTimeout));
}

void
CTestEventQueue::cleanupQuitTimeout()
{
	removeHandler(CEvent::kTimer, m_quitTimeoutTimer);
	delete m_quitTimeoutTimer;
	m_quitTimeoutTimer = nullptr;
}

void
CTestEventQueue::handleQuitTimeout(const CEvent&, void* vclient)
{
	throw std::runtime_error("test event queue timeout");
}
