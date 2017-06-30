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

#include "test/mock/synergy/MockKeyState.h"
#include "test/mock/synergy/MockEventQueue.h"
#include "test/mock/synergy/MockKeyMap.h"

#include "test/global/gtest.h"
#include "test/global/gmock.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

void stubPollPressedKeys (IKeyState::KeyButtonSet& pressedKeys);

void assertMaskIsOne (ForeachKeyCallback cb, void* userData);

const synergy::KeyMap::KeyItem*
stubMapKey (synergy::KeyMap::Keystrokes& keys, KeyID id, SInt32 group,
            synergy::KeyMap::ModifierToKeys& activeModifiers,
            KeyModifierMask& currentState, KeyModifierMask desiredMask,
            bool isAutoRepeat);

synergy::KeyMap::Keystroke s_stubKeystroke (1, false, false);
synergy::KeyMap::KeyItem s_stubKeyItem;

TEST (CKeyStateTests, onKey_aKeyDown_keyStateOne) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    keyState.onKey (1, true, KeyModifierAlt);

    EXPECT_EQ (1, keyState.getKeyState (1));
}

TEST (KeyStateTests, onKey_aKeyUp_keyStateZero) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    keyState.onKey (1, false, KeyModifierAlt);

    EXPECT_EQ (0, keyState.getKeyState (1));
}

TEST (KeyStateTests, onKey_invalidKey_keyStateZero) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    keyState.onKey (0, true, KeyModifierAlt);

    EXPECT_EQ (0, keyState.getKeyState (0));
}

TEST (KeyStateTests, sendKeyEvent_halfDuplexAndRepeat_addEventNotCalled) {
    NiceMock<MockKeyMap> keyMap;
    NiceMock<MockEventQueue> eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    ON_CALL (keyMap, isHalfDuplex (_, _)).WillByDefault (Return (true));

    EXPECT_CALL (eventQueue, addEvent (_)).Times (0);

    keyState.sendKeyEvent (NULL, false, true, kKeyCapsLock, 0, 0, 0);
}

TEST (KeyStateTests, sendKeyEvent_halfDuplex_addEventCalledTwice) {
    NiceMock<MockKeyMap> keyMap;
    NiceMock<MockEventQueue> eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    IKeyStateEvents keyStateEvents;
    keyStateEvents.setEvents (&eventQueue);

    ON_CALL (keyMap, isHalfDuplex (_, _)).WillByDefault (Return (true));
    ON_CALL (eventQueue, forIKeyState ())
        .WillByDefault (ReturnRef (keyStateEvents));

    EXPECT_CALL (eventQueue, addEvent (_)).Times (2);

    keyState.sendKeyEvent (NULL, false, false, kKeyCapsLock, 0, 0, 0);
}

TEST (KeyStateTests, sendKeyEvent_keyRepeat_addEventCalledOnce) {
    NiceMock<MockKeyMap> keyMap;
    NiceMock<MockEventQueue> eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    IKeyStateEvents keyStateEvents;
    keyStateEvents.setEvents (&eventQueue);

    ON_CALL (eventQueue, forIKeyState ())
        .WillByDefault (ReturnRef (keyStateEvents));

    EXPECT_CALL (eventQueue, addEvent (_)).Times (1);

    keyState.sendKeyEvent (NULL, false, true, 1, 0, 0, 0);
}

TEST (KeyStateTests, sendKeyEvent_keyDown_addEventCalledOnce) {
    NiceMock<MockKeyMap> keyMap;
    NiceMock<MockEventQueue> eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    IKeyStateEvents keyStateEvents;
    keyStateEvents.setEvents (&eventQueue);

    ON_CALL (eventQueue, forIKeyState ())
        .WillByDefault (ReturnRef (keyStateEvents));

    EXPECT_CALL (eventQueue, addEvent (_)).Times (1);

    keyState.sendKeyEvent (NULL, true, false, 1, 0, 0, 0);
}

TEST (KeyStateTests, sendKeyEvent_keyUp_addEventCalledOnce) {
    NiceMock<MockKeyMap> keyMap;
    NiceMock<MockEventQueue> eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    IKeyStateEvents keyStateEvents;
    keyStateEvents.setEvents (&eventQueue);

    ON_CALL (eventQueue, forIKeyState ())
        .WillByDefault (ReturnRef (keyStateEvents));

    EXPECT_CALL (eventQueue, addEvent (_)).Times (1);

    keyState.sendKeyEvent (NULL, false, false, 1, 0, 0, 0);
}

