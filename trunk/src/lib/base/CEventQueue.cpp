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

#include "CEventQueue.h"
#include "CLog.h"
#include "CSimpleEventQueueBuffer.h"
#include "CStopwatch.h"
#include "IEventJob.h"
#include "CArch.h"
#include "CEventTypes.h"

EVENT_TYPE_ACCESSOR(CClient)
EVENT_TYPE_ACCESSOR(IStream)
EVENT_TYPE_ACCESSOR(CIpcClient)
EVENT_TYPE_ACCESSOR(CIpcClientProxy)
EVENT_TYPE_ACCESSOR(CIpcServer)
EVENT_TYPE_ACCESSOR(CIpcServerProxy)
EVENT_TYPE_ACCESSOR(IDataSocket)
EVENT_TYPE_ACCESSOR(IListenSocket)
EVENT_TYPE_ACCESSOR(ISocket)
EVENT_TYPE_ACCESSOR(COSXScreen)
EVENT_TYPE_ACCESSOR(CClientListener)
EVENT_TYPE_ACCESSOR(CClientProxy)
EVENT_TYPE_ACCESSOR(CClientProxyUnknown)
EVENT_TYPE_ACCESSOR(CServer)
EVENT_TYPE_ACCESSOR(CServerApp)
EVENT_TYPE_ACCESSOR(IKeyState)
EVENT_TYPE_ACCESSOR(IPrimaryScreen)
EVENT_TYPE_ACCESSOR(IScreen)

// interrupt handler.  this just adds a quit event to the queue.
static
void
interrupt(CArch::ESignal, void* data)
{
	CEventQueue* events = reinterpret_cast<CEventQueue*>(data);
	events->addEvent(CEvent(CEvent::kQuit));
}


//
// CEventQueue
//

CEventQueue::CEventQueue() :
	m_systemTarget(0),
	m_nextType(CEvent::kLast),
	m_typesForCClient(NULL),
	m_typesForIStream(NULL),
	m_typesForCIpcClient(NULL),
	m_typesForCIpcClientProxy(NULL),
	m_typesForCIpcServer(NULL),
	m_typesForCIpcServerProxy(NULL),
	m_typesForIDataSocket(NULL),
	m_typesForIListenSocket(NULL),
	m_typesForISocket(NULL),
	m_typesForCOSXScreen(NULL),
	m_typesForCClientListener(NULL),
	m_typesForCClientProxy(NULL),
	m_typesForCClientProxyUnknown(NULL),
	m_typesForCServer(NULL),
	m_typesForCServerApp(NULL),
	m_typesForIKeyState(NULL),
	m_typesForIPrimaryScreen(NULL),
	m_typesForIScreen(NULL)
{
	m_mutex = ARCH->newMutex();
	ARCH->setSignalHandler(CArch::kINTERRUPT, &interrupt, this);
	ARCH->setSignalHandler(CArch::kTERMINATE, &interrupt, this);
	m_buffer = new CSimpleEventQueueBuffer;
}

CEventQueue::~CEventQueue()
{
	delete m_buffer;
	ARCH->setSignalHandler(CArch::kINTERRUPT, NULL, NULL);
	ARCH->setSignalHandler(CArch::kTERMINATE, NULL, NULL);
	ARCH->closeMutex(m_mutex);
}

void
CEventQueue::loop()
{
	m_buffer->init();
	
	CEvent event;
	getEvent(event);
	while (event.getType() != CEvent::kQuit) {
		dispatchEvent(event);
		CEvent::deleteData(event);
		getEvent(event);
	}
}

CEvent::Type
CEventQueue::registerTypeOnce(CEvent::Type& type, const char* name)
{
	CArchMutexLock lock(m_mutex);
	if (type == CEvent::kUnknown) {
		m_typeMap.insert(std::make_pair(m_nextType, name));
		m_nameMap.insert(std::make_pair(name, m_nextType));
		LOG((CLOG_DEBUG1 "registered event type %s as %d", name, m_nextType));
		type = m_nextType++;
	}
	return type;
}

