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

#include "CEventTypes.h"
#include "IEventQueue.h"
#include <assert.h>
#include <stddef.h>

CEventTypes::CEventTypes() :
	m_events(NULL)
{
}

IEventQueue*
CEventTypes::getEvents() const
{
	assert(m_events != NULL);
	return m_events;
}

void
CEventTypes::setEvents(IEventQueue* events)
{
	m_events = events;
}

//
// CClient
//

REGISTER_EVENT(CClient, connected)
REGISTER_EVENT(CClient, connectionFailed)
REGISTER_EVENT(CClient, disconnected)

//
// IStream
//

REGISTER_EVENT(IStream, inputReady)
REGISTER_EVENT(IStream, outputFlushed)
REGISTER_EVENT(IStream, outputError)
REGISTER_EVENT(IStream, inputShutdown)
REGISTER_EVENT(IStream, outputShutdown)

//
// CIpcClient
//

REGISTER_EVENT(CIpcClient, connected)
REGISTER_EVENT(CIpcClient, messageReceived)

//
// CIpcClientProxy
//

REGISTER_EVENT(CIpcClientProxy, messageReceived)
REGISTER_EVENT(CIpcClientProxy, disconnected)

//
// CIpcServerProxy
//

REGISTER_EVENT(CIpcServerProxy, messageReceived)

//
// IDataSocket
//

REGISTER_EVENT(IDataSocket, connected)
REGISTER_EVENT(IDataSocket, connectionFailed)

//
// IListenSocket
//

REGISTER_EVENT(IListenSocket, connecting)

//
// ISocket
//

REGISTER_EVENT(ISocket, disconnected)

//
// COSXScreen
//

REGISTER_EVENT(COSXScreen, confirmSleep)

//
// CClientListener
//

REGISTER_EVENT(CClientListener, connected)

//
// CClientProxy
//

REGISTER_EVENT(CClientProxy, ready)
REGISTER_EVENT(CClientProxy, disconnected)
REGISTER_EVENT(CClientProxy, clipboardChanged)

//
// CClientProxyUnknown
//

REGISTER_EVENT(CClientProxyUnknown, success)
REGISTER_EVENT(CClientProxyUnknown, failure)

//
// CServer
//

REGISTER_EVENT(CServer, error)
REGISTER_EVENT(CServer, connected)
REGISTER_EVENT(CServer, disconnected)
REGISTER_EVENT(CServer, switchToScreen)
REGISTER_EVENT(CServer, switchInDirection)
REGISTER_EVENT(CServer, keyboardBroadcast)
REGISTER_EVENT(CServer, lockCursorToScreen)
REGISTER_EVENT(CServer, screenSwitched)

//
// CServerApp
//

REGISTER_EVENT(CServerApp, reloadConfig)
REGISTER_EVENT(CServerApp, forceReconnect)
REGISTER_EVENT(CServerApp, resetServer)

//
// IKeyState
//

REGISTER_EVENT(IKeyState, keyDown)
REGISTER_EVENT(IKeyState, keyUp)
REGISTER_EVENT(IKeyState, keyRepeat)

//
// IPrimaryScreen
//

REGISTER_EVENT(IPrimaryScreen, buttonDown)
REGISTER_EVENT(IPrimaryScreen, buttonUp)
REGISTER_EVENT(IPrimaryScreen, motionOnPrimary)
REGISTER_EVENT(IPrimaryScreen, motionOnSecondary)
REGISTER_EVENT(IPrimaryScreen, wheel)
REGISTER_EVENT(IPrimaryScreen, screensaverActivated)
REGISTER_EVENT(IPrimaryScreen, screensaverDeactivated)
REGISTER_EVENT(IPrimaryScreen, hotKeyDown)
REGISTER_EVENT(IPrimaryScreen, hotKeyUp)
REGISTER_EVENT(IPrimaryScreen, fakeInputBegin)
REGISTER_EVENT(IPrimaryScreen, fakeInputEnd)

//
// IScreen
//

REGISTER_EVENT(IScreen, error)
REGISTER_EVENT(IScreen, shapeChanged)
REGISTER_EVENT(IScreen, clipboardGrabbed)
REGISTER_EVENT(IScreen, suspend)
REGISTER_EVENT(IScreen, resume)
REGISTER_EVENT(IScreen, fileChunkSending)
REGISTER_EVENT(IScreen, fileRecieveCompleted)

//
// CIpcServer
//

REGISTER_EVENT(CIpcServer, clientConnected)
REGISTER_EVENT(CIpcServer, messageReceived)
