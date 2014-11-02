/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
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

#pragma once

#define TEST_ENV

#include "synergy/Screen.h"

#include "test/global/gmock.h"

class MockScreen : public Screen
{
public:
	MockScreen() : Screen() { }
	MOCK_METHOD0(disable, void());
	MOCK_CONST_METHOD4(getShape, void(SInt32&, SInt32&, SInt32&, SInt32&));
	MOCK_CONST_METHOD2(getCursorPos, void(SInt32&, SInt32&));
	MOCK_METHOD0(resetOptions, void());
	MOCK_METHOD1(setOptions, void(const OptionsList&));
	MOCK_METHOD0(enable, void());
};
