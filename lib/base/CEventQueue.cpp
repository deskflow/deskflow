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

#include "CEventQueue.h"
#include "IEventJob.h"
#include "CArch.h"

//
// CEventQueue
//

static int				g_systemTarget = 0;
CEventQueue*			CEventQueue::s_instance = NULL;

CEventQueue::CEventQueue()
{
	assert(s_instance == NULL);
	s_instance = this;
	m_mutex    = ARCH->newMutex();
}

CEventQueue::~CEventQueue()
{
	ARCH->closeMutex(m_mutex);
	s_instance = NULL;
}

void*
CEventQueue::getSystemTarget()
{
	// any unique arbitrary pointer will do
	return &g_systemTarget;
}

CEventQueue*
CEventQueue::getInstance()
{
	return s_instance;
}

bool
CEventQueue::getEvent(CEvent& event, double timeout)
{
	// if no events are waiting then handle timers and then wait
	if (doIsEmpty()) {
		// handle timers first
		if (hasTimerExpired(event)) {
			return true;
		}

		// get time until next timer expires.  if there is a timer
		// and it'll expire before the client's timeout then use
		// that duration for our timeout instead.
		double timerTimeout = getNextTimerTimeout();
		if (timerTimeout >= 0.0 && timerTimeout < timeout) {
			timeout = timerTimeout;
		}

		// wait for an event
		waitForEvent(timeout);
	}

	// if no events are pending then do the timers
	if (doIsEmpty()) {
		return hasTimerExpired(event);
	}

	return doGetEvent(event);
}

bool
CEventQueue::dispatchEvent(const CEvent& event)
{
	void* target   = event.getTarget();
	IEventJob* job = getHandler(target);
	if (job != NULL) {
		job->run(event);
		return true;
	}
	return false;
}

void
CEventQueue::addEvent(const CEvent& event)
{
	// discard bogus event types
	switch (event.getType()) {
	case CEvent::kUnknown:
	case CEvent::kSystem:
	case CEvent::kTimer:
		return;

	default:
		break;
	}

	// store the event's data locally
	UInt32 eventID = saveEvent(event);

	// add it
	if (!doAddEvent(eventID)) {
		// failed to send event
		removeEvent(eventID);
		CEvent::deleteData(event);
	}
}

CEventQueueTimer*
CEventQueue::newTimer(double duration, void* target)
{
	assert(duration > 0.0);

	CEventQueueTimer* timer = doNewTimer(duration, false);
	CArchMutexLock lock(m_mutex);
	m_timers.insert(timer);
	m_timerQueue.push(CTimer(timer, duration, target, false));
	return timer;
}

CEventQueueTimer*
CEventQueue::newOneShotTimer(double duration, void* target)
{
	assert(duration > 0.0);

	CEventQueueTimer* timer = doNewTimer(duration, true);
	CArchMutexLock lock(m_mutex);
	m_timers.insert(timer);
	m_timerQueue.push(CTimer(timer, duration, target, true));
	return timer;
}

void
CEventQueue::deleteTimer(CEventQueueTimer* timer)
{
	{
		CArchMutexLock lock(m_mutex);
		for (CTimerQueue::iterator index = m_timerQueue.begin();
								index != m_timerQueue.end(); ++index) {
			if (index->getTimer() == timer) {
				m_timerQueue.erase(index);
				break;
			}
		}
		CTimers::iterator index = m_timers.find(timer);
		if (index != m_timers.end()) {
			m_timers.erase(index);
		}
	}
	doDeleteTimer(timer);
}

void
CEventQueue::adoptHandler(void* target, IEventJob* handler)
{
	CArchMutexLock lock(m_mutex);
	IEventJob*& job = m_handlers[target];
	delete job;
	job = handler;
}

IEventJob*
CEventQueue::orphanHandler(void* target)
{
	CArchMutexLock lock(m_mutex);
	CHandlerTable::iterator index = m_handlers.find(target);
	if (index != m_handlers.end()) {
		IEventJob* handler = index->second;
		m_handlers.erase(index);
		return handler;
	}
	else {
		return NULL;
	}
}

bool
CEventQueue::isEmpty() const
{
	return (doIsEmpty() && getNextTimerTimeout() != 0.0);
}

