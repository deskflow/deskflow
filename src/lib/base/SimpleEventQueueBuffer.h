/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"
#include "base/IEventQueueBuffer.h"
#include "common/stddeque.h"

//! In-memory event queue buffer
/*!
An event queue buffer provides a queue of events for an IEventQueue.
*/
class SimpleEventQueueBuffer : public IEventQueueBuffer
{
public:
  SimpleEventQueueBuffer();
  SimpleEventQueueBuffer(SimpleEventQueueBuffer const &) = delete;
  SimpleEventQueueBuffer(SimpleEventQueueBuffer &&) = delete;
  ~SimpleEventQueueBuffer();

  SimpleEventQueueBuffer &operator=(SimpleEventQueueBuffer const &) = delete;
  SimpleEventQueueBuffer &operator=(SimpleEventQueueBuffer &&) = delete;

  // IEventQueueBuffer overrides
  void init()
  {
  }
  virtual void waitForEvent(double timeout);
  virtual Type getEvent(Event &event, uint32_t &dataID);
  virtual bool addEvent(uint32_t dataID);
  virtual bool isEmpty() const;
  virtual EventQueueTimer *newTimer(double duration, bool oneShot) const;
  virtual void deleteTimer(EventQueueTimer *) const;

private:
  using EventDeque = std::deque<uint32_t>;

  ArchMutex m_queueMutex;
  ArchCond m_queueReadyCond;
  bool m_queueReady;
  EventDeque m_queue;
};
