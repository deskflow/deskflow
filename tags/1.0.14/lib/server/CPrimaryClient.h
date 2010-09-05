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

#ifndef CPRIMARYCLIENT_H
#define CPRIMARYCLIENT_H

#include "IClient.h"
#include "IScreenReceiver.h"
#include "ProtocolTypes.h"

class IClipboard;
class CPrimaryScreen;
class IPrimaryScreenFactory;
class IPrimaryScreenReceiver;
class IServer;

//! Primary screen as pseudo-client
/*!
The primary screen does not have a client associated with it.  This
class provides a pseudo-client to allow the primary screen to be
treated as if it was on a client.
*/
class CPrimaryClient : public IScreenReceiver, public IClient {
public:
	/*!
	\c name is the name of the server.  The caller retains ownership of
	\c factory.  Throws XScreenOpenFailure or whatever the factory can
	throw if the screen cannot be created.
	*/
	CPrimaryClient(IPrimaryScreenFactory* factory, IServer*,
							IPrimaryScreenReceiver*, const CString& name);
	~CPrimaryClient();

	//! @name manipulators
	//@{

	//! Exit event loop
	/*!
	Force mainLoop() to return.  This call can return before
	mainLoop() does (i.e. asynchronously).  This may only be
	called between a successful open() and close().
	*/
	void				exitMainLoop();

	//! Update configuration
	/*!
	Handles reconfiguration of jump zones.
	*/
	void				reconfigure(UInt32 activeSides);

	//! Install a one-shot timer
	/*!
	Installs a one-shot timer for \c timeout seconds and returns the
	id of the timer (which will be passed to \c onTimerExpired()).
	*/
	UInt32				addOneShotTimer(double timeout);

	//@}
	//! @name accessors
	//@{

	//! Get clipboard
	/*!
	Save the marshalled contents of the clipboard indicated by \c id.
	*/
	void				getClipboard(ClipboardID, CString&) const;

	//! Get toggle key state
	/*!
	Returns the primary screen's current toggle modifier key state.
	*/
	KeyModifierMask		getToggleMask() const;

	//! Get screen lock state
	/*!
	Returns true if the user is locked to the screen.
	*/
	bool				isLockedToScreen() const;

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
	virtual void		setClipboardDirty(ClipboardID, bool);
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
	IServer*			m_server;
	CPrimaryScreen*		m_screen;
	CString				m_name;
	UInt32				m_seqNum;
	CClientInfo			m_info;
	bool				m_clipboardDirty[kClipboardEnd];
};

#endif
