/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: fix tests
#if 0

#include "platform/XWindowsScreenSaver.h"
#include "test/mock/deskflow/MockEventQueue.h"

#include <X11/Xlib.h>
#include <gtest/gtest.h>

using ::testing::_;

// TODO: not working on build machine for some reason
TEST(CXWindowsScreenSaverTests, activate_defaultScreen_todo)
{
    Display* display = XOpenDisplay(":0.0");
    Window window = DefaultRootWindow(display);
    MockEventQueue eventQueue;
    EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(1);
    CXWindowsScreenSaver screenSaver(
        display, window, NULL, &eventQueue);

    screenSaver.activate();

    bool isActive = screenSaver.isActive();

    screenSaver.deactivate();

    ASSERT_EQ(true, isActive);
}
#endif
