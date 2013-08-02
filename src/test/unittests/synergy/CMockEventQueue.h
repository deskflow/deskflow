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

#ifndef CMOCKEVENTQUEUE_H 
#define CMOCKEVENTQUEUE_H

#include <gmock/gmock.h>
#include "IEventQueue.h"

class CMockEventQueue : public IEventQueue
{
public:
	MOCK_METHOD0(loop, void());
	MOCK_METHOD2(newOneShotTimer, CEventQueueTimer*(double, void*));
	MOCK_METHOD2(newTimer, CEventQueueTimer*(double, void*));
	MOCK_METHOD2(getEvent, bool(CEvent&, double));
	MOCK_METHOD1(adoptBuffer, void(IEventQueueBuffer*));
	MOCK_METHOD2(registerTypeOnce, CEvent::Type(CEvent::Type&, const char*));
	MOCK_METHOD1(removeHandlers, void(void*));
	MOCK_METHOD1(registerType, CEvent::Type(const char*));
	MOCK_CONST_METHOD0(isEmpty, bool());
	MOCK_METHOD3(adoptHandler, void(CEvent::Type, void*, IEventJob*));
	MOCK_METHOD1(getTypeName, const char*(CEvent::Type));
	MOCK_METHOD1(addEvent, void(const CEvent&));
	MOCK_METHOD2(removeHandler, void(CEvent::Type, void*));
	MOCK_METHOD1(dispatchEvent, bool(const CEvent&));
	MOCK_CONST_METHOD2(getHandler, IEventJob*(CEvent::Type, void*));
	MOCK_METHOD1(deleteTimer, void(CEventQueueTimer*));
	MOCK_CONST_METHOD1(getRegisteredType, CEvent::Type(const CString&));
	MOCK_METHOD0(getSystemTarget, void*());
	MOCK_METHOD0(forCClient, CClientEvents&());
	MOCK_METHOD0(forIStream, IStreamEvents&());
	MOCK_METHOD0(forCIpcClient, CIpcClientEvents&());
	MOCK_METHOD0(forCIpcClientProxy, CIpcClientProxyEvents&());
	MOCK_METHOD0(forCIpcServer, CIpcServerEvents&());
	MOCK_METHOD0(forCIpcServerProxy, CIpcServerProxyEvents&());
	MOCK_METHOD0(forIDataSocket, IDataSocketEvents&());
	MOCK_METHOD0(forIListenSocket, IListenSocketEvents&());
	MOCK_METHOD0(forISocket, ISocketEvents&());
	MOCK_METHOD0(forCOSXScreen, COSXScreenEvents&());
	MOCK_METHOD0(forCClientListener, CClientListenerEvents&());
	MOCK_METHOD0(forCClientProxy, CClientProxyEvents&());
	MOCK_METHOD0(forCClientProxyUnknown, CClientProxyUnknownEvents&());
	MOCK_METHOD0(forCServer, CServerEvents&());
	MOCK_METHOD0(forCServerApp, CServerAppEvents&());
	MOCK_METHOD0(forIKeyState, IKeyStateEvents&());
	MOCK_METHOD0(forIPrimaryScreen, IPrimaryScreenEvents&());
	MOCK_METHOD0(forIScreen, IScreenEvents&());
};

#endif
