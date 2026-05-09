/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
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
// OSXEventQueueBuffer
//

OSXEventQueueBuffer::OSXEventQueueBuffer(IEventQueue *events) : m_eventQueue(events)
{
  // Initialization is now managed using modern constructs
}

void OSXEventQueueBuffer::init()
{
  // No initialization needed for GCD-based implementation
}

void OSXEventQueueBuffer::waitForEvent(double timeout)
{
  std::unique_lock lock(m_mutex);
  if (m_dataQueue.empty()) {
    LOG_VERBOSE("waiting for event, timeout: %f seconds", timeout);
    auto end = timeout < 0 ? std::chrono::steady_clock::time_point::max()
                           : std::chrono::steady_clock::now() + std::chrono::duration<double>(timeout);
    m_cond.wait_until(lock, end, [this] { return !m_dataQueue.empty(); });
  } else {
    LOG_VERBOSE("found events in the queue");
  }
}

IEventQueueBuffer::Type OSXEventQueueBuffer::getEvent(Event &event, uint32_t &dataID)
{
  std::unique_lock lock(m_mutex);
  if (m_dataQueue.empty()) {
    LOG_VERBOSE("no events in queue");
    return IEventQueueBuffer::Type::Unknown;
  }

  dataID = m_dataQueue.front();
  m_dataQueue.pop();
  lock.unlock(); // Unlock early to allow other threads to proceed

  LOG_VERBOSE("handled user event with dataID: %u", dataID);
  return IEventQueueBuffer::Type::User;
}

bool OSXEventQueueBuffer::addEvent(uint32_t dataID)
{
  std::scoped_lock lock{m_mutex};
  LOG_VERBOSE("adding user event with dataID: %u", dataID);
  m_dataQueue.push(dataID);
  m_cond.notify_one();
  LOG_VERBOSE("user event added to queue, dataID=%u", dataID);
  return true;
}

bool OSXEventQueueBuffer::isEmpty() const
{
  std::scoped_lock lock{m_mutex};
  bool empty = m_dataQueue.empty();
  LOG_VERBOSE("queue is %s", empty ? "empty" : "not empty");
  return empty;
}
