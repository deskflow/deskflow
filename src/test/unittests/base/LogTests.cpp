#include "base/Log.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::EndsWith;
using testing::HasSubstr;
using testing::internal::CaptureStderr;
using testing::internal::CaptureStdout;
using testing::internal::GetCapturedStderr;
using testing::internal::GetCapturedStdout;

TEST(LogTests, print_releaseWithErrorLevel_outputIsValid) {
  CaptureStderr();
  Log log(false, false);

  log.print(CLOG_ERR "test message");

  EXPECT_THAT(GetCapturedStderr(), EndsWith("ERROR: test message\n"));
}

TEST(LogTests, print_releaseSimpleString_outputIsValid) {
  CaptureStdout();
  Log log(false, false);

  log.print("test file", 1, "test message");

  EXPECT_THAT(GetCapturedStdout(), EndsWith("INFO: test message\n"));
}

TEST(LogTests, print_debugSimpleString_outputIsValid) {
  CaptureStdout();
  Log log(false, true);

  log.print("test file", 1, "test message");

  EXPECT_THAT(GetCapturedStdout(),
              HasSubstr("INFO: test message\n\ttest file:1\n"));
}

TEST(LogTests, print_debugWithArgs_outputIsValid) {
  CaptureStdout();
  Log log(false, true);

  log.print("test file", 1, "test %d %.2f %s", 1, 1.234, "test arg");

  EXPECT_THAT(GetCapturedStdout(),
              HasSubstr("INFO: test 1 1.23 test arg\n\ttest file:1\n"));
}

TEST(LogTests, print_debugWithPrintLevel_outputIsValid) {
  CaptureStdout();
  Log log(false, true);

  log.print(CLOG_PRINT "test message");

  EXPECT_THAT(GetCapturedStdout(), "test message\n");
}

TEST(LogTests, print_debugWithErrorLevel_outputIsValid) {
  CaptureStderr();
  Log log(false, true);

  log.print(CLOG_ERR "test message");

  EXPECT_THAT(GetCapturedStderr(), HasSubstr("ERROR: test message\n"));
}

TEST(LogTests, print_debugLongMessage_outputIsValid) {
  CaptureStdout();
  Log log(false, true);

  auto longString = std::string(10000, 'a');
  log.print("test file", 1, "%s", longString.c_str());

  EXPECT_THAT(GetCapturedStdout(),
              HasSubstr("INFO: " + longString + "\n\ttest file:1\n"));
}

TEST(LogTests, print_debugHighestLevel_noOutput) {
  CaptureStdout();
  Log log(false, true);

  log.print(CLOG_DEBUG5 "test message");

  EXPECT_EQ(GetCapturedStdout(), "");
}
