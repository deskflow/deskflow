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

#include "IScreenReceiver.h"
#include "KeyTypes.h"
#include "CMutex.h"

class IClient;
class IInputStream;
class IOutputStream;

//! Proxy for server
/*!
This class acts a proxy for the server, converting calls into messages
to the server and messages from the server to calls on the client.
*/
class CServerProxy : public IScreenReceiver {
public:
	/*! \c adoptedInput is the stream from the server and
	\c adoptedOutput is the stream to the server.  This object
	takes ownership of both and destroys them in the d'tor.
	Messages from the server are converted to calls on \c client.
	*/
	CServerProxy(IClient* client,
							IInputStream* adoptedInput,
							IOutputStream* adoptedOutput);
	~CServerProxy();

	//! @name manipulators
	//@{

	//! Run event loop
	/*!
	Run the event loop and return when the server disconnects or
	requests the client to disconnect.  Return true iff the server
	didn't reject our connection.

	(cancellation point)
	*/
	bool				mainLoop();

	//@}
	//! @name accessors
	//@{

	//! Get client
	/*!
	Returns the client passed to the c'tor.
	*/
	IClient*			getClient() const;

	//! Get input stream
	/*!
	Return the input stream passed to the c'tor.
	*/
	IInputStream*		getInputStream() const;

	//! Get output stream
	/*!
	Return the output stream passed to the c'tor.
	*/
	IOutputStream*		getOutputStream() const;

	//@}

	// IScreenReceiver overrides
	virtual void		onError();
	virtual void		onInfoChanged(const CClientInfo&);
	virtual bool		onGrabClipboard(ClipboardID);
	virtual void		onClipboardChanged(ClipboardID, const CString& data);

private:

	// get the client name (from the client)
	CString				getName() const;

	// if compressing mouse motion then send the last motion now
	void				flushCompressedMouse();

	void				sendInfo(const CClientInfo&);

	// modifier key translation
	KeyID				translateKey(KeyID) const;
	KeyModifierMask		translateModifierMask(KeyModifierMask) const;

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
	CMutex				m_mutex;

	IClient*			m_client;
	IInputStream*		m_input;
	IOutputStream*		m_output;

	UInt32				m_seqNum;

	bool				m_compressMouse;
	SInt32				m_xMouse, m_yMouse;

	bool				m_ignoreMouse;

	KeyModifierID		m_modifierTranslationTable[kKeyModifierIDLast];
	double				m_heartRate;
};

#endif