const char*
CEventQueue::getTypeName(CEvent::Type type)
{
	switch (type) {
	case CEvent::kUnknown:
		return "nil";

	case CEvent::kQuit:
		return "quit";

	case CEvent::kSystem:
		return "system";

	case CEvent::kTimer:
		return "timer";

	default:
		CTypeMap::const_iterator i = m_typeMap.find(type);
		if (i == m_typeMap.end()) {
			return "<unknown>";
		}
		else {
			return i->second;
		}
	}
}

void
CEventQueue::adoptBuffer(IEventQueueBuffer* buffer)
{
	CArchMutexLock lock(m_mutex);

	LOG((CLOG_DEBUG "adopting new buffer"));

	if (m_events.size() != 0) {
		// this can come as a nasty surprise to programmers expecting
		// their events to be raised, only to have them deleted.
		LOG((CLOG_DEBUG "discarding %d event(s)", m_events.size()));
	}

	// discard old buffer and old events
	delete m_buffer;
	for (CEventTable::iterator i = m_events.begin(); i != m_events.end(); ++i) {
		CEvent::deleteData(i->second);
	}
	m_events.clear();
	m_oldEventIDs.clear();

	// use new buffer
	m_buffer = buffer;
	if (m_buffer == NULL) {
		m_buffer = new CSimpleEventQueueBuffer;
	}
}

bool
CEventQueue::getEvent(CEvent& event, double timeout)
{
	CStopwatch timer(true);
retry:
	// if no events are waiting then handle timers and then wait
	while (m_buffer->isEmpty()) {
		// handle timers first
		if (hasTimerExpired(event)) {
			return true;
		}

		// get time remaining in timeout
		double timeLeft = timeout - timer.getTime();
		if (timeout >= 0.0 && timeLeft <= 0.0) {
			return false;
		}

		// get time until next timer expires.  if there is a timer
		// and it'll expire before the client's timeout then use
		// that duration for our timeout instead.
		double timerTimeout = getNextTimerTimeout();
		if (timeout < 0.0 || (timerTimeout >= 0.0 && timerTimeout < timeLeft)) {
			timeLeft = timerTimeout;
		}

		// wait for an event
		m_buffer->waitForEvent(timeLeft);
	}

	// get the event
	UInt32 dataID;
	IEventQueueBuffer::Type type = m_buffer->getEvent(event, dataID);
	switch (type) {
	case IEventQueueBuffer::kNone:
		if (timeout < 0.0 || timeout <= timer.getTime()) {
			// don't want to fail if client isn't expecting that
			// so if getEvent() fails with an infinite timeout
			// then just try getting another event.
			goto retry;
		}
		return false;

	case IEventQueueBuffer::kSystem:
		return true;

	case IEventQueueBuffer::kUser:
		{
			CArchMutexLock lock(m_mutex);
			event = removeEvent(dataID);
			return true;
		}

	default:
		assert(0 && "invalid event type");
		return false;
	}
}

bool
CEventQueue::dispatchEvent(const CEvent& event)
{
	void* target   = event.getTarget();
	IEventJob* job = getHandler(event.getType(), target);
	if (job == NULL) {
		job = getHandler(CEvent::kUnknown, target);
	}
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
	
	if ((event.getFlags() & CEvent::kDeliverImmediately) != 0) {
		dispatchEvent(event);
		CEvent::deleteData(event);
	}
	else {
		CArchMutexLock lock(m_mutex);
		
		// store the event's data locally
		UInt32 eventID = saveEvent(event);
		
		// add it
		if (!m_buffer->addEvent(eventID)) {
			// failed to send event
			removeEvent(eventID);
			CEvent::deleteData(event);
		}
	}
}

CEventQueueTimer*
CEventQueue::newTimer(double duration, void* target)
{
	assert(duration > 0.0);

	CEventQueueTimer* timer = m_buffer->newTimer(duration, false);
	if (target == NULL) {
		target = timer;
	}
	CArchMutexLock lock(m_mutex);
	m_timers.insert(timer);
	// initial duration is requested duration plus whatever's on
	// the clock currently because the latter will be subtracted
	// the next time we check for timers.
	m_timerQueue.push(CTimer(timer, duration,
							duration + m_time.getTime(), target, false));
	return timer;
}

