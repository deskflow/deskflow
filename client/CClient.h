#ifndef CCLIENT_H
#define CCLIENT_H

#include "ClipboardTypes.h"
#include "IClipboard.h"
#include "CMutex.h"
#include "CString.h"

class CNetworkAddress;
class IInputStream;
class IOutputStream;
class ISecondaryScreen;

class CClient {
public:
	CClient(const CString& clientName);
	~CClient();

	// manipulators

	// turn camping on or off.  when camping the client will keep
	// trying to connect to the server until it succeeds.  this
	// is useful if the client may start before the server.  do
	// not call this while in run().
	void				camp(bool on);

	// open the client's screen
	bool				open();

	// start the client.  does not return until quit() is called.
	// returns true if the client ever connected to the server
	// successfully.  may also throw exceptions after successfully
	// connecting.  a successful open() must preceed this call.
	bool				run(const CNetworkAddress& serverAddress);

	// tell client to exit run() gracefully.  this must only be called
	// after a successful open().
	void				quit();

	// handle events on client's screen
	void				onClipboardChanged(ClipboardID);
	void				onResolutionChanged();

	// accessors


private:
	void				runSession(void*);

	// open/close the secondary screen
	void				openSecondaryScreen();
	void				closeSecondaryScreen();

	// if compressing mouse motion then send the last motion now
	void				flushCompressedMouse();

	// message handlers
	void				onEnter();
	void				onLeave();
	void				onGrabClipboard();
	void				onScreenSaver();
	void				onQueryInfo();
	void				onQueryInfoNoLock();
	void				onInfoAcknowledgment();
	void				onSetClipboard();
	void				onKeyDown();
	void				onKeyRepeat();
	void				onKeyUp();
	void				onMouseDown();
	void				onMouseUp();
	void				onMouseMove();
	void				onMouseWheel();
	void				onErrorIncompatible();
	void				onErrorBusy();
	void				onErrorUnknown();
	void				onErrorBad();

private:
	CMutex				m_mutex;
	CString				m_name;
	IInputStream*		m_input;
	IOutputStream*		m_output;
	ISecondaryScreen*	m_screen;
	const CNetworkAddress*	m_serverAddress;
	bool				m_camp;
	bool				m_active;
	UInt32				m_seqNum;
	bool				m_ignoreMove;
	bool				m_ownClipboard[kClipboardEnd];
	IClipboard::Time	m_timeClipboard[kClipboardEnd];
	CString				m_dataClipboard[kClipboardEnd];

	bool				m_compressMouse;
	SInt32				m_xMouse, m_yMouse;
};

#endif
