/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test/mock/deskflow/MockEventQueue.h"

#include <gtest/gtest.h>

#include "platform/XWindowsScreen.h"

using ::testing::_;

// TODO: fix failing tests
#if 0
TEST(CXWindowsScreenTests, fakeMouseMove_nonPrimary_getCursorPosValuesCorrect) {
  MockEventQueue eventQueue;
  EXPECT_CALL(eventQueue, adoptHandler(_, _, _)).Times(2);
  EXPECT_CALL(eventQueue, adoptBuffer(_)).Times(2);
  EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(2);
  XWindowsScreen screen(
      ":0.0", false, false, 0, &eventQueue);

  screen.fakeMouseMove(10, 20);

  int x, y;
  screen.getCursorPos(x, y);
  ASSERT_EQ(10, x);
  ASSERT_EQ(20, y);
}
#endif
