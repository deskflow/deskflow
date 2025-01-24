/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ArgParser.h"

#include <gtest/gtest.h>

using namespace deskflow;

TEST(DeprecatedArgsParsingTests, parseDeprecatedArgs_cryptoPass_returnTrue)
{
  int i = 1;
  const int argc = 3;
  const char *kCryptoPassCmd[argc] = {"stub", "--crypto-pass", "mock_pass"};

  ArgParser argParser(NULL);

  bool result = argParser.parseDeprecatedArgs(argc, kCryptoPassCmd, i);

  EXPECT_EQ(true, result);
  EXPECT_EQ(2, i);
}

TEST(DeprecatedArgsParsingTests, parseDeprecatedArgs_cryptoPass_returnFalse)
{
  int i = 1;
  const int argc = 3;
  const char *kCryptoPassCmd[argc] = {"stub", "--mock-arg", "mock_value"};

  ArgParser argParser(NULL);

  bool result = argParser.parseDeprecatedArgs(argc, kCryptoPassCmd, i);

  EXPECT_FALSE(result);
  EXPECT_EQ(1, i);
}
