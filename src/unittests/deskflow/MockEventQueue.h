/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"
#include "common/ExitCodes.h"

class MockEventQueue : public IEventQueue
{
public:
  EventQueueTimer *newOneShotTimer(double duration, void *target) override
  {
    return nullptr;
  }

  EventQueueTimer *newTimer(double, void *) override
  {
    return nullptr;
  }

  bool getEvent(Event &, double) override
  {
    return true;
  }

  int loop() override
  {
    return s_exitSuccess;
  }

  void adoptBuffer(IEventQueueBuffer *) override
  {
    // do nothing
  }

  void removeHandlers(void *) override
  {
    // do nothing
  }

  EventTypes registerType(const char *)
  {
    return EventTypes::Unknown;
  }

  void addHandler(EventTypes, void *, const EventHandler &) override
  {
    // do nothing
  }

  void addEvent(Event &&event) override
  {
    // do nothing
  }

  void removeHandler(EventTypes, void *) override
  {
    // do nothing
  }

  bool dispatchEvent(const Event &) override
  {
    return true;
  }

  void deleteTimer(EventQueueTimer *) override
  {
    // do nothing
  }

  void waitForReady() const override
  {
    // do nothing
  }

  void *getSystemTarget() override
  {
    return nullptr;
  }
};
