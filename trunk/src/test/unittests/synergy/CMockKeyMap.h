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

#ifndef CMOCKKEYMAP_H
#define CMOCKKEYMAP_H

#include <gmock/gmock.h>
#include "CKeyMap.h"

class CMockKeyMap : public CKeyMap
{
public:
	MOCK_METHOD1(swap, void(CKeyMap&));
	MOCK_METHOD0(finish, void());
	MOCK_METHOD2(foreachKey, void(ForeachKeyCallback, void*));
	MOCK_METHOD1(addHalfDuplexModifier, void(KeyID));
	MOCK_CONST_METHOD2(isHalfDuplex, bool(KeyID, KeyButton));
	MOCK_CONST_METHOD7(mapKey, const CKeyMap::KeyItem*(
		Keystrokes&, KeyID, SInt32, ModifierToKeys&, KeyModifierMask&,
		KeyModifierMask, bool));
};

#endif
