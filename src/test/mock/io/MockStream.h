/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "io/IStream.h"

#include <gmock/gmock.h>

class IEventQueue;

namespace {

class MockStream : public deskflow::IStream
{
public:
  MockStream()
  {
  }
  MOCK_METHOD(void, close, (), (override));
  MOCK_METHOD(uint32_t, read, (void *, uint32_t), (override));
  MOCK_METHOD(void, write, (const void *, uint32_t), (override));
  MOCK_METHOD(void, flush, (), (override));
  MOCK_METHOD(void, shutdownInput, (), (override));
  MOCK_METHOD(void, shutdownOutput, (), (override));
  MOCK_METHOD(Event::Type, getInputReadyEvent, ());
  MOCK_METHOD(Event::Type, getOutputErrorEvent, ());
  MOCK_METHOD(Event::Type, getInputShutdownEvent, ());
  MOCK_METHOD(Event::Type, getOutputShutdownEvent, ());
  MOCK_METHOD(void *, getEventTarget, (), (const, override));
  MOCK_METHOD(bool, isReady, (), (const, override));
  MOCK_METHOD(uint32_t, getSize, (), (const, override));
};

} // namespace
