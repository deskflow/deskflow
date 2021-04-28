/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test/mock/synergy/MockKeyMap.h"
#include "test/mock/synergy/MockEventQueue.h"
#include "platform/MSWindowsHook.h"
#include "synergy/XScreen.h"

#include "test/global/gtest.h"
#include "test/global/gmock.h"

TEST(MSWindowsHookTests, hookInit)
{
	MSWindowsHook hook;
    try {
        hook.loadLibrary();
    }  catch (const XBase&) {
        FAIL() << "MSWindowsHook loadLibrary failed";
    }
}
