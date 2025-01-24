/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/SimpleEventQueueBuffer.h"
#include "arch/Arch.h"
#include "base/Stopwatch.h"

class EventQueueTimer
{
};

//
// SimpleEventQueueBuffer
//

SimpleEventQueueBuffer::SimpleEventQueueBuffer()
{
  m_queueMutex = ARCH->newMutex();
  m_queueReadyCond = ARCH->newCondVar();
  m_queueReady = false;
}

SimpleEventQueueBuffer::~SimpleEventQueueBuffer()
{
  ARCH->closeCondVar(m_queueReadyCond);
  ARCH->closeMutex(m_queueMutex);
}

void SimpleEventQueueBuffer::waitForEvent(double timeout)
{
  ArchMutexLock lock(m_queueMutex);
  Stopwatch timer(true);
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

IEventQueueBuffer::Type SimpleEventQueueBuffer::getEvent(Event &, uint32_t &dataID)
{
  ArchMutexLock lock(m_queueMutex);
  if (!m_queueReady) {
    return kNone;
  }
  dataID = m_queue.back();
  m_queue.pop_back();
  m_queueReady = !m_queue.empty();
  return kUser;
}

bool SimpleEventQueueBuffer::addEvent(uint32_t dataID)
{
  ArchMutexLock lock(m_queueMutex);
  m_queue.push_front(dataID);
  if (!m_queueReady) {
    m_queueReady = true;
    ARCH->broadcastCondVar(m_queueReadyCond);
  }
  return true;
}

bool SimpleEventQueueBuffer::isEmpty() const
{
  ArchMutexLock lock(m_queueMutex);
  return !m_queueReady;
}

EventQueueTimer *SimpleEventQueueBuffer::newTimer(double, bool) const
{
  return new EventQueueTimer;
}

void SimpleEventQueueBuffer::deleteTimer(EventQueueTimer *timer) const
{
  delete timer;
}
