/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"
#include "base/IEventQueueBuffer.h"

#include <deque>
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
  ~SimpleEventQueueBuffer() override;

  SimpleEventQueueBuffer &operator=(SimpleEventQueueBuffer const &) = delete;
  SimpleEventQueueBuffer &operator=(SimpleEventQueueBuffer &&) = delete;

  // IEventQueueBuffer overrides
  void init() override
  {
    // do nothing
  }
  void waitForEvent(double timeout) override;
  Type getEvent(Event &event, uint32_t &dataID) override;
  bool addEvent(uint32_t dataID) override;
  bool isEmpty() const override;

private:
  using EventDeque = std::deque<uint32_t>;

  ArchMutex m_queueMutex;
  ArchCond m_queueReadyCond;
  bool m_queueReady = false;
  EventDeque m_queue;
};
