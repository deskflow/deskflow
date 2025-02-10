/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"

#include <gmock/gmock.h>

class MockEventQueue : public IEventQueue
{
public:
  MOCK_METHOD(void, loop, (), (override));
  MOCK_METHOD(EventQueueTimer *, newOneShotTimer, (double, void *), (override));
  MOCK_METHOD(EventQueueTimer *, newTimer, (double, void *), (override));
  MOCK_METHOD(bool, getEvent, (Event &, double), (override));
  MOCK_METHOD(void, adoptBuffer, (IEventQueueBuffer *), (override));
  MOCK_METHOD(Event::Type, registerTypeOnce, (Event::Type &, const char *), (override));
  MOCK_METHOD(void, removeHandlers, (void *), (override));
  MOCK_METHOD(Event::Type, registerType, (const char *));
  MOCK_METHOD(bool, isEmpty, (), (const, override));
  MOCK_METHOD(void, adoptHandler, (Event::Type, void *, IEventJob *), (override));
  MOCK_METHOD(const char *, getTypeName, (Event::Type), (override));
  MOCK_METHOD(void, addEvent, (const Event &), (override));
  MOCK_METHOD(void, removeHandler, (Event::Type, void *), (override));
  MOCK_METHOD(bool, dispatchEvent, (const Event &), (override));
  MOCK_METHOD(IEventJob *, getHandler, (Event::Type, void *), (const, override));
  MOCK_METHOD(void, deleteTimer, (EventQueueTimer *), (override));
  MOCK_METHOD(Event::Type, getRegisteredType, (const std::string &), (const, override));
  MOCK_METHOD(void *, getSystemTarget, (), (override));
  MOCK_METHOD(ClientEvents &, forClient, (), (override));
  MOCK_METHOD(IStreamEvents &, forIStream, (), (override));
  MOCK_METHOD(IDataSocketEvents &, forIDataSocket, (), (override));
  MOCK_METHOD(IListenSocketEvents &, forIListenSocket, (), (override));
  MOCK_METHOD(ISocketEvents &, forISocket, (), (override));
  MOCK_METHOD(OSXScreenEvents &, forOSXScreen, (), (override));
  MOCK_METHOD(ClientListenerEvents &, forClientListener, (), (override));
  MOCK_METHOD(ClientProxyEvents &, forClientProxy, (), (override));
  MOCK_METHOD(ClientProxyUnknownEvents &, forClientProxyUnknown, (), (override));
  MOCK_METHOD(ServerEvents &, forServer, (), (override));
  MOCK_METHOD(ServerAppEvents &, forServerApp, (), (override));
  MOCK_METHOD(IKeyStateEvents &, forIKeyState, (), (override));
  MOCK_METHOD(IPrimaryScreenEvents &, forIPrimaryScreen, (), (override));
  MOCK_METHOD(IScreenEvents &, forIScreen, (), (override));
  MOCK_METHOD(ClipboardEvents &, forClipboard, (), (override));
  MOCK_METHOD(FileEvents &, forFile, (), (override));
  MOCK_METHOD(EiEvents &, forEi, (), (override));
  MOCK_METHOD(void, waitForReady, (), (const, override));
};
