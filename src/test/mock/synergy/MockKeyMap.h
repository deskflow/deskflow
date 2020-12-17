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

#include "synergy/KeyMap.h"

#include "test/global/gmock.h"

class MockKeyMap : public synergy::KeyMap
{
public:
    MOCK_METHOD(void, swap, (KeyMap&), (override));
    MOCK_METHOD(void, finish, (), (override));
    MOCK_METHOD(void, foreachKey, (ForeachKeyCallback, void*), (override));
    MOCK_METHOD(void, addHalfDuplexModifier, (KeyID), (override));
    MOCK_METHOD(bool, isHalfDuplex, (KeyID, KeyButton), (const, override));
    MOCK_METHOD(const KeyMap::KeyItem*, mapKey, (Keystrokes&, KeyID, SInt32, ModifierToKeys&, KeyModifierMask&,
        KeyModifierMask, bool), (const, override));
};
