/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
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
  ~OSXEventQueueBuffer() override = default;

  // IEventQueueBuffer overrides
  void init() override;
  void waitForEvent(double timeout) override;
  Type getEvent(Event &event, uint32_t &dataID) override;
  bool addEvent(uint32_t dataID) override;
  bool isEmpty() const override;

private:
  IEventQueue *m_eventQueue;

  mutable std::mutex m_mutex;
  std::condition_variable m_cond;
  std::queue<uint32_t> m_dataQueue;
};
