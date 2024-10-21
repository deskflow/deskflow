/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Red Hat, Inc.
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
