/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define TEST_ENV

#include "deskflow/Screen.h"

#include <gmock/gmock.h>

class MockScreen : public deskflow::Screen
{
public:
  MockScreen() : deskflow::Screen()
  {
  }
  MOCK_METHOD(void, disable, (), (override));
  MOCK_METHOD(void, getShape, (int32_t &, int32_t &, int32_t &, int32_t &), (const, override));
  MOCK_METHOD(void, getCursorPos, (int32_t &, int32_t &), (const, override));
  MOCK_METHOD(void, resetOptions, (), (override));
  MOCK_METHOD(void, setOptions, (const OptionsList &), (override));
  MOCK_METHOD(void, enable, (), (override));
};
