/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueueBuffer.h"
#include "deskflow/IScreen.h"
#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <libei.h>
#include <memory>
#include <mutex>
#include <queue>

namespace deskflow {

//! Event queue buffer for Ei
class EiEventQueueBuffer : public IEventQueueBuffer
{
public:
  EiEventQueueBuffer(const EiScreen *screen, ei *ei, IEventQueue *events);
  ~EiEventQueueBuffer() override;

  // IEventQueueBuffer overrides
  void init() override
  {
    // do nothing
  }
  void waitForEvent(double msTimeout) override;
  Type getEvent(Event &event, uint32_t &dataID) override;
  bool addEvent(uint32_t dataID) override;
  bool isEmpty() const override;
  EventQueueTimer *newTimer(double duration, bool oneShot) const override;
  void deleteTimer(EventQueueTimer *) const override;

private:
  ei *m_ei;
  IEventQueue *m_events;
  std::queue<std::pair<bool, uint32_t>> m_queue;
  int m_pipeWrite;
  int m_pipeRead;

  mutable std::mutex m_mutex;
};

} // namespace deskflow
