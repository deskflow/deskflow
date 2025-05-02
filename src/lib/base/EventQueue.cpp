/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/EventQueue.h"

#include "arch/Arch.h"
#include "base/IEventJob.h"
#include "base/Log.h"
#include "base/SimpleEventQueueBuffer.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"

#include <stdexcept>

// interrupt handler.  this just adds a quit event to the queue.
static void interrupt(Arch::ESignal, void *data)
{
  auto *events = static_cast<EventQueue *>(data);
  events->addEvent(Event(EventTypes::Quit));
}

//
// EventQueue
//

EventQueue::EventQueue()
    : m_systemTarget(0),
      m_readyMutex(new Mutex),
      m_readyCondVar(new CondVar<bool>(m_readyMutex, false))
{
  m_mutex = ARCH->newMutex();
  ARCH->setSignalHandler(Arch::kINTERRUPT, &interrupt, this);
  ARCH->setSignalHandler(Arch::kTERMINATE, &interrupt, this);
  m_buffer = new SimpleEventQueueBuffer;
}

EventQueue::~EventQueue()
{
  delete m_buffer;
  delete m_readyCondVar;
  delete m_readyMutex;

  ARCH->setSignalHandler(Arch::kINTERRUPT, nullptr, nullptr);
  ARCH->setSignalHandler(Arch::kTERMINATE, nullptr, nullptr);
  ARCH->closeMutex(m_mutex);
}

void EventQueue::loop()
{
  m_buffer->init();
  {
    Lock lock(m_readyMutex);
    *m_readyCondVar = true;
    m_readyCondVar->signal();
  }
  LOG((CLOG_DEBUG "event queue is ready"));
  while (!m_pending.empty()) {
    LOG((CLOG_DEBUG "add pending events to buffer"));
    Event &event = m_pending.front();
    addEventToBuffer(event);
    m_pending.pop();
  }

  Event event;
  getEvent(event);
  while (event.getType() != EventTypes::Quit) {
    dispatchEvent(event);
    Event::deleteData(event);
    getEvent(event);
  }
}

void EventQueue::adoptBuffer(IEventQueueBuffer *buffer)
{
  ArchMutexLock lock(m_mutex);

  LOG((CLOG_DEBUG "adopting new buffer"));

  if (m_events.size() != 0) {
    // this can come as a nasty surprise to programmers expecting
    // their events to be raised, only to have them deleted.
    LOG((CLOG_DEBUG "discarding %d event(s)", m_events.size()));
  }

  // discard old buffer and old events
  delete m_buffer;
  for (EventTable::iterator i = m_events.begin(); i != m_events.end(); ++i) {
    Event::deleteData(i->second);
  }
  m_events.clear();
  m_oldEventIDs.clear();

  // use new buffer
  m_buffer = buffer;
  if (m_buffer == nullptr) {
    m_buffer = new SimpleEventQueueBuffer;
  }
}

bool EventQueue::getEvent(Event &event, double timeout)
{
  Stopwatch timer(true);
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
  uint32_t dataID;
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

  case IEventQueueBuffer::kUser: {
    ArchMutexLock lock(m_mutex);
    event = removeEvent(dataID);
    return true;
  }

  default:
    assert(0 && "invalid event type");
    return false;
  }
}

bool EventQueue::dispatchEvent(const Event &event)
{
  void *target = event.getTarget();
  IEventJob *job = getHandler(event.getType(), target);
  if (job == nullptr) {
    job = getHandler(EventTypes::Unknown, target);
  }
  if (job != nullptr) {
    job->run(event);
    return true;
  }
  return false;
}

void EventQueue::addEvent(const Event &event)
{
  // discard bogus event types
  switch (event.getType()) {
  case EventTypes::Unknown:
  case EventTypes::System:
  case EventTypes::Timer:
    return;

  default:
    break;
  }

  if ((event.getFlags() & Event::kDeliverImmediately) != 0) {
    dispatchEvent(event);
    Event::deleteData(event);
  } else if (!(*m_readyCondVar)) {
    m_pending.push(event);
  } else {
    addEventToBuffer(event);
  }
}

void EventQueue::addEventToBuffer(const Event &event)
{
  ArchMutexLock lock(m_mutex);

  // store the event's data locally
  uint32_t eventID = saveEvent(event);

  // add it
  if (!m_buffer->addEvent(eventID)) {
    // failed to send event
    removeEvent(eventID);
    Event::deleteData(event);
  }
}

EventQueueTimer *EventQueue::newTimer(double duration, void *target)
{
  assert(duration > 0.0);

  EventQueueTimer *timer = m_buffer->newTimer(duration, false);
  if (target == nullptr) {
    target = timer;
  }
  ArchMutexLock lock(m_mutex);
  m_timers.insert(timer);
  // initial duration is requested duration plus whatever's on
  // the clock currently because the latter will be subtracted
  // the next time we check for timers.
  m_timerQueue.push(Timer(timer, duration, duration + m_time.getTime(), target, false));
  return timer;
}

EventQueueTimer *EventQueue::newOneShotTimer(double duration, void *target)
{
  assert(duration > 0.0);

  EventQueueTimer *timer = m_buffer->newTimer(duration, true);
  if (target == nullptr) {
    target = timer;
  }
  ArchMutexLock lock(m_mutex);
  m_timers.insert(timer);
  // initial duration is requested duration plus whatever's on
  // the clock currently because the latter will be subtracted
  // the next time we check for timers.
  m_timerQueue.push(Timer(timer, duration, duration + m_time.getTime(), target, true));
  return timer;
}

