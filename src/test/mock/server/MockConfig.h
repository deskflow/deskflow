/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define TEST_ENV

#include "server/Config.h"

#include <gmock/gmock.h>

class MockConfig : public Config
{
public:
  MockConfig() : Config()
  {
  }
  MOCK_METHOD(InputFilter *, getInputFilter, (), (override));
  MOCK_METHOD(bool, isScreen, (const std::string &), (const, override));
};
