/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include "gmock/gmock-matchers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define LEVEL_PRINT "%z\057"
#define LEVEL_ERR "%z\061"
#define LEVEL_INFO "%z\064"

using testing::EndsWith;
using testing::HasSubstr;
using testing::internal::CaptureStderr;
using testing::internal::CaptureStdout;
using testing::internal::GetCapturedStderr;
using testing::internal::GetCapturedStdout;

TEST(LogTests, print_withErrorLevel_outputIsValid)
{
  CaptureStderr();
  Log log(false);

  log.print(nullptr, 0, LEVEL_ERR "test message");

  EXPECT_THAT(GetCapturedStderr(), EndsWith("ERROR: test message\n"));
}

TEST(LogTests, print_simpleString_outputIsValid)
{
  CaptureStdout();
  Log log(false);

  log.print(nullptr, 0, LEVEL_PRINT "test message");

  EXPECT_THAT(GetCapturedStdout(), EndsWith("test message\n"));
}

TEST(LogTests, print_withArgs_outputIsValid)
{
  CaptureStdout();
  Log log(false);

  log.print(nullptr, 0, LEVEL_INFO "test %d %.2f %s", 1, 1.234, "test arg");

  EXPECT_THAT(GetCapturedStdout(), HasSubstr("INFO: test 1 1.23 test arg\n"));
}

TEST(LogTests, print_withPrintLevel_outputIsValid)
{
  CaptureStdout();
  Log log(false);

  log.print(nullptr, 0, LEVEL_PRINT "test message");

  EXPECT_THAT(GetCapturedStdout(), "test message\n");
}

TEST(LogTests, print_longMessage_outputIsValid)
{
  CaptureStdout();
  Log log(false);

  auto longString = std::string(10000, 'a');
  log.print(nullptr, 0, LEVEL_INFO "%s", longString.c_str());

  EXPECT_THAT(GetCapturedStdout(), HasSubstr("INFO: " + longString + "\n"));
}

TEST(LogTests, print_highestLevel_noOutput)
{
  CaptureStdout();
  Log log(false);

  log.print(CLOG_DEBUG5 "test message");

  EXPECT_EQ(GetCapturedStdout(), "");
}

TEST(LogTests, print_infoWithFileAndLine_outputIsValid)
{
  CaptureStdout();
  Log log(false);

  log.print("test file", 123, LEVEL_INFO "test message");

  EXPECT_THAT(GetCapturedStdout(), EndsWith("INFO: test message\n\ttest file:123\n"));
}

TEST(LogTests, print_errorWithFileAndLine_outputIsValid)
{
  CaptureStderr();
  Log log(false);

  log.print("test file", 123, LEVEL_ERR "test message");

  EXPECT_THAT(GetCapturedStderr(), EndsWith("ERROR: test message\n\ttest file:123\n"));
}
