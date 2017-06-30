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

#include "test/global/gtest.h"
#include "test/global/gmock.h"

TEST (OSXKeyStateTests, mapModifiersFromOSX_OSXMask_returnSynergyMask) {
    synergy::KeyMap keyMap;
    MockEventQueue eventQueue;
    OSXKeyState keyState (&eventQueue, keyMap);

    KeyModifierMask outMask = 0;

    UInt32 shiftMask = 0 | kCGEventFlagMaskShift;
    outMask          = keyState.mapModifiersFromOSX (shiftMask);
    EXPECT_EQ (KeyModifierShift, outMask);

    UInt32 ctrlMask = 0 | kCGEventFlagMaskControl;
    outMask         = keyState.mapModifiersFromOSX (ctrlMask);
    EXPECT_EQ (KeyModifierControl, outMask);

    UInt32 altMask = 0 | kCGEventFlagMaskAlternate;
    outMask        = keyState.mapModifiersFromOSX (altMask);
    EXPECT_EQ (KeyModifierAlt, outMask);

    UInt32 cmdMask = 0 | kCGEventFlagMaskCommand;
    outMask        = keyState.mapModifiersFromOSX (cmdMask);
    EXPECT_EQ (KeyModifierSuper, outMask);

    UInt32 capsMask = 0 | kCGEventFlagMaskAlphaShift;
    outMask         = keyState.mapModifiersFromOSX (capsMask);
    EXPECT_EQ (KeyModifierCapsLock, outMask);

    UInt32 numMask = 0 | kCGEventFlagMaskNumericPad;
    outMask        = keyState.mapModifiersFromOSX (numMask);
    EXPECT_EQ (KeyModifierNumLock, outMask);
}
