/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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

class MSWindowsKeyStateTests : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
		m_hook.loadLibrary();
		m_screensaver = new MSWindowsScreenSaver();
	}

	virtual void TearDown()
	{
		delete m_screensaver;
	}

	MSWindowsDesks* newDesks(IEventQueue* eventQueue)
	{
		return new MSWindowsDesks(
			true, false, m_hook.getInstance(), m_screensaver, eventQueue,
			new TMethodJob<MSWindowsKeyStateTests>(
				this, &MSWindowsKeyStateTests::updateKeysCB), false);
	}

	void* getEventTarget() const
	{
		return const_cast<MSWindowsKeyStateTests*>(this);
	}

private:
	void updateKeysCB(void*) { }
	IScreenSaver* m_screensaver;
	MSWindowsHook m_hook;
};

TEST_F(MSWindowsKeyStateTests, disable_eventQueueNotUsed)
{
	NiceMock<MockEventQueue> eventQueue;
	MSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	MSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);
	
	EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(0);

	keyState.disable();
	delete desks;
}

TEST_F(MSWindowsKeyStateTests, testAutoRepeat_noRepeatAndButtonIsZero_resultIsTrue)
{
	NiceMock<MockEventQueue> eventQueue;
	MSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	MSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);
	keyState.setLastDown(1);

	bool actual = keyState.testAutoRepeat(true, false, 1);

	ASSERT_TRUE(actual);
	delete desks;
}

TEST_F(MSWindowsKeyStateTests, testAutoRepeat_pressFalse_lastDownIsZero)
{
	NiceMock<MockEventQueue> eventQueue;
	MSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	MSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);
	keyState.setLastDown(1);

	keyState.testAutoRepeat(false, false, 1);

	ASSERT_EQ(0, keyState.getLastDown());
	delete desks;
}

TEST_F(MSWindowsKeyStateTests, saveModifiers_noModifiers_savedModifiers0)
{
	NiceMock<MockEventQueue> eventQueue;
	MSWindowsDesks* desks = newDesks(&eventQueue);
	MockKeyMap keyMap;
	MSWindowsKeyState keyState(desks, getEventTarget(), &eventQueue, keyMap);

	keyState.saveModifiers();

	ASSERT_EQ(0, keyState.getSavedModifiers());
	delete desks;
}
