#include "lib/deskflow/IKeyState.h"

#include <gtest/gtest.h>

TEST(IKeyStateTests, KeyInfo_alloc_destinations)
{
  auto info = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1", "test2"});

  EXPECT_STREQ(info->m_screensBuffer, ":test1:test2:");
}
