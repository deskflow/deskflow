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

#include <gtest/gtest.h>
#include "CKeyState.h"

enum {
	kAKey = 30
};

class CKeyStateImpl : public CKeyState
{
protected:
	virtual SInt32 pollActiveGroup() const
	{
		throw std::exception("The method or operation is not implemented.");
	}

	virtual KeyModifierMask pollActiveModifiers() const
	{
		throw std::exception("The method or operation is not implemented.");
	}

	virtual bool fakeCtrlAltDel() 
	{
		throw std::exception("The method or operation is not implemented.");
	}

	virtual void getKeyMap( CKeyMap& keyMap ) 
	{
		throw std::exception("The method or operation is not implemented.");
	}

	virtual void fakeKey( const Keystroke& keystroke ) 
	{
		throw std::exception("The method or operation is not implemented.");
	}

	virtual void pollPressedKeys( KeyButtonSet& pressedKeys ) const
	{
		throw std::exception("The method or operation is not implemented.");
	}
};

TEST(CKeyStateTests, onKey_aKeyPressed_keyStateOne)
{
	CKeyStateImpl keyState;

	keyState.onKey(kAKey, true, KeyModifierAlt);

	EXPECT_EQ(1, keyState.getKeyState(kAKey));
}

TEST(CKeyStateTests, onKey_validButtonUp_keyStateZero)
{
	CKeyStateImpl keyState;

	keyState.onKey(0, true, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(0));
}

TEST(CKeyStateTests, onKey_bogusButtonDown_keyStateZero)
{
	CKeyStateImpl keyState;

	keyState.onKey(0, true, KeyModifierAlt);

	EXPECT_EQ(0, keyState.getKeyState(0));
}
