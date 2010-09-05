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

#include "IScreenReceiver.h"
#include "IClient.h"
#include "IClipboard.h"
#include "CNetworkAddress.h"
#include "CMutex.h"
#include "CJobList.h"

class CSecondaryScreen;
class CServerProxy;
class CThread;
class IDataSocket;
class IScreenReceiver;
class ISecondaryScreenFactory;
class ISocketFactory;
class IStreamFilterFactory;

//! Synergy client
/*!
This class implements the top-level client algorithms for synergy.
*/
class CClient : public IScreenReceiver, public IClient {
public:
	enum EStatus {
		kNotRunning,
		kRunning,
		kError,
		kMaxStatus
	};

	/*!
	This client will attempt to connect the server using \c clientName
	as its name.
	*/
	CClient(const CString& clientName);
	~CClient();

	//! @name manipulators
	//@{

	//! Set server address
	/*!
	Sets the server's address that the client should connect to.
	*/
	void				setAddress(const CNetworkAddress& serverAddress);

	//! Set secondary screen factory
	/*!
	Sets the factory for creating secondary screens.  This must be
	set before calling open().  This object takes ownership of the
	factory.
	*/
	void				setScreenFactory(ISecondaryScreenFactory*);

	//! Set socket factory
	/*!
	Sets the factory used to create a socket to connect to the server.
	This must be set before calling mainLoop().  This object takes
	ownership of the factory.
	*/
	void				setSocketFactory(ISocketFactory*);

	//! Set stream filter factory
	/*!
	Sets the factory used to filter the socket streams used to
	communicate with the server.  This object takes ownership
	of the factory.
	*/
	void				setStreamFilterFactory(IStreamFilterFactory*);

	//! Exit event loop
	/*!
	Force mainLoop() to return.  This call can return before
	mainLoop() does (i.e. asynchronously).  This may only be
	called between a successful open() and close().
	*/
	void				exitMainLoop();

	//! Add a job to notify of status changes
	/*!
	The added job is run whenever the server's status changes in
	certain externally visible ways.  The client keeps ownership
	of the job.
	*/
	void				addStatusJob(IJob*);

	//! Remove a job to notify of status changes
	/*!
	Removes a previously added status notification job.  A job can
	remove itself when called but must not remove any other jobs.
	The client keeps ownership of the job.
	*/
	void				removeStatusJob(IJob*);

	//@}
	//! @name accessors
	//@{

	//!
	/*!
	Returns true if the server rejected our connection.
	*/
	bool 				wasRejected() const;

	//! Get the status
	/*!
	Returns the current status and status message.
	*/
	EStatus				getStatus(CString* = NULL) const;

	//@}

	// IScreenReceiver overrides
	virtual void		onError();
	virtual void		onInfoChanged(const CClientInfo&);
	virtual bool		onGrabClipboard(ClipboardID);
	virtual void		onClipboardChanged(ClipboardID, const CString&);

	// IClient overrides
	virtual void		open();
	virtual void		mainLoop();
	virtual void		close();
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool forScreensaver);
	virtual bool		leave();
	virtual void		setClipboard(ClipboardID, const CString&);
	virtual void		grabClipboard(ClipboardID);
	virtual void		setClipboardDirty(ClipboardID, bool dirty);
	virtual void		keyDown(KeyID, KeyModifierMask, KeyButton);
	virtual void		keyRepeat(KeyID, KeyModifierMask,
							SInt32 count, KeyButton);
	virtual void		keyUp(KeyID, KeyModifierMask, KeyButton);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs);
	virtual void		mouseWheel(SInt32 delta);
	virtual void		screensaver(bool activate);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual CString		getName() const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const;

private:
	// notify status jobs of a change
	void				runStatusJobs() const;

	// set new status
	void				setStatus(EStatus, const char* msg = NULL);

	// open/close the secondary screen
	void				openSecondaryScreen();
	void				closeSecondaryScreen();

	// send the clipboard to the server
	void				sendClipboard(ClipboardID);

	// handle server messaging
	void				runSession(void*);
	void				deleteSession(double timeout = -1.0);
	void				runServer();
	CServerProxy*		handshakeServer(IDataSocket*);

private:
	CMutex				m_mutex;
	CString				m_name;
	CSecondaryScreen*	m_screen;
	IScreenReceiver*	m_server;
	CNetworkAddress		m_serverAddress;
	ISecondaryScreenFactory*	m_screenFactory;
	ISocketFactory*				m_socketFactory;
	IStreamFilterFactory*		m_streamFilterFactory;
	CThread*			m_session;
	bool				m_active;
	bool				m_rejected;
	bool				m_ownClipboard[kClipboardEnd];
	IClipboard::Time	m_timeClipboard[kClipboardEnd];
	CString				m_dataClipboard[kClipboardEnd];

	// the status change jobs and status
	CJobList			m_statusJobs;
	EStatus				m_status;
	CString				m_statusMessage;
};

#endif
