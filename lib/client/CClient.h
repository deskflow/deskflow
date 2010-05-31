/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CCLIENT_H
#define CCLIENT_H

#include "IClient.h"
#include "IClipboard.h"
#include "CNetworkAddress.h"
#include "INode.h"

class CEventQueueTimer;
class CScreen;
class CServerProxy;
class IDataSocket;
class ISocketFactory;
class IStream;
class IStreamFilterFactory;

//! Synergy client
/*!
This class implements the top-level client algorithms for synergy.
*/
class CClient : public IClient, public INode {
public:
	class CFailInfo {
	public:
		CFailInfo(const char* what) : m_retry(false), m_what(what) { }
		bool			m_retry;
		CString			m_what;
	};

	/*!
	This client will attempt to connect to the server using \p name
	as its name and \p address as the server's address and \p factory
	to create the socket.  \p screen is	the local screen.
	*/
	CClient(const CString& name, const CNetworkAddress& address,
							ISocketFactory* socketFactory,
							IStreamFilterFactory* streamFilterFactory,
							CScreen* screen);
	~CClient();

	//! @name manipulators
	//@{

	//! Connect to server
	/*!
	Starts an attempt to connect to the server.  This is ignored if
	the client is trying to connect or is already connected.
	*/
	void				connect();

	//! Disconnect
	/*!
	Disconnects from the server with an optional error message.
	*/
	void				disconnect(const char* msg);

	//! Notify of handshake complete
	/*!
	Notifies the client that the connection handshake has completed.
	*/
	void				handshakeComplete();

	//@}
	//! @name accessors
	//@{

	//! Test if connected
	/*!
	Returns true iff the client is successfully connected to the server.
	*/
	bool				isConnected() const;

	//! Test if connecting
	/*!
	Returns true iff the client is currently attempting to connect to
	the server.
	*/
	bool				isConnecting() const;

	//! Get address of server
	/*!
	Returns the address of the server the client is connected (or wants
	to connect) to.
	*/
	CNetworkAddress		getServerAddress() const;

	//! Get connected event type
	/*!
	Returns the connected event type.  This is sent when the client has
	successfully connected to the server.
	*/
	static CEvent::Type	getConnectedEvent();

	//! Get connection failed event type
	/*!
	Returns the connection failed event type.  This is sent when the
	server fails for some reason.  The event data is a CFailInfo*.
	*/
	static CEvent::Type	getConnectionFailedEvent();

	//! Get disconnected event type
	/*!
	Returns the disconnected event type.  This is sent when the client
	has disconnected from the server (and only after having successfully
	connected).
	*/
	static CEvent::Type	getDisconnectedEvent();

	//@}

	// IScreen overrides
	virtual void*		getEventTarget() const;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;

	// IClient overrides
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool forScreensaver);
	virtual bool		leave();
	virtual void		setClipboard(ClipboardID, const IClipboard*);
	virtual void		grabClipboard(ClipboardID);
	virtual void		setClipboardDirty(ClipboardID, bool);
	virtual void		keyDown(KeyID, KeyModifierMask, KeyButton);
	virtual void		keyRepeat(KeyID, KeyModifierMask,
							SInt32 count, KeyButton);
	virtual void		keyUp(KeyID, KeyModifierMask, KeyButton);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs);
	virtual void		mouseRelativeMove(SInt32 xRel, SInt32 yRel);
	virtual void		mouseWheel(SInt32 xDelta, SInt32 yDelta);
	virtual void		screensaver(bool activate);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual CString		getName() const;

private:
	void				sendClipboard(ClipboardID);
	void				sendEvent(CEvent::Type, void*);
	void				sendConnectionFailedEvent(const char* msg);
	void				setupConnecting();
	void				setupConnection();
	void				setupScreen();
	void				setupTimer();
	void				cleanupConnecting();
	void				cleanupConnection();
	void				cleanupScreen();
	void				cleanupTimer();
	void				handleConnected(const CEvent&, void*);
	void				handleConnectionFailed(const CEvent&, void*);
	void				handleConnectTimeout(const CEvent&, void*);
	void				handleOutputError(const CEvent&, void*);
	void				handleDisconnected(const CEvent&, void*);
	void				handleShapeChanged(const CEvent&, void*);
	void				handleClipboardGrabbed(const CEvent&, void*);
	void				handleHello(const CEvent&, void*);
	void				handleSuspend(const CEvent& event, void*);
	void				handleResume(const CEvent& event, void*);
	
private:
	CString					m_name;
	CNetworkAddress			m_serverAddress;
	ISocketFactory*			m_socketFactory;
	IStreamFilterFactory*	m_streamFilterFactory;
	CScreen*				m_screen;
	IStream*				m_stream;
	CEventQueueTimer*		m_timer;
	CServerProxy*			m_server;
	bool					m_ready;
	bool					m_active;
	bool					m_suspended;
	bool					m_connectOnResume;
	bool				m_ownClipboard[kClipboardEnd];
	bool				m_sentClipboard[kClipboardEnd];
	IClipboard::Time	m_timeClipboard[kClipboardEnd];
	CString				m_dataClipboard[kClipboardEnd];

	static CEvent::Type	s_connectedEvent;
	static CEvent::Type	s_connectionFailedEvent;
	static CEvent::Type	s_disconnectedEvent;
};

#endif
