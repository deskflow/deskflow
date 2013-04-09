/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CCLIENTPROXY_H
#define CCLIENTPROXY_H

#include "CBaseClientProxy.h"
#include "CEvent.h"
#include "CString.h"

namespace synergy { class IStream; }

//! Generic proxy for client
class CClientProxy : public CBaseClientProxy {
public:
	/*!
	\c name is the name of the client.
	*/
	CClientProxy(const CString& name, synergy::IStream* adoptedStream);
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
	Returns a crypto stream if the user has this enabled,
	otherwise returns the original stream passed to the c'tor.
	*/
	synergy::IStream*			getStream() const;

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

	//! Get game device timing receive event type
	/*!
	Returns the game device timing receive event type.  This is set 
	whenever the server receives to a timing event response from a client.
	*/
	static CEvent::Type	getGameDeviceTimingRespEvent();

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
	virtual void		gameDeviceButtons(GameDeviceID id, GameDeviceButton buttons) = 0;
	virtual void		gameDeviceSticks(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2) = 0;
	virtual void		gameDeviceTriggers(GameDeviceID id, UInt8 t1, UInt8 t2) = 0;
	virtual void		gameDeviceTimingReq() = 0;
	virtual void		cryptoIv(const UInt8* iv) = 0;

private:
	synergy::IStream*	m_stream;

	static CEvent::Type	s_readyEvent;
	static CEvent::Type	s_disconnectedEvent;
	static CEvent::Type	s_clipboardChangedEvent;
	static CEvent::Type	s_gameDeviceTimingRecvEvent;
};

#endif
