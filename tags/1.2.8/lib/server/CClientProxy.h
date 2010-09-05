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

#ifndef CCLIENTPROXY_H
#define CCLIENTPROXY_H

#include "IClient.h"
#include "CEvent.h"
#include "CString.h"

class IStream;

//! Generic proxy for client
class CClientProxy : public IClient {
public:
	/*!
	\c name is the name of the client.
	*/
	CClientProxy(const CString& name, IStream* adoptedStream);
	~CClientProxy();

	//! @name manipulators
	//@{

	//! Disconnect
	/*!
	Ask the client to disconnect, using \p msg as the reason.
	*/
	void				close(const char* msg);

	//@}
	//! @name accessors
	//@{

	//! Get stream
	/*!
	Returns the stream passed to the c'tor.
	*/
	IStream*			getStream() const;

	//! Get ready event type
	/*!
	Returns the ready event type.  This is sent when the client has
	completed the initial handshake.  Until it is sent, the client is
	not fully connected.
	*/
	static CEvent::Type	getReadyEvent();

	//! Get disconnect event type
	/*!
	Returns the disconnect event type.  This is sent when the client
	disconnects or is disconnected.  The target is getEventTarget().
	*/
	static CEvent::Type	getDisconnectedEvent();

	//! Get clipboard changed event type
	/*!
	Returns the clipboard changed event type.  This is sent whenever the
	contents of the clipboard has changed.  The data is a pointer to a
	IScreen::CClipboardInfo.
	*/
	static CEvent::Type	getClipboardChangedEvent();

	//@}

	// IScreen
	virtual void*		getEventTarget() const;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const = 0;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const = 0;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const = 0;

	// IClient overrides
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool forScreensaver) = 0;
	virtual bool		leave() = 0;
	virtual void		setClipboard(ClipboardID, const IClipboard*) = 0;
	virtual void		grabClipboard(ClipboardID) = 0;
	virtual void		setClipboardDirty(ClipboardID, bool) = 0;
	virtual void		keyDown(KeyID, KeyModifierMask, KeyButton) = 0;
	virtual void		keyRepeat(KeyID, KeyModifierMask,
							SInt32 count, KeyButton) = 0;
	virtual void		keyUp(KeyID, KeyModifierMask, KeyButton) = 0;
	virtual void		mouseDown(ButtonID) = 0;
	virtual void		mouseUp(ButtonID) = 0;
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs) = 0;
	virtual void		mouseRelativeMove(SInt32 xRel, SInt32 yRel) = 0;
	virtual void		mouseWheel(SInt32 xDelta, SInt32 yDelta) = 0;
	virtual void		screensaver(bool activate) = 0;
	virtual void		resetOptions() = 0;
	virtual void		setOptions(const COptionsList& options) = 0;
	virtual CString		getName() const;

private:
	CString				m_name;
	IStream*			m_stream;

	static CEvent::Type	s_readyEvent;
	static CEvent::Type	s_disconnectedEvent;
	static CEvent::Type	s_clipboardChangedEvent;
};

#endif
