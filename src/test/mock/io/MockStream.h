/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
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

#include "io/IStream.h"

#include "test/global/gmock.h"

class IEventQueue;

class MockStream : public synergy::IStream
{
public:
    MockStream() { }
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(UInt32, read, (void*, UInt32), (override));
    MOCK_METHOD(void, write, (const void*, UInt32), (override));
    MOCK_METHOD(void, flush, (), (override));
    MOCK_METHOD(void, shutdownInput, (), (override));
    MOCK_METHOD(void, shutdownOutput, (), (override));
    MOCK_METHOD(Event::Type, getInputReadyEvent, ());
    MOCK_METHOD(Event::Type, getOutputErrorEvent, ());
    MOCK_METHOD(Event::Type, getInputShutdownEvent, ());
    MOCK_METHOD(Event::Type, getOutputShutdownEvent, ());
    MOCK_METHOD(void*, getEventTarget, (), (const, override));
    MOCK_METHOD(bool, isReady, (), (const, override));
    MOCK_METHOD(UInt32, getSize, (), (const, override));
};