IEventJob*
CEventQueue::getHandler(void* target) const
{
	CArchMutexLock lock(m_mutex);
	CHandlerTable::const_iterator index = m_handlers.find(target);
	if (index != m_handlers.end()) {
		return index->second;
	}
	else {
		return NULL;
	}
}

UInt32
CEventQueue::saveEvent(const CEvent& event)
{
	CArchMutexLock lock(m_mutex);

	// choose id
	UInt32 id;
	if (!m_oldEventIDs.empty()) {
		// reuse an id
		id = m_oldEventIDs.back();
		m_oldEventIDs.pop_back();
	}
	else {
		// make a new id
		id = static_cast<UInt32>(m_oldEventIDs.size());
	}

	// save data
	m_events[id] = event;
	return id;
}

CEvent
CEventQueue::removeEvent(UInt32 eventID)
{
	CArchMutexLock lock(m_mutex);

	// look up id
	CEventTable::iterator index = m_events.find(eventID);
	if (index == m_events.end()) {
		return CEvent();
	}

	// get data
	CEvent event = index->second;
	m_events.erase(index);

	// save old id for reuse
	m_oldEventIDs.push_back(eventID);

	return event;
}

bool
CEventQueue::hasTimerExpired(CEvent& event)
{
	CArchMutexLock lock(m_mutex);

	// return true if there's a timer in the timer priority queue that
	// has expired.  if returning true then fill in event appropriately
	// and reset and reinsert the timer.
	if (m_timerQueue.empty()) {
		return false;
	}

	// get time elapsed since last check
	const double time = m_time.getTime();
	m_time.reset();

	// countdown elapsed time
	for (CTimerQueue::iterator index = m_timerQueue.begin();
							index != m_timerQueue.end(); ++index) {
		(*index) -= time;
	}

	// done if no timers are expired
	if (m_timerQueue.top() > 0.0) {
		return false;
	}

	// remove timer from queue
	CTimer timer = m_timerQueue.top();
	m_timerQueue.pop();

	// prepare event and reset the timer's clock
	timer.fillEvent(m_timerEvent);
	event = CEvent(CEvent::kTimer, timer.getTarget(), &m_timerEvent);
	timer.reset();

	// reinsert timer into queue if it's not a one-shot
	if (!timer.isOneShot()) {
		m_timerQueue.push(timer);
	}

	return true;
}

double
CEventQueue::getNextTimerTimeout() const
{
	CArchMutexLock lock(m_mutex);

	// return -1 if no timers, 0 if the top timer has expired, otherwise
	// the time until the top timer in the timer priority queue will
	// expire.
	if (m_timerQueue.empty()) {
		return -1.0;
	}
	if (m_timerQueue.top() <= 0.0) {
		return 0.0;
	}
	return m_timerQueue.top();
}


//
// CXWindowsScreen::CTimer
//

CEventQueue::CTimer::CTimer(CEventQueueTimer* timer,
				double timeout, void* target, bool oneShot) :
	m_timer(timer),
	m_timeout(timeout),
	m_target(target),
	m_oneShot(oneShot),
	m_time(timeout)
{
	assert(m_timeout > 0.0);
}

CEventQueue::CTimer::~CTimer()
{
	// do nothing
}

void
CEventQueue::CTimer::reset()
{
	m_time = m_timeout;
}

CEventQueue::CTimer::CTimer&
CEventQueue::CTimer::operator-=(double dt)
{
	m_time -= dt;
	return *this;
}

CEventQueue::CTimer::operator double() const
{
	return m_time;
}

bool
CEventQueue::CTimer::isOneShot() const
{
	return m_oneShot;
}

CEventQueueTimer*
CEventQueue::CTimer::getTimer() const
{
	return m_timer;
}

void*
CEventQueue::CTimer::getTarget() const
{
	return m_target;
}

void
CEventQueue::CTimer::fillEvent(CTimerEvent& event) const
{
	event.m_timer = m_timer;
	event.m_count = 0;
	if (m_time <= 0.0) {
		event.m_count = static_cast<UInt32>((m_timeout - m_time) / m_timeout);
	}
}

bool
CEventQueue::CTimer::operator<(const CTimer& t) const
{
	return m_time < t.m_time;
}
