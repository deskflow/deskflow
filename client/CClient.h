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

	void				run(const CNetworkAddress& serverAddress);

	// handle events on client's screen
	void				onClipboardChanged(ClipboardID);

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
	bool				m_ownClipboard[kClipboardEnd];
	IClipboard::Time	m_timeClipboard[kClipboardEnd];
};

#endif
