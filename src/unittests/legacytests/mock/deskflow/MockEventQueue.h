/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"

#include <gmock/gmock.h>

class MockEventQueue : public IEventQueue
{
public:
  MOCK_METHOD(void, loop, (), (override));
  MOCK_METHOD(EventQueueTimer *, newOneShotTimer, (double, void *), (override));
  MOCK_METHOD(EventQueueTimer *, newTimer, (double, void *), (override));
  MOCK_METHOD(bool, getEvent, (Event &, double), (override));
  MOCK_METHOD(void, adoptBuffer, (IEventQueueBuffer *), (override));
  MOCK_METHOD(void, removeHandlers, (void *), (override));
  MOCK_METHOD(EventTypes, registerType, (const char *));
  MOCK_METHOD(bool, isEmpty, (), (const, override));
  MOCK_METHOD(void, adoptHandler, (EventTypes, void *, IEventJob *), (override));
  MOCK_METHOD(void, addEvent, (const Event &), (override));
  MOCK_METHOD(void, removeHandler, (EventTypes, void *), (override));
  MOCK_METHOD(bool, dispatchEvent, (const Event &), (override));
  MOCK_METHOD(IEventJob *, getHandler, (EventTypes, void *), (const, override));
  MOCK_METHOD(void, deleteTimer, (EventQueueTimer *), (override));
  MOCK_METHOD(void *, getSystemTarget, (), (override));
  MOCK_METHOD(void, waitForReady, (), (const, override));
};
