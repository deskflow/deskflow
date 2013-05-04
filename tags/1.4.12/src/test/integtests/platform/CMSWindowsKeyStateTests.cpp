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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define TEST_ENV
#include "Global.h"

#include "CMSWindowsKeyState.h"
#include "CMSWindowsDesks.h"
#include "CMSWindowsScreen.h"
#include "CMSWindowsScreenSaver.h"
#include "TMethodJob.h"
#include "CMockEventQueue.h"
#include "CMockKeyMap.h"

// wParam = flags, HIBYTE(lParam) = virtual key, LOBYTE(lParam) = scan code
#define SYNERGY_MSG_FAKE_KEY		SYNERGY_HOOK_LAST_MSG + 4

using ::testing::_;
using ::testing::NiceMock;

class CMSWindowsKeyStateTests : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
		// load synrgyhk.dll
		m_hookLibrary = m_hookLibraryLoader.openHookLibrary("synrgyhk");
		m_screensaver = new CMSWindowsScreenSaver();
	}

	virtual void TearDown()
	{
		delete m_screensaver;
	}

	CMSWindowsDesks* newDesks(IEventQueue& eventQueue)
	{
		return new CMSWindowsDesks(
			true, false, m_hookLibrary, m_screensaver, eventQueue,
			new TMethodJob<CMSWindowsKeyStateTests>(
				this, &CMSWindowsKeyStateTests::updateKeysCB), false);
	}

	void* getEventTarget() const
	{
		return const_cast<CMSWindowsKeyStateTests*>(this);
	}

private:
	void updateKeysCB(void*) { }
	HINSTANCE m_hookLibrary;
	IScreenSaver* m_screensaver;
	CMSWindowsHookLibraryLoader m_hookLibraryLoader;
};

TEST_F(CMSWindowsKeyStateTests, disable_nonWin95OS_eventQueueNotUsed)
{
	NiceMock<CMockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(eventQueue);
	CMockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), eventQueue, keyMap);
	
	// in anything above win95-family, event handler should not be called.
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(0);

	keyState.disable();
	delete desks;
}

TEST_F(CMSWindowsKeyStateTests, testAutoRepeat_noRepeatAndButtonIsZero_resultIsTrue)
{
	NiceMock<CMockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(eventQueue);
	CMockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), eventQueue, keyMap);
	keyState.setLastDown(1);

	bool actual = keyState.testAutoRepeat(true, false, 1);

	ASSERT_TRUE(actual);
	delete desks;
}

TEST_F(CMSWindowsKeyStateTests, testAutoRepeat_pressFalse_lastDownIsZero)
{
	NiceMock<CMockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(eventQueue);
	CMockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), eventQueue, keyMap);
	keyState.setLastDown(1);

	keyState.testAutoRepeat(false, false, 1);

	ASSERT_EQ(0, keyState.getLastDown());
	delete desks;
}

TEST_F(CMSWindowsKeyStateTests, saveModifiers_noModifiers_savedModifiers0)
{
	NiceMock<CMockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(eventQueue);
	CMockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), eventQueue, keyMap);

	keyState.saveModifiers();

	ASSERT_EQ(0, keyState.getSavedModifiers());
	delete desks;
}
/*
TEST_F(CMSWindowsKeyStateTests, saveModifiers_shiftKeyDown_savedModifiers4)
{
	NiceMock<CMockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(eventQueue);
	CMockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), eventQueue, keyMap);
	desks->enable();
	desks->fakeKeyEvent(1, 1, true, false);

	keyState.saveModifiers();

	ASSERT_EQ(1, keyState.getSavedModifiers());
	delete desks;
}
*/