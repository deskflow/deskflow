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

#include "CKeyState.h"

class CKeyStateImpl : public CKeyState
{
public:
	CKeyStateImpl() : CKeyState()
	{
	}

	CKeyStateImpl(const CMockEventQueue& eventQueue, const CMockKeyMap& keyMap) :
		CKeyState((IEventQueue&)eventQueue, (CKeyMap&)keyMap)
	{
	}

	SInt32 pollActiveGroup() const { return 0; }
	KeyModifierMask pollActiveModifiers() const { return 0; }
	bool fakeCtrlAltDel() { return false; }
	void getKeyMap(CKeyMap&) { }
	void fakeKey(const Keystroke&) { }
	void pollPressedKeys(KeyButtonSet&) const { }
};
