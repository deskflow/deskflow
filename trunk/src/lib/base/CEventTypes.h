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

#include "CEvent.h"

class IEventQueue;

class CEventTypes {
public:
	CEventTypes();
	void				setEvents(IEventQueue* events);

protected:
	IEventQueue*		getEvents() const;

private:
	IEventQueue*		m_events;
};

#define REGISTER_EVENT(type_, name_)									\
CEvent::Type															\
type_##Events::name_()													\
{																		\
	return getEvents()->registerTypeOnce(m_##name_, __FUNCTION__);			\
}

class CClientEvents : public CEventTypes {
public:
	CClientEvents() :
		m_connected(CEvent::kUnknown),
		m_connectionFailed(CEvent::kUnknown),
		m_disconnected(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get connected event type
	/*!
	Returns the connected event type.  This is sent when the client has
	successfully connected to the server.
	*/
	CEvent::Type		connected();

	//! Get connection failed event type
	/*!
	Returns the connection failed event type.  This is sent when the
	server fails for some reason.  The event data is a CFailInfo*.
	*/
	CEvent::Type		connectionFailed();

	//! Get disconnected event type
	/*!
	Returns the disconnected event type.  This is sent when the client
	has disconnected from the server (and only after having successfully
	connected).
	*/
	CEvent::Type		disconnected();

	//@}

private:
	CEvent::Type		m_connected;
	CEvent::Type		m_connectionFailed;
	CEvent::Type		m_disconnected;
};

class IStreamEvents : public CEventTypes {
public:
	IStreamEvents() :
		m_inputReady(CEvent::kUnknown),
		m_outputFlushed(CEvent::kUnknown),
		m_outputError(CEvent::kUnknown),
		m_inputShutdown(CEvent::kUnknown),
		m_outputShutdown(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get input ready event type
	/*!
	Returns the input ready event type.  A stream sends this event
	when \c read() will return with data.
	*/
	CEvent::Type		inputReady();

	//! Get output flushed event type
	/*!
	Returns the output flushed event type.  A stream sends this event
	when the output buffer has been flushed.  If there have been no
	writes since the event was posted, calling \c shutdownOutput() or
	\c close() will not discard any data and \c flush() will return
	immediately.
	*/
	CEvent::Type		outputFlushed();

	//! Get output error event type
	/*!
	Returns the output error event type.  A stream sends this event
	when a write has failed.
	*/
	CEvent::Type		outputError();

	//! Get input shutdown event type
	/*!
	Returns the input shutdown event type.  This is sent when the
	input side of the stream has shutdown.  When the input has
	shutdown, no more data will ever be available to read.
	*/
	CEvent::Type		inputShutdown();

	//! Get output shutdown event type
	/*!
	Returns the output shutdown event type.  This is sent when the
	output side of the stream has shutdown.  When the output has
	shutdown, no more data can ever be written to the stream.  Any
	attempt to do so will generate a output error event.
	*/
	CEvent::Type		outputShutdown();

	//@}
		
private:
	CEvent::Type		m_inputReady;
	CEvent::Type		m_outputFlushed;
	CEvent::Type		m_outputError;
	CEvent::Type		m_inputShutdown;
	CEvent::Type		m_outputShutdown;
};

class CIpcClientEvents : public CEventTypes {
public:
	CIpcClientEvents() :
		m_connected(CEvent::kUnknown),
		m_messageReceived(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Raised when the socket is connected.
	CEvent::Type		connected();

	//! Raised when a message is received.
	CEvent::Type		messageReceived();

	//@}
	
private:
	CEvent::Type		m_connected;
	CEvent::Type		m_messageReceived;
};

class CIpcClientProxyEvents : public CEventTypes {
public:
	CIpcClientProxyEvents() :
		m_messageReceived(CEvent::kUnknown),
		m_disconnected(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Raised when the server receives a message from a client.
	CEvent::Type		messageReceived();

	//! Raised when the client disconnects from the server.
	CEvent::Type		disconnected();
		
	//@}

private:
	CEvent::Type		m_messageReceived;
	CEvent::Type		m_disconnected;
};

class CIpcServerEvents : public CEventTypes {
public:
	CIpcServerEvents() :
		m_clientConnected(CEvent::kUnknown),
		m_messageReceived(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Raised when we have created the client proxy.
	CEvent::Type		clientConnected();
	
	//! Raised when a message is received through a client proxy.
	CEvent::Type		messageReceived();

	//@}

private:
	CEvent::Type		m_clientConnected;
	CEvent::Type		m_messageReceived;
};

class CIpcServerProxyEvents : public CEventTypes {
public:
	CIpcServerProxyEvents() :
		m_messageReceived(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Raised when the client receives a message from the server.
	CEvent::Type		messageReceived();

	//@}

private:
	CEvent::Type		m_messageReceived;
};

class IDataSocketEvents : public CEventTypes {
public:
	IDataSocketEvents() :
		m_connected(CEvent::kUnknown),
		m_connectionFailed(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get connected event type
	/*!
	Returns the socket connected event type.  A socket sends this
	event when a remote connection has been established.
	*/
	CEvent::Type		connected();

	//! Get connection failed event type
	/*!
	Returns the socket connection failed event type.  A socket sends
	this event when an attempt to connect to a remote port has failed.
	The data is a pointer to a CConnectionFailedInfo.
	*/
	CEvent::Type		connectionFailed();

	//@}

private:
	CEvent::Type		m_connected;
	CEvent::Type		m_connectionFailed;
};

class IListenSocketEvents : public CEventTypes {
public:
	IListenSocketEvents() :
		m_connecting(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get connecting event type
	/*!
	Returns the socket connecting event type.  A socket sends this
	event when a remote connection is waiting to be accepted.
	*/
	CEvent::Type		connecting();

	//@}

private:
	CEvent::Type		m_connecting;
};

class ISocketEvents : public CEventTypes {
public:
	ISocketEvents() :
		m_disconnected(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get disconnected event type
	/*!
	Returns the socket disconnected event type.  A socket sends this
	event when the remote side of the socket has disconnected or
	shutdown both input and output.
	*/
	CEvent::Type		disconnected();

	//@}

private:
	CEvent::Type		m_disconnected;
};

class COSXScreenEvents : public CEventTypes {
public:
	COSXScreenEvents() :
		m_confirmSleep(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	CEvent::Type		confirmSleep();

	//@}

private:
	CEvent::Type		m_confirmSleep;
};

class CClientListenerEvents : public CEventTypes {
public:
	CClientListenerEvents() :
		m_connected(CEvent::kUnknown) { }

	//! @name accessors
	//@{
		
	//! Get connected event type
	/*!
	Returns the connected event type.  This is sent whenever a
	a client connects.
	*/
	CEvent::Type		connected();

	//@}

private:
	CEvent::Type		m_connected;
};

class CClientProxyEvents : public CEventTypes {
public:
	CClientProxyEvents() :
		m_ready(CEvent::kUnknown),
		m_disconnected(CEvent::kUnknown),
		m_clipboardChanged(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get ready event type
	/*!
	Returns the ready event type.  This is sent when the client has
	completed the initial handshake.  Until it is sent, the client is
	not fully connected.
	*/
	CEvent::Type		ready();

	//! Get disconnect event type
	/*!
	Returns the disconnect event type.  This is sent when the client
	disconnects or is disconnected.  The target is getEventTarget().
	*/
	CEvent::Type		disconnected();

	//! Get clipboard changed event type
	/*!
	Returns the clipboard changed event type.  This is sent whenever the
	contents of the clipboard has changed.  The data is a pointer to a
	IScreen::CClipboardInfo.
	*/
	CEvent::Type		clipboardChanged();

	//@}

private:
	CEvent::Type		m_ready;
	CEvent::Type		m_disconnected;
	CEvent::Type		m_clipboardChanged;
};

class CClientProxyUnknownEvents : public CEventTypes {
public:
	CClientProxyUnknownEvents() :
		m_success(CEvent::kUnknown),
		m_failure(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get success event type
	/*!
	Returns the success event type.  This is sent when the client has
	correctly responded to the hello message.  The target is this.
	*/
	CEvent::Type		success();

	//! Get failure event type
	/*!
	Returns the failure event type.  This is sent when a client fails
	to correctly respond to the hello message.  The target is this.
	*/
	CEvent::Type		failure();

	//@}
		
private:
	CEvent::Type		m_success;
	CEvent::Type		m_failure;
};

class CServerEvents : public CEventTypes {
public:
	CServerEvents() :
		m_error(CEvent::kUnknown),
		m_connected(CEvent::kUnknown),
		m_disconnected(CEvent::kUnknown),
		m_switchToScreen(CEvent::kUnknown),
		m_switchInDirection(CEvent::kUnknown),
		m_keyboardBroadcast(CEvent::kUnknown),
		m_lockCursorToScreen(CEvent::kUnknown),
		m_screenSwitched(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get error event type
	/*!
	Returns the error event type.  This is sent when the server fails
	for some reason.
	*/
	CEvent::Type		error();

	//! Get connected event type
	/*!
	Returns the connected event type.  This is sent when a client screen
	has connected.  The event data is a \c CScreenConnectedInfo* that
	indicates the connected screen.
	*/
	CEvent::Type		connected();

	//! Get disconnected event type
	/*!
	Returns the disconnected event type.  This is sent when all the
	clients have disconnected.
	*/
	CEvent::Type		disconnected();

	//! Get switch to screen event type
	/*!
	Returns the switch to screen event type.  The server responds to this
	by switching screens.  The event data is a \c CSwitchToScreenInfo*
	that indicates the target screen.
	*/
	CEvent::Type		switchToScreen();

	//! Get switch in direction event type
	/*!
	Returns the switch in direction event type.  The server responds to this
	by switching screens.  The event data is a \c CSwitchInDirectionInfo*
	that indicates the target direction.
	*/
	CEvent::Type		switchInDirection();

	//! Get keyboard broadcast event type
	/*!
	Returns the keyboard broadcast event type.  The server responds
	to this by turning on keyboard broadcasting or turning it off.  The
	event data is a \c CKeyboardBroadcastInfo*.
	*/
	CEvent::Type		keyboardBroadcast();

	//! Get lock cursor event type
	/*!
	Returns the lock cursor event type.  The server responds to this
	by locking the cursor to the active screen or unlocking it.  The
	event data is a \c CLockCursorToScreenInfo*.
	*/
	CEvent::Type		lockCursorToScreen();

	//! Get screen switched event type
	/*!
	Returns the screen switched event type.  This is raised when the
	screen has been switched to a client.
	*/
	CEvent::Type		screenSwitched();

	//@}
		
private:
	CEvent::Type		m_error;
	CEvent::Type		m_connected;
	CEvent::Type		m_disconnected;
	CEvent::Type		m_switchToScreen;
	CEvent::Type		m_switchInDirection;
	CEvent::Type		m_keyboardBroadcast;
	CEvent::Type		m_lockCursorToScreen;
	CEvent::Type		m_screenSwitched;
};

class CServerAppEvents : public CEventTypes {
public:
	CServerAppEvents() :
		m_reloadConfig(CEvent::kUnknown),
		m_forceReconnect(CEvent::kUnknown),
		m_resetServer(CEvent::kUnknown) { }
		
	//! @name accessors
	//@{
		
	CEvent::Type		reloadConfig();
	CEvent::Type		forceReconnect();
	CEvent::Type		resetServer();

	//@}
		
private:
	CEvent::Type		m_reloadConfig;
	CEvent::Type		m_forceReconnect;
	CEvent::Type		m_resetServer;
};

class IKeyStateEvents : public CEventTypes {
public:
	IKeyStateEvents() :
		m_keyDown(CEvent::kUnknown),
		m_keyUp(CEvent::kUnknown),
		m_keyRepeat(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get key down event type.  Event data is CKeyInfo*, count == 1.
	CEvent::Type		keyDown();

	//! Get key up event type.  Event data is CKeyInfo*, count == 1.
	CEvent::Type		keyUp();

	//! Get key repeat event type.  Event data is CKeyInfo*.
	CEvent::Type		keyRepeat();

	//@}
		
private:
	CEvent::Type		m_keyDown;
	CEvent::Type		m_keyUp;
	CEvent::Type		m_keyRepeat;
};

class IPrimaryScreenEvents : public CEventTypes {
public:
	IPrimaryScreenEvents() :
		m_buttonDown(CEvent::kUnknown),
		m_buttonUp(CEvent::kUnknown),
		m_motionOnPrimary(CEvent::kUnknown),
		m_motionOnSecondary(CEvent::kUnknown),
		m_wheel(CEvent::kUnknown),
		m_screensaverActivated(CEvent::kUnknown),
		m_screensaverDeactivated(CEvent::kUnknown),
		m_hotKeyDown(CEvent::kUnknown),
		m_hotKeyUp(CEvent::kUnknown),
		m_fakeInputBegin(CEvent::kUnknown),
		m_fakeInputEnd(CEvent::kUnknown) { }

	//! @name accessors
	//@{
	
	//!  button down event type.  Event data is CButtonInfo*.
	CEvent::Type		buttonDown();

	//!  button up event type.  Event data is CButtonInfo*.
	CEvent::Type		buttonUp();

	//!  mouse motion on the primary screen event type
	/*!
	Event data is CMotionInfo* and the values are an absolute position.
	*/
	CEvent::Type		motionOnPrimary();

	//!  mouse motion on a secondary screen event type
	/*!
	Event data is CMotionInfo* and the values are motion deltas not
	absolute coordinates.
	*/
	CEvent::Type		motionOnSecondary();

	//!  mouse wheel event type.  Event data is CWheelInfo*.
	CEvent::Type		wheel();

	//!  screensaver activated event type
	CEvent::Type		screensaverActivated();

	//!  screensaver deactivated event type
	CEvent::Type		screensaverDeactivated();

	//!  hot key down event type.  Event data is CHotKeyInfo*.
	CEvent::Type		hotKeyDown();

	//!  hot key up event type.  Event data is CHotKeyInfo*.
	CEvent::Type		hotKeyUp();

	//!  start of fake input event type
	CEvent::Type		fakeInputBegin();

	//!  end of fake input event type
	CEvent::Type		fakeInputEnd();

	//@}

private:
	CEvent::Type		m_buttonDown;
	CEvent::Type		m_buttonUp;
	CEvent::Type		m_motionOnPrimary;
	CEvent::Type		m_motionOnSecondary;
	CEvent::Type		m_wheel;
	CEvent::Type		m_screensaverActivated;
	CEvent::Type		m_screensaverDeactivated;
	CEvent::Type		m_hotKeyDown;
	CEvent::Type		m_hotKeyUp;
	CEvent::Type		m_fakeInputBegin;
	CEvent::Type		m_fakeInputEnd;
};

class IScreenEvents : public CEventTypes {
public:
	IScreenEvents() :
		m_error(CEvent::kUnknown),
		m_shapeChanged(CEvent::kUnknown),
		m_clipboardGrabbed(CEvent::kUnknown),
		m_suspend(CEvent::kUnknown),
		m_resume(CEvent::kUnknown),
		m_fileChunkSending(CEvent::kUnknown),
		m_fileRecieveCompleted(CEvent::kUnknown) { }

	//! @name accessors
	//@{

	//! Get error event type
	/*!
	Returns the error event type.  This is sent whenever the screen has
	failed for some reason (e.g. the X Windows server died).
	*/
	CEvent::Type		error();

	//! Get shape changed event type
	/*!
	Returns the shape changed event type.  This is sent whenever the
	screen's shape changes.
	*/
	CEvent::Type		shapeChanged();

	//! Get clipboard grabbed event type
	/*!
	Returns the clipboard grabbed event type.  This is sent whenever the
	clipboard is grabbed by some other application so we don't own it
	anymore.  The data is a pointer to a CClipboardInfo.
	*/
	CEvent::Type		clipboardGrabbed();

	//! Get suspend event type
	/*!
	Returns the suspend event type. This is sent whenever the system goes
	to sleep or a user session is deactivated (fast user switching).
	*/
	CEvent::Type		suspend();
	
	//! Get resume event type
	/*!
	Returns the resume event type. This is sent whenever the system wakes
	up or a user session is activated (fast user switching).
	*/
	CEvent::Type		resume();

	//! Sending a file chunk
	CEvent::Type		fileChunkSending();

	//! Completed receiving a file
	CEvent::Type		fileRecieveCompleted();

	//@}
		
private:
	CEvent::Type		m_error;
	CEvent::Type		m_shapeChanged;
	CEvent::Type		m_clipboardGrabbed;
	CEvent::Type		m_suspend;
	CEvent::Type		m_resume;
	CEvent::Type		m_fileChunkSending;
	CEvent::Type		m_fileRecieveCompleted;
};
