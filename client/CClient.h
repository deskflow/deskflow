#ifndef CCLIENT_H
#define CCLIENT_H

#include "IScreenReceiver.h"
#include "IClient.h"
#include "IClipboard.h"
#include "CNetworkAddress.h"
#include "CMutex.h"

class CSecondaryScreen;
class CServerProxy;
class CThread;
class IDataSocket;
class IScreenReceiver;

//! Synergy client
/*!
This class implements the top-level client algorithms for synergy.
*/
class CClient : public IScreenReceiver, public IClient {
public:
	/*!
	This client will attempt to connect the server using \c clientName
	as its name.
	*/
	CClient(const CString& clientName);
	~CClient();

	//! @name manipulators
	//@{

	//! Set camping state
	/*!
	Turns camping on or off.  When camping the client will keep
	trying to connect to the server until it succeeds.  This
	is useful if the client may start before the server.  Do
	not call this while in run().
	*/
	void				camp(bool on);

	//! Set server address
	/*!
	Sets the server's address that the client should connect to.
	*/
	void				setAddress(const CNetworkAddress& serverAddress);

	//! Exit event loop
	/*!
	Force run() to return.  This call can return before
	run() does (i.e. asynchronously).  This may only be
	called between a successful open() and close().
	*/
	void				quit();

	//@}
	//! @name accessors
	//@{

	//!
	/*!
	Returns true if the server rejected our connection.
	*/
	bool 				wasRejected() const;

	//@}

	// IScreenReceiver overrides
	virtual void		onError();
	virtual void		onInfoChanged(const CClientInfo&);
	virtual bool		onGrabClipboard(ClipboardID);
	virtual void		onClipboardChanged(ClipboardID, const CString&);

	// IClient overrides
	virtual bool		open();
	virtual void		run();
	virtual void		close();
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool forScreensaver);
	virtual bool		leave();
	virtual void		setClipboard(ClipboardID, const CString&);
	virtual void		grabClipboard(ClipboardID);
	virtual void		setClipboardDirty(ClipboardID, bool dirty);
	virtual void		keyDown(KeyID, KeyModifierMask);
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		keyUp(KeyID, KeyModifierMask);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs);
	virtual void		mouseWheel(SInt32 delta);
	virtual void		screensaver(bool activate);
	virtual CString		getName() const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const;

private:
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
	bool				m_camp;
	CThread*			m_session;
	bool				m_active;
	bool				m_rejected;
	bool				m_ownClipboard[kClipboardEnd];
	IClipboard::Time	m_timeClipboard[kClipboardEnd];
	CString				m_dataClipboard[kClipboardEnd];
};

#endif
