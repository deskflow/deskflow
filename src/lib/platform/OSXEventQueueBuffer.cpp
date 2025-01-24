/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXEventQueueBuffer.h"

#include "base/Event.h"
#include "base/IEventQueue.h"

//
// EventQueueTimer
//

class EventQueueTimer
{
};

//
// OSXEventQueueBuffer
//

OSXEventQueueBuffer::OSXEventQueueBuffer(IEventQueue *events)
    : m_event(NULL),
      m_eventQueue(events),
      m_carbonEventQueue(NULL)
{
  // do nothing
}

OSXEventQueueBuffer::~OSXEventQueueBuffer()
{
  // release the last event
  if (m_event != NULL) {
    ReleaseEvent(m_event);
  }
}

void OSXEventQueueBuffer::init()
{
  m_carbonEventQueue = GetCurrentEventQueue();
}

void OSXEventQueueBuffer::waitForEvent(double timeout)
{
  EventRef event;
  ReceiveNextEvent(0, NULL, timeout, false, &event);
}

IEventQueueBuffer::Type OSXEventQueueBuffer::getEvent(Event &event, uint32_t &dataID)
{
  // release the previous event
  if (m_event != NULL) {
    ReleaseEvent(m_event);
    m_event = NULL;
  }

  // get the next event
  OSStatus error = ReceiveNextEvent(0, NULL, 0.0, true, &m_event);

  // handle the event
  if (error == eventLoopQuitErr) {
    event = Event(Event::kQuit);
    return kSystem;
  } else if (error != noErr) {
    return kNone;
  } else {
    uint32_t eventClass = GetEventClass(m_event);
    switch (eventClass) {
    case 'Syne':
      dataID = GetEventKind(m_event);
      return kUser;

    default:
      event = Event(Event::kSystem, m_eventQueue->getSystemTarget(), &m_event);
      return kSystem;
    }
  }
}

bool OSXEventQueueBuffer::addEvent(uint32_t dataID)
{
  EventRef event;
  OSStatus error = CreateEvent(kCFAllocatorDefault, 'Syne', dataID, 0, kEventAttributeNone, &event);

  if (error == noErr) {

    assert(m_carbonEventQueue != NULL);

    error = PostEventToQueue(m_carbonEventQueue, event, kEventPriorityStandard);

    ReleaseEvent(event);
  }

  return (error == noErr);
}

bool OSXEventQueueBuffer::isEmpty() const
{
  EventRef event;
  OSStatus status = ReceiveNextEvent(0, NULL, 0.0, false, &event);
  return (status == eventLoopTimedOutErr);
}

EventQueueTimer *OSXEventQueueBuffer::newTimer(double, bool) const
{
  return new EventQueueTimer;
}

void OSXEventQueueBuffer::deleteTimer(EventQueueTimer *timer) const
{
  delete timer;
}
