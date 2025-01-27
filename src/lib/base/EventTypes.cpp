/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/EventTypes.h"
#include "base/IEventQueue.h"

#include <assert.h>
#include <stddef.h>

EventTypes::EventTypes() : m_events(NULL)
{
}

IEventQueue *EventTypes::getEvents() const
{
  assert(m_events != NULL);
  return m_events;
}

void EventTypes::setEvents(IEventQueue *events)
{
  m_events = events;
}

//
// Client
//

REGISTER_EVENT(Client, connected)
REGISTER_EVENT(Client, connectionFailed)
REGISTER_EVENT(Client, connectionRefused)
REGISTER_EVENT(Client, disconnected)

//
// IStream
//

REGISTER_EVENT(IStream, inputReady)
REGISTER_EVENT(IStream, outputFlushed)
REGISTER_EVENT(IStream, outputError)
REGISTER_EVENT(IStream, inputShutdown)
REGISTER_EVENT(IStream, outputShutdown)
REGISTER_EVENT(IStream, inputFormatError)

//
// IpcClient
//

REGISTER_EVENT(IpcClient, connected)
REGISTER_EVENT(IpcClient, messageReceived)

//
// IpcClientProxy
//

REGISTER_EVENT(IpcClientProxy, messageReceived)
REGISTER_EVENT(IpcClientProxy, disconnected)

//
// IpcServerProxy
//

REGISTER_EVENT(IpcServerProxy, messageReceived)

//
// IDataSocket
//

REGISTER_EVENT(IDataSocket, connected)
REGISTER_EVENT(IDataSocket, secureConnected)
REGISTER_EVENT(IDataSocket, connectionFailed)

//
// IListenSocket
//

REGISTER_EVENT(IListenSocket, connecting)

//
// ISocket
//

REGISTER_EVENT(ISocket, disconnected)
REGISTER_EVENT(ISocket, stopRetry)

//
// OSXScreen
//

REGISTER_EVENT(OSXScreen, confirmSleep)

//
// ClientListener
//

REGISTER_EVENT(ClientListener, accepted)
REGISTER_EVENT(ClientListener, connected)

//
// ClientProxy
//

REGISTER_EVENT(ClientProxy, ready)
REGISTER_EVENT(ClientProxy, disconnected)

//
// ClientProxyUnknown
//

REGISTER_EVENT(ClientProxyUnknown, success)
REGISTER_EVENT(ClientProxyUnknown, failure)

//
// Server
//

REGISTER_EVENT(Server, error)
REGISTER_EVENT(Server, connected)
REGISTER_EVENT(Server, disconnected)
REGISTER_EVENT(Server, switchToScreen)
REGISTER_EVENT(Server, switchInDirection)
REGISTER_EVENT(Server, keyboardBroadcast)
REGISTER_EVENT(Server, lockCursorToScreen)
REGISTER_EVENT(Server, screenSwitched)

//
// ServerApp
//

REGISTER_EVENT(ServerApp, reloadConfig)
REGISTER_EVENT(ServerApp, forceReconnect)
REGISTER_EVENT(ServerApp, resetServer)

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
REGISTER_EVENT(IScreen, suspend)
REGISTER_EVENT(IScreen, resume)

//
// IpcServer
//

REGISTER_EVENT(IpcServer, clientConnected)
REGISTER_EVENT(IpcServer, messageReceived)

//
// Clipboard
//

REGISTER_EVENT(Clipboard, clipboardGrabbed)
REGISTER_EVENT(Clipboard, clipboardChanged)
REGISTER_EVENT(Clipboard, clipboardSending)

//
// File
//

REGISTER_EVENT(File, fileChunkSending)
REGISTER_EVENT(File, fileRecieveCompleted)
REGISTER_EVENT(File, keepAlive)

//
// Ei
//

REGISTER_EVENT(Ei, connected)
REGISTER_EVENT(Ei, sessionClosed)
