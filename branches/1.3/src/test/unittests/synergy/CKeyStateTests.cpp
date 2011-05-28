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
#include "CKeyStateTests.h"
#include "CMockEventQueue.h"
#include "CMockKeyMap.h"
#include "CKeyStateImpl.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;

CKeyMap::Keystroke s_stubKeystroke(1, false, false);
CKeyMap::KeyItem s_stubKeyItem;

TEST(CKeyStateTests, onKey_aKeyDown_keyStateOne)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	keyState.onKey(kAKey, true, KeyModifierAlt);

	EXPECT_EQ(1, keyState.getKeyState(kAKey));
}

TEST(CKeyStateTests, onKey_aKeyUp_keyStateZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	keyState.onKey(kAKey, false, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(kAKey));
}

TEST(CKeyStateTests, onKey_invalidKey_keyStateZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	keyState.onKey(0, true, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(0));
}

TEST(CKeyStateTests, sendKeyEvent_halfDuplexAndRepeat_addEventNotCalled)
{
	NiceMock<CMockKeyMap> keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);
	ON_CALL(keyMap, isHalfDuplex(_, _)).WillByDefault(Return(true));

	EXPECT_CALL(eventQueue, addEvent(_)).Times(0);

	keyState.sendKeyEvent(NULL, false, true, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_halfDuplex_addEventCalledTwice)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);
	ON_CALL(keyMap, isHalfDuplex(_, _)).WillByDefault(Return(true));

	EXPECT_CALL(eventQueue, addEvent(_)).Times(2);

	keyState.sendKeyEvent(NULL, false, false, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyRepeat_addEventCalledOnce)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, false, true, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyDown_addEventCalledOnce)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, true, false, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyUp_addEventCalledOnce)
{
	CMockKeyMap keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, false, false, kAKey, 0, 0, 0);
}

TEST(CKeyStateTests, updateKeyMap_mockKeyMap_keyMapGotMock)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, swap(_));
	EXPECT_CALL(keyMap, finish());

	keyState.updateKeyMap();
}

void
stubPollPressedKeys(IKeyState::KeyButtonSet& pressedKeys)
{
	pressedKeys.insert(kAKey);
}

TEST(CKeyStateTests, updateKeyState_pollInsertsSingleKey_keyIsDown)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);
	ON_CALL(keyState, pollPressedKeys(_)).WillByDefault(Invoke(stubPollPressedKeys));

	keyState.updateKeyState();

	bool actual = keyState.isKeyDown(kAKey);
	ASSERT_TRUE(actual);
}

TEST(CKeyStateTests, updateKeyState_pollDoesNothing_keyNotSet)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	keyState.updateKeyState();

	bool actual = keyState.isKeyDown(kAKey);
	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_maskSet)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);
	ON_CALL(keyState, pollActiveModifiers()).WillByDefault(Return(KeyModifierAlt));

	keyState.updateKeyState();

	KeyModifierMask actual = keyState.getActiveModifiers();
	ASSERT_EQ(KeyModifierAlt, actual);
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_maskNotSet)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	keyState.updateKeyState();

	KeyModifierMask actual = keyState.getActiveModifiers();
	ASSERT_EQ(0, actual);
}

void
assertMaskIsOne(ForeachKeyCallback cb, void* userData)
{
	ASSERT_EQ(1, ((CKeyState::CAddActiveModifierContext*)userData)->m_mask);
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_keyMapGotModifers)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);
	EXPECT_CALL(keyMap, foreachKey(_, _));

	ON_CALL(keyState, pollActiveModifiers()).WillByDefault(Return(1));
	ON_CALL(keyMap, foreachKey(_, _)).WillByDefault(Invoke(assertMaskIsOne));

	keyState.updateKeyState();
}

TEST(CKeyStateTests, setHalfDuplexMask_capsLock_halfDuplexCapsLockAdded)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, addHalfDuplexModifier(kKeyCapsLock));

	keyState.setHalfDuplexMask(KeyModifierCapsLock);
}

TEST(CKeyStateTests, setHalfDuplexMask_numLock_halfDuplexNumLockAdded)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, addHalfDuplexModifier(kKeyNumLock));

	keyState.setHalfDuplexMask(KeyModifierNumLock);
}

