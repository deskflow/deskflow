/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "platform/OSXScreen.h"

#include <gtest/gtest.h>

// TODO: fix intermittently failing test
// return value of CGCursorIsVisible is unreliable
#if 0

TEST(OSXScreenTests, hideCursor_notPrimary)
{
    EventQueue queue;
    OSXScreen screen(true, false);

    screen.hideCursor();

    EXPECT_EQ(false, CGCursorIsVisible());

    // workaround for screen class race condition.
    ARCH->sleep(.1f);
}

TEST(OSXScreenTests, showCursor_notPrimary)
{
    EventQueue queue;
    OSXScreen screen(false, false);

    screen.showCursor();

    EXPECT_EQ(true, CGCursorIsVisible());

    // workaround for screen class race condition.
    ARCH->sleep(.1f);
}

#endif
