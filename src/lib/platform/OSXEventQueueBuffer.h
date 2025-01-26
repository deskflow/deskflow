/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Stephen Jensen <sjensen313@proton.me>
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueueBuffer.h"

#include <condition_variable>
#include <dispatch/dispatch.h>
#include <mutex>
#include <queue>

class IEventQueue;

//! Event queue buffer for OS X
class OSXEventQueueBuffer : public IEventQueueBuffer
{
public:
  OSXEventQueueBuffer(IEventQueue *eventQueue);
  virtual ~OSXEventQueueBuffer();

  // IEventQueueBuffer overrides
  virtual void init() override;
  virtual void waitForEvent(double timeout) override;
  virtual Type getEvent(Event &event, uint32_t &dataID) override;
  virtual bool addEvent(uint32_t dataID) override;
  virtual bool isEmpty() const override;
  virtual EventQueueTimer *newTimer(double duration, bool oneShot) const override;
  virtual void deleteTimer(EventQueueTimer *timer) const override;

private:
  IEventQueue *m_eventQueue;

  mutable std::mutex m_mutex;
  std::condition_variable m_cond;
  std::queue<uint32_t> m_dataQueue;
};
