#ifndef CCLIENT_H
#define CCLIENT_H

#include "CString.h"
#include "BasicTypes.h"

class CNetworkAddress;
class IInputStream;
class IOutputStream;

class CClient {
  public:
	CClient(const CString& clientName);
	~CClient();

	// manipulators

	void				run(const CNetworkAddress& serverAddress);

	// accessors


  private:
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
	CString				m_name;
	IInputStream*		m_output;
	IOutputStream*		m_output;
};

#endif
