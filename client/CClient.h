#ifndef CCLIENT_H
#define CCLIENT_H

#include "CMutex.h"
#include "CString.h"
#include "BasicTypes.h"

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
	void				onClipboardChanged();

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
	void				onQueryClipboard();
	void				onSetClipboard();
	void				onKeyDown();
	void				onKeyRepeat();
	void				onKeyUp();
	void				onMouseDown();
	void				onMouseUp();
	void				onMouseMove();
	void				onMouseWheel();

  private:
	CMutex				m_mutex;
	CString				m_name;
	IInputStream*		m_input;
	IOutputStream*		m_output;
	ISecondaryScreen*	m_screen;
	const CNetworkAddress*	m_serverAddress;
};

#endif
