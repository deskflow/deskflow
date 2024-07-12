#include "base/Log.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::HasSubstr;

TEST(LogTests, print_simpleString_outputToConsole) {
  testing::internal::CaptureStdout();

  Log::getInstance()->print("test file", 1, "test message");

  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, HasSubstr("INFO: test message\n\ttest file:1\n"));
}
