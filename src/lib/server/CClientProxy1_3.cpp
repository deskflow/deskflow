/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2006 Chris Schoeneman
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

#include "CClientProxy1_3.h"
#include "CProtocolUtil.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include <cstring>
#include <memory>

//
// CClientProxy1_3
//

CClientProxy1_3::CClientProxy1_3(const CString& name, synergy::IStream* stream, IEventQueue* eventQueue) :
	CClientProxy1_2(name, stream, eventQueue),
	m_keepAliveRate(kKeepAliveRate),
	m_keepAliveTimer(NULL)
{
	setHeartbeatRate(kKeepAliveRate, kKeepAliveRate * kKeepAlivesUntilDeath);
}

CClientProxy1_3::~CClientProxy1_3()
{
	// cannot do this in superclass or our override wouldn't get called
	removeHeartbeatTimer();
}

void
CClientProxy1_3::mouseWheel(SInt32 xDelta, SInt32 yDelta)
{
	LOG((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d,%+d", getName().c_str(), xDelta, yDelta));
	CProtocolUtil::writef(getStream(), kMsgDMouseWheel, xDelta, yDelta);
}

bool
CClientProxy1_3::parseMessage(const UInt8* code)
{
	// process message
	if (memcmp(code, kMsgCKeepAlive, 4) == 0) {
		// reset alarm
		resetHeartbeatTimer();
		return true;
	}
	else {
		return CClientProxy1_2::parseMessage(code);
	}
}

void
CClientProxy1_3::resetHeartbeatRate()
{
	setHeartbeatRate(kKeepAliveRate, kKeepAliveRate * kKeepAlivesUntilDeath);
}

void
CClientProxy1_3::setHeartbeatRate(double rate, double)
{
	m_keepAliveRate = rate;
	CClientProxy1_2::setHeartbeatRate(rate, rate * kKeepAlivesUntilDeath);
}

void
CClientProxy1_3::resetHeartbeatTimer()
{
	// reset the alarm but not the keep alive timer
	CClientProxy1_2::removeHeartbeatTimer();
	CClientProxy1_2::addHeartbeatTimer();
}

void
CClientProxy1_3::addHeartbeatTimer()
{
	// create and install a timer to periodically send keep alives
	if (m_keepAliveRate > 0.0) {
		m_keepAliveTimer = EVENTQUEUE->newTimer(m_keepAliveRate, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, m_keepAliveTimer,
							new TMethodEventJob<CClientProxy1_3>(this,
								&CClientProxy1_3::handleKeepAlive, NULL));
	}

	// superclass does the alarm
	CClientProxy1_2::addHeartbeatTimer();
}

void
CClientProxy1_3::removeHeartbeatTimer()
{
	// remove the timer that sends keep alives periodically
	if (m_keepAliveTimer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_keepAliveTimer);
		EVENTQUEUE->deleteTimer(m_keepAliveTimer);
		m_keepAliveTimer = NULL;
	}

	// superclass does the alarm
	CClientProxy1_2::removeHeartbeatTimer();
}

void
CClientProxy1_3::handleKeepAlive(const CEvent&, void*)
{
	CProtocolUtil::writef(getStream(), kMsgCKeepAlive);
}