TEST (KeyStateTests, updateKeyMap_mockKeyMap_keyMapGotMock) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    // key map member gets a new key map via swap()
    EXPECT_CALL (keyMap, swap (_));

    keyState.updateKeyMap ();
}

TEST (KeyStateTests, updateKeyState_pollInsertsSingleKey_keyIsDown) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    ON_CALL (keyState, pollPressedKeys (_))
        .WillByDefault (Invoke (stubPollPressedKeys));

    keyState.updateKeyState ();

    bool actual = keyState.isKeyDown (1);
    ASSERT_TRUE (actual);
}

TEST (KeyStateTests, updateKeyState_pollDoesNothing_keyNotSet) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    keyState.updateKeyState ();

    bool actual = keyState.isKeyDown (1);
    ASSERT_FALSE (actual);
}

TEST (KeyStateTests, updateKeyState_activeModifiers_maskSet) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    ON_CALL (keyState, pollActiveModifiers ())
        .WillByDefault (Return (KeyModifierAlt));

    keyState.updateKeyState ();

    KeyModifierMask actual = keyState.getActiveModifiers ();
    ASSERT_EQ (KeyModifierAlt, actual);
}

TEST (KeyStateTests, updateKeyState_activeModifiers_maskNotSet) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    keyState.updateKeyState ();

    KeyModifierMask actual = keyState.getActiveModifiers ();
    ASSERT_EQ (0, actual);
}

TEST (KeyStateTests, updateKeyState_activeModifiers_keyMapGotModifers) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    ON_CALL (keyState, pollActiveModifiers ()).WillByDefault (Return (1));
    ON_CALL (keyMap, foreachKey (_, _))
        .WillByDefault (Invoke (assertMaskIsOne));

    // key map gets new modifiers via foreachKey()
    EXPECT_CALL (keyMap, foreachKey (_, _));

    keyState.updateKeyState ();
}

TEST (KeyStateTests, setHalfDuplexMask_capsLock_halfDuplexCapsLockAdded) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    EXPECT_CALL (keyMap, addHalfDuplexModifier (kKeyCapsLock));

    keyState.setHalfDuplexMask (KeyModifierCapsLock);
}

TEST (KeyStateTests, setHalfDuplexMask_numLock_halfDuplexNumLockAdded) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    EXPECT_CALL (keyMap, addHalfDuplexModifier (kKeyNumLock));

    keyState.setHalfDuplexMask (KeyModifierNumLock);
}

TEST (KeyStateTests, setHalfDuplexMask_scrollLock_halfDuplexScollLockAdded) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    EXPECT_CALL (keyMap, addHalfDuplexModifier (kKeyScrollLock));

    keyState.setHalfDuplexMask (KeyModifierScrollLock);
}

TEST (KeyStateTests, fakeKeyDown_serverKeyAlreadyDown_fakeKeyCalledTwice) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    s_stubKeyItem.m_client = 0;
    s_stubKeyItem.m_button = 1;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Invoke (stubMapKey));

    // 2 calls to fakeKeyDown should still call fakeKey, even though
    // repeated keys are handled differently.
    EXPECT_CALL (keyState, fakeKey (_)).Times (2);

    // call twice to simulate server key already down (a misreported
    // autorepeat).
    keyState.fakeKeyDown (1, 0, 0);
    keyState.fakeKeyDown (1, 0, 0);
}

TEST (KeyStateTests, fakeKeyDown_isIgnoredKey_fakeKeyNotCalled) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    EXPECT_CALL (keyState, fakeKey (_)).Times (0);

    keyState.fakeKeyDown (kKeyCapsLock, 0, 0);
}

TEST (KeyStateTests, fakeKeyDown_mapReturnsKeystrokes_fakeKeyCalled) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    s_stubKeyItem.m_button = 0;
    s_stubKeyItem.m_client = 0;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Invoke (stubMapKey));

    EXPECT_CALL (keyState, fakeKey (_)).Times (1);

    keyState.fakeKeyDown (1, 0, 0);
}

TEST (KeyStateTests, fakeKeyRepeat_invalidKey_returnsFalse) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    bool actual = keyState.fakeKeyRepeat (0, 0, 0, 0);

    ASSERT_FALSE (actual);
}

