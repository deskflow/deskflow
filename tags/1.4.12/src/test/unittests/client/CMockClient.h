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

#pragma once

#include <gmock/gmock.h>

#define TEST_ENV
#include "CClient.h"

class IEventQueue;

class CMockClient : public CClient
{
public:
	CMockClient() { m_mock = true; }
	MOCK_METHOD2(mouseMove, void(SInt32, SInt32));
	MOCK_METHOD1(setOptions, void(const COptionsList&));
	MOCK_METHOD0(handshakeComplete, void());
	MOCK_METHOD1(setDecryptIv, void(const UInt8*));
};
