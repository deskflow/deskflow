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
#include "CPrimaryClient.h"
#include "CString.h"

class CMockPrimaryClient : public CPrimaryClient
{
public:
	MOCK_CONST_METHOD0(getEventTarget, void*());
	MOCK_CONST_METHOD2(getCursorPos, void(SInt32&, SInt32&));
	MOCK_CONST_METHOD2(setJumpCursorPos, void(SInt32, SInt32));
	MOCK_METHOD1(reconfigure, void(UInt32));
	MOCK_METHOD0(resetOptions, void());
	MOCK_METHOD1(setOptions, void(const COptionsList&));
	MOCK_METHOD0(enable, void());
	MOCK_METHOD0(disable, void());
	MOCK_METHOD2(registerHotKey, UInt32(KeyID, KeyModifierMask));
	MOCK_CONST_METHOD0(getToggleMask, KeyModifierMask());
	MOCK_METHOD1(unregisterHotKey, void(UInt32));
};