TEST (KeyStateTests, fakeKeyRepeat_nullKey_returnsFalse) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    // set the key to down (we need to make mapKey return a valid key to do
    // this).
    synergy::KeyMap::KeyItem keyItem;
    keyItem.m_client = 0;
    keyItem.m_button = 1;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Return (&keyItem));
    keyState.fakeKeyDown (1, 0, 0);

    // change mapKey to return NULL so that fakeKeyRepeat exits early.
    synergy::KeyMap::KeyItem* nullKeyItem = NULL;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Return (nullKeyItem));

    bool actual = keyState.fakeKeyRepeat (1, 0, 0, 0);

    ASSERT_FALSE (actual);
}

TEST (KeyStateTests, fakeKeyRepeat_invalidButton_returnsFalse) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    // set the key to down (we need to make mapKey return a valid key to do
    // this).
    synergy::KeyMap::KeyItem keyItem;
    keyItem.m_client = 0;
    keyItem.m_button = 1; // set to 1 to make fakeKeyDown work.
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Return (&keyItem));
    keyState.fakeKeyDown (1, 0, 0);

    // change button to 0 so that fakeKeyRepeat will return early.
    keyItem.m_button = 0;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Return (&keyItem));

    bool actual = keyState.fakeKeyRepeat (1, 0, 0, 0);

    ASSERT_FALSE (actual);
}

TEST (KeyStateTests, fakeKeyRepeat_validKey_returnsTrue) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);
    s_stubKeyItem.m_client = 0;
    s_stubKeystroke.m_type = synergy::KeyMap::Keystroke::kButton;
    s_stubKeystroke.m_data.m_button.m_button = 2;

    // set the button to 1 for fakeKeyDown call
    s_stubKeyItem.m_button = 1;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Invoke (stubMapKey));
    keyState.fakeKeyDown (1, 0, 0);

    // change the button to 2
    s_stubKeyItem.m_button = 2;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Invoke (stubMapKey));

    bool actual = keyState.fakeKeyRepeat (1, 0, 0, 0);

    ASSERT_TRUE (actual);
}

TEST (KeyStateTests, fakeKeyUp_buttonNotDown_returnsFalse) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    bool actual = keyState.fakeKeyUp (0);

    ASSERT_FALSE (actual);
}

TEST (KeyStateTests, fakeKeyUp_buttonAlreadyDown_returnsTrue) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    // press alt down so we get full coverage.
    ON_CALL (keyState, pollActiveModifiers ())
        .WillByDefault (Return (KeyModifierAlt));
    keyState.updateKeyState ();

    // press button 1 down.
    s_stubKeyItem.m_button = 1;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Invoke (stubMapKey));
    keyState.fakeKeyDown (1, 0, 1);

    // this takes the button id, which is the 3rd arg of fakeKeyDown
    bool actual = keyState.fakeKeyUp (1);

    ASSERT_TRUE (actual);
}

TEST (KeyStateTests, fakeAllKeysUp_keysWereDown_keysAreUp) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    // press button 1 down.
    s_stubKeyItem.m_button = 1;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Invoke (stubMapKey));
    keyState.fakeKeyDown (1, 0, 1);

    // method under test
    keyState.fakeAllKeysUp ();

    bool actual = keyState.isKeyDown (1);
    ASSERT_FALSE (actual);
}

TEST (KeyStateTests, isKeyDown_keyDown_returnsTrue) {
    NiceMock<MockKeyMap> keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    // press button 1 down.
    s_stubKeyItem.m_button = 1;
    ON_CALL (keyMap, mapKey (_, _, _, _, _, _, _))
        .WillByDefault (Invoke (stubMapKey));
    keyState.fakeKeyDown (1, 0, 1);

    // method under test
    bool actual = keyState.isKeyDown (1);

    ASSERT_TRUE (actual);
}

TEST (KeyStateTests, isKeyDown_noKeysDown_returnsFalse) {
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    KeyStateImpl keyState (eventQueue, keyMap);

    // method under test
    bool actual = keyState.isKeyDown (1);

    ASSERT_FALSE (actual);
}

void
stubPollPressedKeys (IKeyState::KeyButtonSet& pressedKeys) {
    pressedKeys.insert (1);
}

void
assertMaskIsOne (ForeachKeyCallback cb, void* userData) {
    ASSERT_EQ (1, ((KeyState::AddActiveModifierContext*) userData)->m_mask);
}

const synergy::KeyMap::KeyItem*
stubMapKey (synergy::KeyMap::Keystrokes& keys, KeyID id, SInt32 group,
            synergy::KeyMap::ModifierToKeys& activeModifiers,
            KeyModifierMask& currentState, KeyModifierMask desiredMask,
            bool isAutoRepeat) {
    keys.push_back (s_stubKeystroke);
    return &s_stubKeyItem;
}