void EventQueue::deleteTimer(EventQueueTimer *timer)
{
  ArchMutexLock lock(m_mutex);
  for (auto index = m_timerQueue.begin(); index != m_timerQueue.end(); ++index) {
    if (index->getTimer() == timer) {
      m_timerQueue.erase(index);
      break;
    }
  }
  Timers::iterator index = m_timers.find(timer);
  if (index != m_timers.end()) {
    m_timers.erase(index);
  }
  m_buffer->deleteTimer(timer);
}

void EventQueue::adoptHandler(EventTypes type, void *target, IEventJob *handler)
{
  ArchMutexLock lock(m_mutex);
  IEventJob *&job = m_handlers[target][type];
  delete job;
  job = handler;
}

void EventQueue::removeHandler(EventTypes type, void *target)
{
  IEventJob *handler = nullptr;
  {
    ArchMutexLock lock(m_mutex);
    HandlerTable::iterator index = m_handlers.find(target);
    if (index != m_handlers.end()) {
      TypeHandlerTable &typeHandlers = index->second;
      TypeHandlerTable::iterator index2 = typeHandlers.find(type);
      if (index2 != typeHandlers.end()) {
        handler = index2->second;
        typeHandlers.erase(index2);
      }
    }
  }
  delete handler;
}

void EventQueue::removeHandlers(void *target)
{
  std::vector<IEventJob *> handlers;
  {
    ArchMutexLock lock(m_mutex);
    HandlerTable::iterator index = m_handlers.find(target);
    if (index != m_handlers.end()) {
      // copy to handlers array and clear table for target
      TypeHandlerTable &typeHandlers = index->second;
      for (auto index2 = typeHandlers.begin(); index2 != typeHandlers.end(); ++index2) {
        handlers.push_back(index2->second);
      }
      typeHandlers.clear();
    }
  }

  // delete handlers
  for (auto index = handlers.begin(); index != handlers.end(); ++index) {
    delete *index;
  }
}

bool EventQueue::isEmpty() const
{
  return (m_buffer->isEmpty() && getNextTimerTimeout() != 0.0);
}

IEventJob *EventQueue::getHandler(EventTypes type, void *target) const
{
  ArchMutexLock lock(m_mutex);
  HandlerTable::const_iterator index = m_handlers.find(target);
  if (index != m_handlers.end()) {
    const TypeHandlerTable &typeHandlers = index->second;
    TypeHandlerTable::const_iterator index2 = typeHandlers.find(type);
    if (index2 != typeHandlers.end()) {
      return index2->second;
    }
  }
  return nullptr;
}

uint32_t EventQueue::saveEvent(const Event &event)
{
  // choose id
  uint32_t id;
  if (!m_oldEventIDs.empty()) {
    // reuse an id
    id = m_oldEventIDs.back();
    m_oldEventIDs.pop_back();
  } else {
    // make a new id
    id = static_cast<uint32_t>(m_events.size());
  }

  // save data
  m_events[id] = event;
  return id;
}

Event EventQueue::removeEvent(uint32_t eventID)
{
  // look up id
  EventTable::iterator index = m_events.find(eventID);
  if (index == m_events.end()) {
    return Event();
  }

  // get data
  Event event = index->second;
  m_events.erase(index);

  // save old id for reuse
  m_oldEventIDs.push_back(eventID);

  return event;
}

bool EventQueue::hasTimerExpired(Event &event)
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
  for (auto index = m_timerQueue.begin(); index != m_timerQueue.end(); ++index) {
    (*index) -= time;
  }

  // done if no timers are expired
  if (m_timerQueue.top() > 0.0) {
    return false;
  }

  // remove timer from queue
  Timer timer = m_timerQueue.top();
  m_timerQueue.pop();

  // prepare event and reset the timer's clock
  timer.fillEvent(m_timerEvent);
  event = Event(EventTypes::Timer, timer.getTarget(), &m_timerEvent);
  timer.reset();

  // reinsert timer into queue if it's not a one-shot
  if (!timer.isOneShot()) {
    m_timerQueue.push(timer);
  }

  return true;
}

double EventQueue::getNextTimerTimeout() const
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

void *EventQueue::getSystemTarget()
{
  // any unique arbitrary pointer will do
  return &m_systemTarget;
}

void EventQueue::waitForReady() const
{
  double timeout = ARCH->time() + 10;
  Lock lock(m_readyMutex);

  while (!m_readyCondVar->wait()) {
    if (ARCH->time() > timeout) {
      throw std::runtime_error("event queue is not ready within 5 sec");
    }
  }
}

//
// EventQueue::Timer
//

EventQueue::Timer::Timer(EventQueueTimer *timer, double timeout, double initialTime, void *target, bool oneShot)
    : m_timer(timer),
      m_timeout(timeout),
      m_target(target),
      m_oneShot(oneShot),
      m_time(initialTime)
{
  assert(m_timeout > 0.0);
}

void EventQueue::Timer::reset()
{
  m_time = m_timeout;
}

EventQueue::Timer &EventQueue::Timer::operator-=(double dt)
{
  m_time -= dt;
  return *this;
}

EventQueue::Timer::operator double() const
{
  return m_time;
}

bool EventQueue::Timer::isOneShot() const
{
  return m_oneShot;
}

EventQueueTimer *EventQueue::Timer::getTimer() const
{
  return m_timer;
}

void *EventQueue::Timer::getTarget() const
{
  return m_target;
}

void EventQueue::Timer::fillEvent(TimerEvent &event) const
{
  event.m_timer = m_timer;
  event.m_count = 0;
  if (m_time <= 0.0) {
    event.m_count = static_cast<uint32_t>((m_timeout - m_time) / m_timeout);
  }
}

bool EventQueue::Timer::operator<(const Timer &t) const
{
  return m_time < t.m_time;
}