TEST(CKeyStateTests, setHalfDuplexMask_scrollLock_halfDuplexScollLockAdded)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, addHalfDuplexModifier(kKeyScrollLock));

	keyState.setHalfDuplexMask(KeyModifierScrollLock);
}

TEST(CKeyStateTests, fakeKeyDown_serverKeyAlreadyDown_fakeKeyRepeatCalled)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);
	CKeyMap::KeyItem keyItem;
	keyItem.m_client = 0;
	keyItem.m_button = 1; // TODO: what should this be?
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(&keyItem));

	EXPECT_CALL(keyState, fakeKeyRepeat(_, _, _, _));

	// call twice to simulate server key already down (a "mis-reported autorepeat").
	keyState.fakeKeyDown(kAKey, 0, 0);
	keyState.fakeKeyDown(kAKey, 0, 0);
}

TEST(CKeyStateTests, fakeKeyDown_isIgnoredKey_fakeKeyNotCalled)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	EXPECT_CALL(keyState, fakeKey(_)).Times(0);

	keyState.fakeKeyDown(kKeyCapsLock, 0, 0);
}

const CKeyMap::KeyItem*
stubMapKey(
	CKeyMap::Keystrokes& keys, KeyID id, SInt32 group,
	CKeyMap::ModifierToKeys& activeModifiers,
	KeyModifierMask& currentState,
	KeyModifierMask desiredMask,
	bool isAutoRepeat)
{
	keys.push_back(s_stubKeystroke);
	return &s_stubKeyItem;
}

TEST(CKeyStateTests, fakeKeyDown_mapReturnsKeystrokes_fakeKeyCalled)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CMockKeyState keyState(eventQueue, keyMap);

	s_stubKeyItem.m_button = 0;
	s_stubKeyItem.m_client = 0;
	EXPECT_CALL(keyMap, mapKey(_, _, _, _, _, _, _));
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));

	EXPECT_CALL(keyState, fakeKey(_)).Times(1);

	keyState.fakeKeyDown(kAKey, 0, 0);
}

TEST(CKeyStateTests, fakeKeyRepeat_invalidKey_returnsFalse)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	bool actual = keyState.fakeKeyRepeat(0, 0, 0, 0);

	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, fakeKeyRepeat_nullKey_returnsFalse)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	// set the key to down (we need to make mapKey return a valid key to do this).
	CKeyMap::KeyItem keyItem;
	keyItem.m_client = 0;
	keyItem.m_button = 1; // TODO: what should this be?
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(&keyItem));
	keyState.fakeKeyDown(kAKey, 0, 0);

	// change mapKey to return NULL so that fakeKeyRepeat exits early.
	CKeyMap::KeyItem* nullKeyItem = NULL;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(nullKeyItem));

	bool actual = keyState.fakeKeyRepeat(kAKey, 0, 0, 0);

	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, fakeKeyRepeat_invalidButton_returnsFalse)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	// set the key to down (we need to make mapKey return a valid key to do this).
	CKeyMap::KeyItem keyItem;
	keyItem.m_client = 0;
	keyItem.m_button = 1; // set to 1 to make fakeKeyDown work.
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(&keyItem));
	keyState.fakeKeyDown(kAKey, 0, 0);

	// change button to 0 so that fakeKeyRepeat will return early.
	keyItem.m_button = 0;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(&keyItem));

	bool actual = keyState.fakeKeyRepeat(kAKey, 0, 0, 0);

	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, fakeKeyRepeat_validKey_returnsTrue)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	s_stubKeyItem.m_client = 0;
	s_stubKeystroke.m_type = CKeyMap::Keystroke::kButton;
	s_stubKeystroke.m_data.m_button.m_button = 2;

	// set the button to 1 for fakeKeyDown call
	s_stubKeyItem.m_button = 1;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));
	keyState.fakeKeyDown(kAKey, 0, 0);

	// change the button to 2
	s_stubKeyItem.m_button = 2;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));

	bool actual = keyState.fakeKeyRepeat(kAKey, 0, 0, 0);

	ASSERT_TRUE(actual);
}
