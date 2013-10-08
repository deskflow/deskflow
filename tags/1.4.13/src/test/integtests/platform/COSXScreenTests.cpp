/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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

#define private public
#include "COSXScreen.h"
#include "CEventQueue.h"
#include "CArch.h"

// disabling these tests - the return value of CGCursorIsVisible is unreliable.
/*
TEST(COSXScreenTests, hideCursor_notPrimary)
{
	CEventQueue queue;
	COSXScreen screen(true, false);

	screen.hideCursor();

	EXPECT_EQ(false, CGCursorIsVisible());

	// workaround for screen class race condition.
	ARCH->sleep(.1f);
}

TEST(COSXScreenTests, showCursor_notPrimary)
{
	CEventQueue queue;
	COSXScreen screen(false, false);

	screen.showCursor();

	EXPECT_EQ(true, CGCursorIsVisible());

	// workaround for screen class race condition.
	ARCH->sleep(.1f);
}
*/
