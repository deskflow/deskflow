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

#ifndef CKEYSTATEIMPL_H
#define CKEYSTATEIMPL_H

#include "CKeyState.h"

class CKeyStateImpl : public CKeyState
{
public:
	CKeyStateImpl() : CKeyState()
	{
	}

	CKeyStateImpl(IEventQueue* eventQueue) : CKeyState(eventQueue)
	{
	}

protected:
	virtual SInt32 pollActiveGroup() const
	{
		return 0;
	}

	virtual KeyModifierMask pollActiveModifiers() const
	{
		return 0;
	}

	virtual bool fakeCtrlAltDel() 
	{
		return false;
	}

	virtual void getKeyMap(CKeyMap& keyMap)
	{
	}

	virtual void fakeKey(const Keystroke& keystroke)
	{
	}

	virtual void pollPressedKeys(KeyButtonSet& pressedKeys) const
	{
	}
};

#endif
