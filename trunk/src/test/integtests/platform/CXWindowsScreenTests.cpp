/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include "CXWindowsScreen.h"
#include "CMockEventQueue.h"

using ::testing::_;

TEST(CXWindowsScreenTests, fakeMouseMove_nonPrimary_getCursorPosValuesCorrect)
{
	CMockEventQueue eventQueue;
	EXPECT_CALL(eventQueue, adoptHandler(_, _, _)).Times(2);
	EXPECT_CALL(eventQueue, adoptBuffer(_)).Times(2);
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(2);
	CXWindowsScreen screen(":0.0", false, false, 0, eventQueue);

	screen.fakeMouseMove(10, 20);

	int x, y;
	screen.getCursorPos(x, y);
	ASSERT_EQ(10, x);
	ASSERT_EQ(20, y);
}
