#include "lib/server/Server.h"
#include <gtest/gtest.h>

TEST(ServerTests, allocLockCursorToScreenInfo_withState_setsState) {
  auto info = Server::LockCursorToScreenInfo::alloc(
      Server::LockCursorToScreenInfo::kOff);

  EXPECT_EQ(info->m_state, Server::LockCursorToScreenInfo::kOff);
}
