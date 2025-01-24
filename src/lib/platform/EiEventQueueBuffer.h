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
  EiEventQueueBuffer(EiScreen *screen, ei *ei, IEventQueue *events);
  ~EiEventQueueBuffer();

  // IEventQueueBuffer overrides
  void init() override
  {
  }
  void waitForEvent(double timeout_in_ms) override;
  Type getEvent(Event &event, uint32_t &dataID) override;
  bool addEvent(uint32_t dataID) override;
  bool isEmpty() const override;
  EventQueueTimer *newTimer(double duration, bool oneShot) const override;
  void deleteTimer(EventQueueTimer *) const override;

private:
  ei *ei_;
  IEventQueue *events_;
  std::queue<std::pair<bool, uint32_t>> queue_;
  int pipe_w_, pipe_r_;

  mutable std::mutex mutex_;
};

} // namespace deskflow
