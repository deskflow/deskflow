/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#include <gmock/gmock.h>

#define TEST_ENV
#include "CScreen.h"

class CMockScreen : public CScreen
{
public:
	CMockScreen() : CScreen() { }
	MOCK_METHOD0(disable, void());
	MOCK_CONST_METHOD4(getShape, void(SInt32&, SInt32&, SInt32&, SInt32&));
	MOCK_CONST_METHOD2(getCursorPos, void(SInt32&, SInt32&));
	MOCK_METHOD0(resetOptions, void());
	MOCK_METHOD1(setOptions, void(const COptionsList&));
	MOCK_METHOD0(enable, void());
};
