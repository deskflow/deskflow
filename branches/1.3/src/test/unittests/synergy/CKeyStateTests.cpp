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
#include <gmock/gmock.h>
#include "CKeyStateImpl.h"
#include "CMockEventQueue.h"

using ::testing::_;
using ::testing::NiceMock;

enum {
	kAKey = 30
};

TEST(CKeyStateTests, onKey_aKeyDown_keyStateOne)
{
	CKeyStateImpl keyState(&CMockEventQueue());

	keyState.onKey(kAKey, true, KeyModifierAlt);

	EXPECT_EQ(1, keyState.getKeyState(kAKey));
}

TEST(CKeyStateTests, onKey_aKeyUp_keyStateZero)
{
	CKeyStateImpl keyState(&CMockEventQueue());

	keyState.onKey(kAKey, false, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(kAKey));
}

TEST(CKeyStateTests, onKey_invalidKey_keyStateZero)
{
	CKeyStateImpl keyState(&CMockEventQueue());

	keyState.onKey(0, true, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(0));
}

TEST(CKeyStateTests, sendKeyEvent_halfDuplexAndRepeat_addEventNotCalled)
{
	CMockEventQueue mockEventQueue;
	EXPECT_CALL(mockEventQueue, addEvent(_)).Times(0);

	CKeyStateImpl keyState(&mockEventQueue);
	keyState.setHalfDuplexMask(KeyModifierCapsLock);
	keyState.sendKeyEvent(NULL, false, true, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_halfDuplex_addEventCalledTwice)
{
	NiceMock<CMockEventQueue> mockEventQueue;
	EXPECT_CALL(mockEventQueue, addEvent(_)).Times(2);

	CKeyStateImpl keyState(&mockEventQueue);
	keyState.setHalfDuplexMask(KeyModifierCapsLock);
	keyState.sendKeyEvent(NULL, false, false, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyRepeat_addEventCalledOnce)
{
	NiceMock<CMockEventQueue> mockEventQueue;
	EXPECT_CALL(mockEventQueue, addEvent(_)).Times(1);

	CKeyStateImpl keyState(&mockEventQueue);
	keyState.sendKeyEvent(NULL, false, true, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyDown_addEventCalledOnce)
{
	NiceMock<CMockEventQueue> mockEventQueue;
	EXPECT_CALL(mockEventQueue, addEvent(_)).Times(1);

	CKeyStateImpl keyState(&mockEventQueue);
	keyState.sendKeyEvent(NULL, true, false, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyUp_addEventCalledOnce)
{
	NiceMock<CMockEventQueue> mockEventQueue;
	EXPECT_CALL(mockEventQueue, addEvent(_)).Times(1);

	CKeyStateImpl keyState(&mockEventQueue);
	keyState.sendKeyEvent(NULL, false, false, kAKey, 0, 0, 0);
}
