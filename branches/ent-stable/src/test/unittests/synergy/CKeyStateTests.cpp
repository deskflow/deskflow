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
#include "CKeyStateTests.h"
#include "CMockEventQueue.h"
#include "CMockKeyMap.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

TEST(CKeyStateTests, onKey_aKeyDown_keyStateOne)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	keyState.onKey(1, true, KeyModifierAlt);

	EXPECT_EQ(1, keyState.getKeyState(1));
}

TEST(CKeyStateTests, onKey_aKeyUp_keyStateZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	keyState.onKey(1, false, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(1));
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
	NiceMock<CMockKeyMap> keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	ON_CALL(keyMap, isHalfDuplex(_, _)).WillByDefault(Return(true));

	EXPECT_CALL(eventQueue, addEvent(_)).Times(0);

	keyState.sendKeyEvent(NULL, false, true, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_halfDuplex_addEventCalledTwice)
{
	NiceMock<CMockKeyMap> keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	IKeyStateEvents keyStateEvents;
	keyStateEvents.setEvents(&eventQueue);
	
	ON_CALL(keyMap, isHalfDuplex(_, _)).WillByDefault(Return(true));
	ON_CALL(eventQueue, forIKeyState()).WillByDefault(ReturnRef(keyStateEvents));

	EXPECT_CALL(eventQueue, addEvent(_)).Times(2);

	keyState.sendKeyEvent(NULL, false, false, kKeyCapsLock, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyRepeat_addEventCalledOnce)
{
	NiceMock<CMockKeyMap> keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	IKeyStateEvents keyStateEvents;
	keyStateEvents.setEvents(&eventQueue);
	
	ON_CALL(eventQueue, forIKeyState()).WillByDefault(ReturnRef(keyStateEvents));

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, false, true, 1, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyDown_addEventCalledOnce)
{
	NiceMock<CMockKeyMap> keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	IKeyStateEvents keyStateEvents;
	keyStateEvents.setEvents(&eventQueue);
	
	ON_CALL(eventQueue, forIKeyState()).WillByDefault(ReturnRef(keyStateEvents));

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, true, false, 1, 0, 0, 0);
}

TEST(CKeyStateTests, sendKeyEvent_keyUp_addEventCalledOnce)
{
	NiceMock<CMockKeyMap> keyMap;
	NiceMock<CMockEventQueue> eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	IKeyStateEvents keyStateEvents;
	keyStateEvents.setEvents(&eventQueue);
	
	ON_CALL(eventQueue, forIKeyState()).WillByDefault(ReturnRef(keyStateEvents));

	EXPECT_CALL(eventQueue, addEvent(_)).Times(1);

	keyState.sendKeyEvent(NULL, false, false, 1, 0, 0, 0);
}

TEST(CKeyStateTests, updateKeyMap_mockKeyMap_keyMapGotMock)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	// key map member gets a new key map via swap()
	EXPECT_CALL(keyMap, swap(_));

	keyState.updateKeyMap();
}

TEST(CKeyStateTests, updateKeyState_pollInsertsSingleKey_keyIsDown)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	ON_CALL(keyState, pollPressedKeys(_)).WillByDefault(Invoke(stubPollPressedKeys));

	keyState.updateKeyState();

	bool actual = keyState.isKeyDown(1);
	ASSERT_TRUE(actual);
}

TEST(CKeyStateTests, updateKeyState_pollDoesNothing_keyNotSet)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	keyState.updateKeyState();

	bool actual = keyState.isKeyDown(1);
	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_maskSet)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	ON_CALL(keyState, pollActiveModifiers()).WillByDefault(Return(KeyModifierAlt));

	keyState.updateKeyState();

	KeyModifierMask actual = keyState.getActiveModifiers();
	ASSERT_EQ(KeyModifierAlt, actual);
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_maskNotSet)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	keyState.updateKeyState();

	KeyModifierMask actual = keyState.getActiveModifiers();
	ASSERT_EQ(0, actual);
}

TEST(CKeyStateTests, updateKeyState_activeModifiers_keyMapGotModifers)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	ON_CALL(keyState, pollActiveModifiers()).WillByDefault(Return(1));
	ON_CALL(keyMap, foreachKey(_, _)).WillByDefault(Invoke(assertMaskIsOne));

	// key map gets new modifiers via foreachKey()
	EXPECT_CALL(keyMap, foreachKey(_, _));

	keyState.updateKeyState();
}

TEST(CKeyStateTests, setHalfDuplexMask_capsLock_halfDuplexCapsLockAdded)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, addHalfDuplexModifier(kKeyCapsLock));

	keyState.setHalfDuplexMask(KeyModifierCapsLock);
}

