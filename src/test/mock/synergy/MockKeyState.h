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

#pragma once

#include "synergy/KeyState.h"

#include "test/global/gmock.h"

class MockKeyMap;
class MockEventQueue;

// NOTE: do not mock methods that are not pure virtual. this mock exists only
// to provide an implementation of the KeyState abstract class.
class MockKeyState : public KeyState {
public:
    MockKeyState (const MockEventQueue& eventQueue)
        : KeyState ((IEventQueue*) &eventQueue) {
    }

    MockKeyState (const MockEventQueue& eventQueue, const MockKeyMap& keyMap)
        : KeyState ((IEventQueue*) &eventQueue, (synergy::KeyMap&) keyMap) {
    }

    MOCK_CONST_METHOD0 (pollActiveGroup, SInt32 ());
    MOCK_CONST_METHOD0 (pollActiveModifiers, KeyModifierMask ());
    MOCK_METHOD0 (fakeCtrlAltDel, bool());
    MOCK_METHOD1 (getKeyMap, void(synergy::KeyMap&));
    MOCK_METHOD1 (fakeKey, void(const Keystroke&));
    MOCK_METHOD1 (fakeMediaKey, bool(KeyID));
    MOCK_CONST_METHOD1 (pollPressedKeys, void(KeyButtonSet&));
};

typedef ::testing::NiceMock<MockKeyState> KeyStateImpl;

typedef UInt32 KeyID;

typedef void (*ForeachKeyCallback) (KeyID, SInt32 group,
                                    synergy::KeyMap::KeyItem&, void* userData);
