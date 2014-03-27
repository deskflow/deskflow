/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#if 0
#include <gtest/gtest.h>
#include "CXWindowsScreenSaver.h"
#include "CMockEventQueue.h"
#include <X11/Xlib.h>

using ::testing::_;

// TODO: not working on build machine for some reason
TEST(CXWindowsScreenSaverTests, activate_defaultScreen_todo)
{
	Display* display = XOpenDisplay(":0.0");
	Window window = DefaultRootWindow(display);
	CMockEventQueue eventQueue;
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(1);
	CXWindowsScreenSaver screenSaver(
		display, window, NULL, &eventQueue);

	screenSaver.activate();

	bool isActive = screenSaver.isActive();

	screenSaver.deactivate();

	ASSERT_EQ(true, isActive);
}
#endif
