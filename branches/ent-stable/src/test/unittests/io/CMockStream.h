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
#include "IStream.h"

class IEventQueue;

class CMockStream : public synergy::IStream
{
public:
	CMockStream() : synergy::IStream(NULL) { }
	MOCK_METHOD0(close, void());
	MOCK_METHOD2(read, UInt32(void*, UInt32));
	MOCK_METHOD2(write, void(const void*, UInt32));
	MOCK_METHOD0(flush, void());
	MOCK_METHOD0(shutdownInput, void());
	MOCK_METHOD0(shutdownOutput, void());
	MOCK_METHOD0(getInputReadyEvent, CEvent::Type());
	MOCK_METHOD0(getOutputErrorEvent, CEvent::Type());
	MOCK_METHOD0(getInputShutdownEvent, CEvent::Type());
	MOCK_METHOD0(getOutputShutdownEvent, CEvent::Type());
	MOCK_CONST_METHOD0(getEventTarget, void*());
	MOCK_CONST_METHOD0(isReady, bool());
	MOCK_CONST_METHOD0(getSize, UInt32());
};