TEST(CKeyStateTests, setHalfDuplexMask_numLock_halfDuplexNumLockAdded)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, addHalfDuplexModifier(kKeyNumLock));

	keyState.setHalfDuplexMask(KeyModifierNumLock);
}

TEST(CKeyStateTests, setHalfDuplexMask_scrollLock_halfDuplexScollLockAdded)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(keyMap, addHalfDuplexModifier(kKeyScrollLock));

	keyState.setHalfDuplexMask(KeyModifierScrollLock);
}

TEST(CKeyStateTests, fakeKeyDown_serverKeyAlreadyDown_fakeKeyCalledTwice)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	s_stubKeyItem.m_client = 0;
	s_stubKeyItem.m_button = 1;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));

	// 2 calls to fakeKeyDown should still call fakeKey, even though
	// repeated keys are handled differently.
	EXPECT_CALL(keyState, fakeKey(_)).Times(2);

	// call twice to simulate server key already down (a misreported autorepeat).
	keyState.fakeKeyDown(1, 0, 0);
	keyState.fakeKeyDown(1, 0, 0);
}

TEST(CKeyStateTests, fakeKeyDown_isIgnoredKey_fakeKeyNotCalled)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	EXPECT_CALL(keyState, fakeKey(_)).Times(0);

	keyState.fakeKeyDown(kKeyCapsLock, 0, 0);
}

TEST(CKeyStateTests, fakeKeyDown_mapReturnsKeystrokes_fakeKeyCalled)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);
	s_stubKeyItem.m_button = 0;
	s_stubKeyItem.m_client = 0;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));

	EXPECT_CALL(keyState, fakeKey(_)).Times(1);

	keyState.fakeKeyDown(1, 0, 0);
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
	keyItem.m_button = 1;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(&keyItem));
	keyState.fakeKeyDown(1, 0, 0);

	// change mapKey to return NULL so that fakeKeyRepeat exits early.
	CKeyMap::KeyItem* nullKeyItem = NULL;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(nullKeyItem));

	bool actual = keyState.fakeKeyRepeat(1, 0, 0, 0);

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
	keyState.fakeKeyDown(1, 0, 0);

	// change button to 0 so that fakeKeyRepeat will return early.
	keyItem.m_button = 0;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Return(&keyItem));

	bool actual = keyState.fakeKeyRepeat(1, 0, 0, 0);

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
	keyState.fakeKeyDown(1, 0, 0);

	// change the button to 2
	s_stubKeyItem.m_button = 2;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));

	bool actual = keyState.fakeKeyRepeat(1, 0, 0, 0);

	ASSERT_TRUE(actual);
}

TEST(CKeyStateTests, fakeKeyUp_buttonNotDown_returnsFalse)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	bool actual = keyState.fakeKeyUp(0);

	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, fakeKeyUp_buttonAlreadyDown_returnsTrue)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	// press alt down so we get full coverage.
	ON_CALL(keyState, pollActiveModifiers()).WillByDefault(Return(KeyModifierAlt));
	keyState.updateKeyState();

	// press button 1 down.
	s_stubKeyItem.m_button = 1;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));
	keyState.fakeKeyDown(1, 0, 1);

	// this takes the button id, which is the 3rd arg of fakeKeyDown
	bool actual = keyState.fakeKeyUp(1);

	ASSERT_TRUE(actual);
}

TEST(CKeyStateTests, fakeAllKeysUp_keysWereDown_keysAreUp)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	// press button 1 down.
	s_stubKeyItem.m_button = 1;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));
	keyState.fakeKeyDown(1, 0, 1);

	// method under test
	keyState.fakeAllKeysUp();

	bool actual = keyState.isKeyDown(1);
	ASSERT_FALSE(actual);
}

TEST(CKeyStateTests, isKeyDown_keyDown_returnsTrue)
{
	NiceMock<CMockKeyMap> keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	// press button 1 down.
	s_stubKeyItem.m_button = 1;
	ON_CALL(keyMap, mapKey(_, _, _, _, _, _, _)).WillByDefault(Invoke(stubMapKey));
	keyState.fakeKeyDown(1, 0, 1);

	// method under test
	bool actual = keyState.isKeyDown(1);

	ASSERT_TRUE(actual);
}

TEST(CKeyStateTests, isKeyDown_noKeysDown_returnsFalse)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CKeyStateImpl keyState(eventQueue, keyMap);

	// method under test
	bool actual = keyState.isKeyDown(1);

	ASSERT_FALSE(actual);
}

void
stubPollPressedKeys(IKeyState::KeyButtonSet& pressedKeys)
{
	pressedKeys.insert(1);
}

void
assertMaskIsOne(ForeachKeyCallback cb, void* userData)
{
	ASSERT_EQ(1, ((CKeyState::CAddActiveModifierContext*)userData)->m_mask);
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
