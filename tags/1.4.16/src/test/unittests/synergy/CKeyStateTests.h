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

#ifndef CKEYSTATETESTS_H
#define CKEYSTATETESTS_H

#include "CKeyState.h"
#include "gmock/gmock.h"

class CMockKeyMap;
class CMockEventQueue;

// NOTE: do not mock methods that are not pure virtual. this mock exists only
// to provide an implementation of the CKeyState abstract class.
class CMockKeyState : public CKeyState
{
public:
	CMockKeyState(const CMockEventQueue& eventQueue) :
		CKeyState((IEventQueue*)&eventQueue)
	{
	}

	CMockKeyState(const CMockEventQueue& eventQueue, const CMockKeyMap& keyMap) :
		CKeyState((IEventQueue*)&eventQueue, (CKeyMap&)keyMap)
	{
	}

	MOCK_CONST_METHOD0(pollActiveGroup, SInt32());
	MOCK_CONST_METHOD0(pollActiveModifiers, KeyModifierMask());
	MOCK_METHOD0(fakeCtrlAltDel, bool());
	MOCK_METHOD1(getKeyMap, void(CKeyMap&));
	MOCK_METHOD1(fakeKey, void(const Keystroke&));
	MOCK_CONST_METHOD1(pollPressedKeys, void(KeyButtonSet&));
};

typedef ::testing::NiceMock<CMockKeyState> CKeyStateImpl;

typedef UInt32 KeyID;

typedef void (*ForeachKeyCallback)(
		KeyID, SInt32 group, CKeyMap::KeyItem&, void* userData);

void
stubPollPressedKeys(IKeyState::KeyButtonSet& pressedKeys);

void
assertMaskIsOne(ForeachKeyCallback cb, void* userData);

const CKeyMap::KeyItem*
stubMapKey(
	CKeyMap::Keystrokes& keys, KeyID id, SInt32 group,
	CKeyMap::ModifierToKeys& activeModifiers,
	KeyModifierMask& currentState,
	KeyModifierMask desiredMask,
	bool isAutoRepeat);

CKeyMap::Keystroke s_stubKeystroke(1, false, false);
CKeyMap::KeyItem s_stubKeyItem;

#endif
