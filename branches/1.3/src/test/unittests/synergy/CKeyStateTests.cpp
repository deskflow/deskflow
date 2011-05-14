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
#include "CMockKeyMap.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Invoke;

enum {
	kAKey = 30
};

TEST(CKeyStateTests, onKey_aKeyDown_keyStateOne)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	keyState.onKey(kAKey, true, KeyModifierAlt);

	EXPECT_EQ(1, keyState.getKeyState(kAKey));
}

TEST(CKeyStateTests, onKey_aKeyUp_keyStateZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	keyState.onKey(kAKey, false, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(kAKey));
}

TEST(CKeyStateTests, onKey_invalidKey_keyStateZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	keyState.onKey(0, true, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(0));
}

TEST(CKeyStateTests, sendKeyEvent_halfDuplexAndRepeat_addEventNotCalled)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	keyState.setHalfDuplexMask(KeyModifierCapsLock);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(0);

	keyState.sendKeyEvent(NULL, false, true, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_halfDuplex_addEventCalledTwice)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	keyState.setHalfDuplexMask(KeyModifierCapsLock);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(2);

	keyState.sendKeyEvent(NULL, false, false, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyRepeat_addEventCalledOnce)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, false, true, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyDown_addEventCalledOnce)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, true, false, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyUp_addEventCalledOnce)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, false, false, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, updateKeyMap_mockKeyMap_keyMapGotMock)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, swap(_));
	EXPECT_CALL(keyMap, finish());

	keyState.updateKeyMap();
}

void stubPollPressedKeys(IKeyState::KeyButtonSet& pressedKeys)
{
	pressedKeys.insert(kAKey);
}

TEST(CKeyStateTests, updateKeyState_pollInsertsSingleKey_keyIsDown)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(keyState, pollPressedKeys(_));
	ON_CALL(keyState, pollPressedKeys(_)).WillByDefault(Invoke(stubPollPressedKeys));

	keyState.updateKeyState();

	bool actual = keyState.isKeyDown(kAKey);
	ASSERT_TRUE(actual);
}

TEST(CKeyStateTests, updateKeyState_pollDoesNothing_keyNotSet)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(keyState, pollPressedKeys(_));

	keyState.updateKeyState();

	bool actual = keyState.isKeyDown(kAKey);
	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_maskSet)
{
	// TODO
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_keyMapGotModifers)
{
	// TODO
}
