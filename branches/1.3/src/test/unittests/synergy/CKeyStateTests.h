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

#ifndef CKEYSTATETESTS_H
#define CKEYSTATETESTS_H

#include "CKeyState.h"
#include "gmock/gmock.h"

class CMockKeyMap;
class CMockEventQueue;

// while the class name indicates that this is actually a mock, we use a 
// typedef later to rename it (so the name matches the compilation unit)
// so the tests are less confusing.
class CMockKeyState : public CKeyState
{
public:
	CMockKeyState() : CKeyState()
	{
	}

	CMockKeyState(const CMockEventQueue& eventQueue, const CMockKeyMap& keyMap) :
		CKeyState((IEventQueue&)eventQueue, (CKeyMap&)keyMap)
	{
	}

	MOCK_CONST_METHOD0(pollActiveGroup, SInt32());
	MOCK_CONST_METHOD0(pollActiveModifiers, KeyModifierMask());
	MOCK_METHOD0(fakeCtrlAltDel, bool());
	MOCK_METHOD1(getKeyMap, void(CKeyMap&));
	MOCK_METHOD1(fakeKey, void(const Keystroke&));
	MOCK_CONST_METHOD1(pollPressedKeys, void(KeyButtonSet&));
	MOCK_METHOD4(fakeKeyRepeat, void(KeyID, KeyModifierMask, SInt32, KeyButton));
};

// hide that we're actually testing a mock to make the unit tests less 
// confusing. use NiceMock so that we don't get warnings for unexpected
// calls.
typedef ::testing::NiceMock<CMockKeyState> CKeyStateImpl;

typedef UInt32 KeyID;

typedef void (*ForeachKeyCallback)(
		KeyID, SInt32 group, CKeyMap::KeyItem&, void* userData);

enum {
	kAKey = 30
};

#endif
