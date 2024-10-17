#include "base/XBase.h"

#include <gtest/gtest.h>

TEST(XBaseTests, what_emptyWhat_returnsWhatFromGetWhat)
{
  XBase xbase;

  const char *result = xbase.what();

  EXPECT_STREQ("", result);
}

TEST(XBaseTests, what_nonEmptyWhat_returnsWhatFromGetWhat)
{
  XBase xbase("test");

  const char *result = xbase.what();

  EXPECT_STREQ("test", result);
}
