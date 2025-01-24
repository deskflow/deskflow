/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define TEST_ENV

#include "deskflow/ArgParser.h"

#include <gmock/gmock.h>

class MockArgParser : public ArgParser
{
public:
  MockArgParser() : ArgParser(NULL)
  {
  }

  MOCK_METHOD(bool, parseGenericArgs, (int, const char *const *, int &));
  MOCK_METHOD(bool, checkUnexpectedArgs, ());
};
