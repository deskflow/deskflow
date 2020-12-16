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

#include "base/IEventQueue.h"

#include "test/global/gmock.h"

class MockEventQueue : public IEventQueue
{
public:
    MOCK_METHOD(void, loop, (), (override));
    MOCK_METHOD(EventQueueTimer*, newOneShotTimer, (double, void*), (override));
    MOCK_METHOD(EventQueueTimer*, newTimer, (double, void*), (override));
    MOCK_METHOD(bool, getEvent, (Event&, double), (override));
    MOCK_METHOD(void, adoptBuffer, (IEventQueueBuffer*), (override));
    MOCK_METHOD(Event::Type, registerTypeOnce, (Event::Type&, const char*), (override));
    MOCK_METHOD(void, removeHandlers, (void*), (override));
    MOCK_METHOD(Event::Type, registerType, (const char*));
    MOCK_METHOD(bool, isEmpty, (), (const, override));
    MOCK_METHOD(void, adoptHandler, (Event::Type, void*, IEventJob*), (override));
    MOCK_METHOD(const char*, getTypeName, (Event::Type), (override));
    MOCK_METHOD(void, addEvent, (const Event&), (override));
    MOCK_METHOD(void, removeHandler, (Event::Type, void*), (override));
    MOCK_METHOD(bool, dispatchEvent, (const Event&), (override));
    MOCK_METHOD(IEventJob*, getHandler, (Event::Type, void*), (const, override));
    MOCK_METHOD(void, deleteTimer, (EventQueueTimer*), (override));
    MOCK_METHOD(Event::Type, getRegisteredType, (const String&), (const, override));
    MOCK_METHOD(void*, getSystemTarget, (), (override));
    MOCK_METHOD(ClientEvents&, forClient, (), (override));
    MOCK_METHOD(IStreamEvents&, forIStream, (), (override));
    MOCK_METHOD(IpcClientEvents&, forIpcClient, (), (override));
    MOCK_METHOD(IpcClientProxyEvents&, forIpcClientProxy, (), (override));
    MOCK_METHOD(IpcServerEvents&, forIpcServer, (), (override));
    MOCK_METHOD(IpcServerProxyEvents&, forIpcServerProxy, (), (override));
    MOCK_METHOD(IDataSocketEvents&, forIDataSocket, (), (override));
    MOCK_METHOD(IListenSocketEvents&, forIListenSocket, (), (override));
    MOCK_METHOD(ISocketEvents&, forISocket, (), (override));
    MOCK_METHOD(OSXScreenEvents&, forOSXScreen, (), (override));
    MOCK_METHOD(ClientListenerEvents&, forClientListener, (), (override));
    MOCK_METHOD(ClientProxyEvents&, forClientProxy, (), (override));
    MOCK_METHOD(ClientProxyUnknownEvents&, forClientProxyUnknown, (), (override));
    MOCK_METHOD(ServerEvents&, forServer, (), (override));
    MOCK_METHOD(ServerAppEvents&, forServerApp, (), (override));
    MOCK_METHOD(IKeyStateEvents&, forIKeyState, (), (override));
    MOCK_METHOD(IPrimaryScreenEvents&, forIPrimaryScreen, (), (override));
    MOCK_METHOD(IScreenEvents&, forIScreen, (), (override));
    MOCK_METHOD(ClipboardEvents&, forClipboard, (), (override));
    MOCK_METHOD(FileEvents&, forFile, (), (override));
    MOCK_METHOD(void, waitForReady, (), (const, override));
};
