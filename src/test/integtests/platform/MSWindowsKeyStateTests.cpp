/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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

#define TEST_ENV

#include "test/mock/synergy/MockEventQueue.h"
#include "test/mock/synergy/MockKeyMap.h"
#include "platform/MSWindowsKeyState.h"
#include "platform/MSWindowsDesks.h"
#include "platform/MSWindowsScreen.h"
#include "platform/MSWindowsScreenSaver.h"
#include "base/TMethodJob.h"

#include "test/global/gtest.h"
#include "test/global/gmock.h"

// wParam = flags, HIBYTE(lParam) = virtual key, LOBYTE(lParam) = scan code
#define SYNERGY_MSG_FAKE_KEY		SYNERGY_HOOK_LAST_MSG + 4

using ::testing::_;
using ::testing::NiceMock;

class CMSWindowsKeyStateTests : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
		m_hook.loadLibrary();
		m_screensaver = new CMSWindowsScreenSaver();
	}

	virtual void TearDown()
	{
		delete m_screensaver;
	}

	CMSWindowsDesks* newDesks(IEventQueue* eventQueue)
	{
		return new CMSWindowsDesks(
			true, false, m_hook.getInstance(), m_screensaver, eventQueue,
			new TMethodJob<CMSWindowsKeyStateTests>(
				this, &CMSWindowsKeyStateTests::updateKeysCB), false);
	}

	void* getEventTarget() const
	{
		return const_cast<CMSWindowsKeyStateTests*>(this);
	}

private:
	void updateKeysCB(void*) { }
	IScreenSaver* m_screensaver;
	CMSWindowsHook m_hook;
};

TEST_F(CMSWindowsKeyStateTests, disable_eventQueueNotUsed)
{
	NiceMock<MockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);
	
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(0);

	keyState.disable();
	delete desks;
}

TEST_F(CMSWindowsKeyStateTests, testAutoRepeat_noRepeatAndButtonIsZero_resultIsTrue)
{
	NiceMock<MockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);
	keyState.setLastDown(1);

	bool actual = keyState.testAutoRepeat(true, false, 1);

	ASSERT_TRUE(actual);
	delete desks;
}

TEST_F(CMSWindowsKeyStateTests, testAutoRepeat_pressFalse_lastDownIsZero)
{
	NiceMock<MockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);
	keyState.setLastDown(1);

	keyState.testAutoRepeat(false, false, 1);

	ASSERT_EQ(0, keyState.getLastDown());
	delete desks;
}

TEST_F(CMSWindowsKeyStateTests, saveModifiers_noModifiers_savedModifiers0)
{
	NiceMock<MockEventQueue> eventQueue;
	CMSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	CMSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);

	keyState.saveModifiers();

	ASSERT_EQ(0, keyState.getSavedModifiers());
	delete desks;
}