CEventQueueTimer*
CEventQueue::newOneShotTimer(double duration, void* target)
{
	assert(duration > 0.0);

	CEventQueueTimer* timer = m_buffer->newTimer(duration, true);
	if (target == NULL) {
		target = timer;
	}
	CArchMutexLock lock(m_mutex);
	m_timers.insert(timer);
	// initial duration is requested duration plus whatever's on
	// the clock currently because the latter will be subtracted
	// the next time we check for timers.
	m_timerQueue.push(CTimer(timer, duration,
							duration + m_time.getTime(), target, true));
	return timer;
}

void
CEventQueue::deleteTimer(CEventQueueTimer* timer)
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
	m_buffer->deleteTimer(timer);
}

void
CEventQueue::adoptHandler(CEvent::Type type, void* target, IEventJob* handler)
{
	CArchMutexLock lock(m_mutex);
	IEventJob*& job = m_handlers[target][type];
	delete job;
	job = handler;
}

void
CEventQueue::removeHandler(CEvent::Type type, void* target)
{
	IEventJob* handler = NULL;
	{
		CArchMutexLock lock(m_mutex);
		CHandlerTable::iterator index = m_handlers.find(target);
		if (index != m_handlers.end()) {
			CTypeHandlerTable& typeHandlers = index->second;
			CTypeHandlerTable::iterator index2 = typeHandlers.find(type);
			if (index2 != typeHandlers.end()) {
				handler = index2->second;
				typeHandlers.erase(index2);
			}
		}
	}
	delete handler;
}

void
CEventQueue::removeHandlers(void* target)
{
	std::vector<IEventJob*> handlers;
	{
		CArchMutexLock lock(m_mutex);
		CHandlerTable::iterator index = m_handlers.find(target);
		if (index != m_handlers.end()) {
			// copy to handlers array and clear table for target
			CTypeHandlerTable& typeHandlers = index->second;
			for (CTypeHandlerTable::iterator index2 = typeHandlers.begin();
							index2 != typeHandlers.end(); ++index2) {
				handlers.push_back(index2->second);
			}
			typeHandlers.clear();
		}
	}

	// delete handlers
	for (std::vector<IEventJob*>::iterator index = handlers.begin();
							index != handlers.end(); ++index) {
		delete *index;
	}
}

bool
CEventQueue::isEmpty() const
{
	return (m_buffer->isEmpty() && getNextTimerTimeout() != 0.0);
}

IEventJob*
CEventQueue::getHandler(CEvent::Type type, void* target) const
{
	CArchMutexLock lock(m_mutex);
	CHandlerTable::const_iterator index = m_handlers.find(target);
	if (index != m_handlers.end()) {
		const CTypeHandlerTable& typeHandlers = index->second;
		CTypeHandlerTable::const_iterator index2 = typeHandlers.find(type);
		if (index2 != typeHandlers.end()) {
			return index2->second;
		}
	}
	return NULL;
}

UInt32
CEventQueue::saveEvent(const CEvent& event)
{
	// choose id
	UInt32 id;
	if (!m_oldEventIDs.empty()) {
		// reuse an id
		id = m_oldEventIDs.back();
		m_oldEventIDs.pop_back();
	}
	else {
		// make a new id
		id = static_cast<UInt32>(m_events.size());
	}

	// save data
	m_events[id] = event;
	return id;
}

CEvent
CEventQueue::removeEvent(UInt32 eventID)
{
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

CEvent::Type
CEventQueue::getRegisteredType(const CString& name) const
{
	CNameMap::const_iterator found = m_nameMap.find(name);
	if (found != m_nameMap.end())
		return found->second;

	return CEvent::kUnknown;
}

void*
CEventQueue::getSystemTarget()
{
	// any unique arbitrary pointer will do
	return &m_systemTarget;
}

//
// CEventQueue::CTimer
//

CEventQueue::CTimer::CTimer(CEventQueueTimer* timer, double timeout,
				double initialTime, void* target, bool oneShot) :
	m_timer(timer),
	m_timeout(timeout),
	m_target(target),
	m_oneShot(oneShot),
	m_time(initialTime)
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

CEventQueue::CTimer&
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
