#include "base/Log.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::HasSubstr;
using testing::internal::CaptureStdout;
using testing::internal::GetCapturedStdout;

TEST(LogTests, print_simpleString_outputToConsole) {
  CaptureStdout();

  Log::getInstance()->print("test file", 1, "test message");

  std::string output = GetCapturedStdout();
  EXPECT_THAT(output, HasSubstr("INFO: test message\n\ttest file:1\n"));
}

TEST(LogTests, print_withArgs_outputToConsole) {
  CaptureStdout();

  Log::getInstance()->print("test file", 1, "test %d %.2f %s", 1, 1.234,
                            "test arg");

  std::string output = GetCapturedStdout();
  EXPECT_THAT(output, HasSubstr("INFO: test 1 1.23 test arg\n\ttest file:1\n"));
}
