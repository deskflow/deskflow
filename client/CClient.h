#ifndef CCLIENT_H
#define CCLIENT_H

#include "IScreenReceiver.h"
#include "IClient.h"
#include "IClipboard.h"
#include "CNetworkAddress.h"
#include "CMutex.h"

class CServerProxy;
class CThread;
class IDataSocket;
class ISecondaryScreen;
class IScreenReceiver;

class CClient : public IScreenReceiver, public IClient {
public:
	CClient(const CString& clientName);
	~CClient();

	// manipulators

	// turn camping on or off.  when camping the client will keep
	// trying to connect to the server until it succeeds.  this
	// is useful if the client may start before the server.  do
	// not call this while in run().
	void				camp(bool on);

	// set the server's address that the client should connect to
	void				setAddress(const CNetworkAddress& serverAddress);

	// tell client to exit run() gracefully.  this must only be called
	// after a successful open().
	void				quit();

	// accessors

	// returns true if the server rejected us
	bool 				wasRejected() const;

	// IScreenReceiver overrides
	virtual void		onInfoChanged(const CClientInfo&);
	virtual bool		onGrabClipboard(ClipboardID);
	virtual void		onClipboardChanged(ClipboardID, const CString&);

	// IClient overrides
	virtual bool		open();
	virtual void		run();
	virtual void		close();
// FIXME -- can we avoid passing everything here?
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool screenSaver);
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
	virtual void		screenSaver(bool activate);
	virtual CString		getName() const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCenter(SInt32& x, SInt32& y) const;
	virtual void		getMousePos(SInt32& x, SInt32& y) const;
	virtual SInt32		getJumpZoneSize() const;

private:
	// open/close the secondary screen
	void				openSecondaryScreen();
	void				closeSecondaryScreen();

	// send the clipboard to the server
	void				sendClipboard(ClipboardID);

	// handle server messaging
	void				runSession(void*);
	void				deleteSession(CThread*);
	void				runServer();
	CServerProxy*		handshakeServer(IDataSocket*);

private:
	CMutex				m_mutex;
	CString				m_name;
	ISecondaryScreen*	m_screen;
	IScreenReceiver*	m_server;
	CNetworkAddress		m_serverAddress;
	bool				m_camp;
	bool				m_active;
	bool				m_rejected;
	bool				m_ownClipboard[kClipboardEnd];
	IClipboard::Time	m_timeClipboard[kClipboardEnd];
	CString				m_dataClipboard[kClipboardEnd];
};

#endif
