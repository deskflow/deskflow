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
#include "platform/OSXKeyState.h"
#include "base/Log.h"

#include "test/global/gtest.h"
#include "test/global/gmock.h"

#define SHIFT_ID_L kKeyShift_L
#define SHIFT_ID_R kKeyShift_R
#define SHIFT_BUTTON 57
#define A_CHAR_ID 0x00000061
#define A_CHAR_BUTTON 001

class OSXKeyStateTests : public ::testing::Test {
public:
    static bool isKeyPressed (const OSXKeyState& keyState, KeyButton button);
};

// fakeAndPoll_shift seems to always fail on osx10.6
#if __MAC_OS_X_VERSION_MIN_REQUIRED > 1060

TEST_F (OSXKeyStateTests, fakeAndPoll_shift) {
    synergy::KeyMap keyMap;
    MockEventQueue eventQueue;
    OSXKeyState keyState (&eventQueue, keyMap);
    keyState.updateKeyMap ();

    keyState.fakeKeyDown (SHIFT_ID_L, 0, 1);
    EXPECT_TRUE (isKeyPressed (keyState, SHIFT_BUTTON));

    keyState.fakeKeyUp (1);
    EXPECT_TRUE (!isKeyPressed (keyState, SHIFT_BUTTON));

    keyState.fakeKeyDown (SHIFT_ID_R, 0, 2);
    EXPECT_TRUE (isKeyPressed (keyState, SHIFT_BUTTON));

    keyState.fakeKeyUp (2);
    EXPECT_TRUE (!isKeyPressed (keyState, SHIFT_BUTTON));
}

TEST_F (OSXKeyStateTests, fakeAndPoll_charKey) {
    synergy::KeyMap keyMap;
    MockEventQueue eventQueue;
    OSXKeyState keyState (&eventQueue, keyMap);
    keyState.updateKeyMap ();

    keyState.fakeKeyDown (A_CHAR_ID, 0, 1);
    EXPECT_TRUE (isKeyPressed (keyState, A_CHAR_BUTTON));

    keyState.fakeKeyUp (1);
    EXPECT_TRUE (!isKeyPressed (keyState, A_CHAR_BUTTON));

    // HACK: delete the key in case it was typed into a text editor.
    // we should really set focus to an invisible window.
    keyState.fakeKeyDown (kKeyBackSpace, 0, 2);
    keyState.fakeKeyUp (2);
}

TEST_F (OSXKeyStateTests, fakeAndPoll_charKeyAndModifier) {
    synergy::KeyMap keyMap;
    MockEventQueue eventQueue;
    OSXKeyState keyState (&eventQueue, keyMap);
    keyState.updateKeyMap ();

    keyState.fakeKeyDown (A_CHAR_ID, KeyModifierShift, 1);
    EXPECT_TRUE (isKeyPressed (keyState, A_CHAR_BUTTON));

    keyState.fakeKeyUp (1);
    EXPECT_TRUE (!isKeyPressed (keyState, A_CHAR_BUTTON));

    // HACK: delete the key in case it was typed into a text editor.
    // we should really set focus to an invisible window.
    keyState.fakeKeyDown (kKeyBackSpace, 0, 2);
    keyState.fakeKeyUp (2);
}

bool
OSXKeyStateTests::isKeyPressed (const OSXKeyState& keyState, KeyButton button) {
    // HACK: allow os to realize key state changes.
    ARCH->sleep (.2);

    IKeyState::KeyButtonSet pressed;
    keyState.pollPressedKeys (pressed);

    IKeyState::KeyButtonSet::const_iterator it;
    for (it = pressed.begin (); it != pressed.end (); ++it) {
        LOG ((CLOG_DEBUG "checking key %d", *it));
        if (*it == button) {
            return true;
        }
    }
    return false;
}

#endif
