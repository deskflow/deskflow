/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Stephen Jensen <sjensen313@proton.me>
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXEventQueueBuffer.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"

//
// EventQueueTimer
//

class EventQueueTimer
{
};

//
// OSXEventQueueBuffer
//

OSXEventQueueBuffer::OSXEventQueueBuffer(IEventQueue *events) : m_eventQueue(events)
{
  // Initialization is now managed using modern constructs
}

OSXEventQueueBuffer::~OSXEventQueueBuffer()
{
  // No explicit clean-up needed as GCD and STL handle resource management
}

void OSXEventQueueBuffer::init()
{
  // No initialization needed for GCD-based implementation
}

void OSXEventQueueBuffer::waitForEvent(double timeout)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_dataQueue.empty()) {
    auto duration = std::chrono::duration<double>(timeout);
    LOG_DEBUG2("waiting for event, timeout: %f seconds", timeout);
    m_cond.wait_for(lock, duration, [this] { return !m_dataQueue.empty(); });
  } else {
    LOG_DEBUG2("found events in the queue");
  }
}

IEventQueueBuffer::Type OSXEventQueueBuffer::getEvent(Event &event, uint32_t &dataID)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_dataQueue.empty()) {
    LOG_DEBUG2("no events in queue");
    return kNone;
  }

  dataID = m_dataQueue.front();
  m_dataQueue.pop();
  lock.unlock(); // Unlock early to allow other threads to proceed

  LOG_DEBUG2("handled user event with dataID: %u", dataID);
  return kUser;
}

bool OSXEventQueueBuffer::addEvent(uint32_t dataID)
{
  // Use GCD to dispatch event addition on the main queue
  dispatch_async(dispatch_get_main_queue(), ^{
    std::lock_guard<std::mutex> lock(this->m_mutex);
    LOG_DEBUG2("adding user event with dataID: %u", dataID);
    this->m_dataQueue.push(dataID);
    this->m_cond.notify_one();
    LOG_DEBUG2("user event added to queue, dataID=%u", dataID);
  });

  // Always return true since dispatch_async does not fail under normal conditions
  return true;
}

bool OSXEventQueueBuffer::isEmpty() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  bool empty = m_dataQueue.empty();
  LOG_DEBUG2("queue is %s", empty ? "empty" : "not empty");
  return empty;
}

EventQueueTimer *OSXEventQueueBuffer::newTimer(double, bool) const
{
  return new EventQueueTimer;
}

void OSXEventQueueBuffer::deleteTimer(EventQueueTimer *timer) const
{
  delete timer;
}
