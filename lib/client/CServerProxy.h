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

#ifndef CSERVERPROXY_H
#define CSERVERPROXY_H

#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "CEvent.h"

class CClient;
class CClientInfo;
class CEventQueueTimer;
class IClipboard;
class IStream;

//! Proxy for server
/*!
This class acts a proxy for the server, converting calls into messages
to the server and messages from the server to calls on the client.
*/
class CServerProxy {
public:
	/*!
	Process messages from the server on \p stream and forward to
	\p client.
	*/
	CServerProxy(CClient* client, IStream* stream);
	~CServerProxy();

	//! @name manipulators
	//@{

	virtual void		onInfoChanged();
	virtual bool		onGrabClipboard(ClipboardID);
	virtual void		onClipboardChanged(ClipboardID, const IClipboard*);

	//@}
	//! @name accessors
	//@{

	//! Get handshake complete event type
	/*!
	Returns the handshake complete event type.  This is sent when the
	client has completed the handshake with the server.
	*/
	static CEvent::Type	getHandshakeCompleteEvent();

	//@}

private:
	// if compressing mouse motion then send the last motion now
	void				flushCompressedMouse();

	void				sendInfo(const CClientInfo&);

	void				installHeartBeat(double);

	// modifier key translation
	KeyID				translateKey(KeyID) const;
	KeyModifierMask		translateModifierMask(KeyModifierMask) const;

	// event handlers
	void				handleMessage(const CEvent&, void*);
	void				handleHeartBeat(const CEvent&, void*);

	// message handlers
	void				enter();
	void				leave();
	void				setClipboard();
	void				grabClipboard();
	void				keyDown();
	void				keyRepeat();
	void				keyUp();
	void				mouseDown();
	void				mouseUp();
	void				mouseMove();
	void				mouseWheel();
	void				screensaver();
	void				resetOptions();
	void				setOptions();
	void				queryInfo();
	void				infoAcknowledgment();

private:
	CClient*			m_client;
	IStream*			m_stream;
	CEventQueueTimer*	m_timer;

	UInt32				m_seqNum;

	bool				m_compressMouse;
	SInt32				m_xMouse, m_yMouse;

	bool				m_ignoreMouse;

	KeyModifierID		m_modifierTranslationTable[kKeyModifierIDLast];
	double				m_heartRate;

	static CEvent::Type	s_handshakeCompleteEvent;
};

#endif
