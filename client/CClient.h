#ifndef CCLIENT_H
#define CCLIENT_H

#include "CMutex.h"
#include "CString.h"
#include "BasicTypes.h"
#include "ClipboardTypes.h"
#include "IClipboard.h"

class CNetworkAddress;
class IInputStream;
class IOutputStream;
class ISecondaryScreen;

class CClient {
public:
	CClient(const CString& clientName);
	~CClient();

	// manipulators

	// start the client.  does not return until quit() is called.
	void				run(const CNetworkAddress& serverAddress);

	// tell client to exit gracefully
	void				quit();

	// handle events on client's screen
	void				onClipboardChanged(ClipboardID);
	void				onResolutionChanged();

	// accessors


private:
	void				runSession(void*);

	// open/close the primary screen
	void				openSecondaryScreen();
	void				closeSecondaryScreen();

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
	bool				m_active;
	UInt32				m_seqNum;
	bool				m_ignoreMove;
	bool				m_ownClipboard[kClipboardEnd];
	IClipboard::Time	m_timeClipboard[kClipboardEnd];
	CString				m_dataClipboard[kClipboardEnd];
};

#endif
