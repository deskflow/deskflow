#ifndef CSERVERPROXY_H
#define CSERVERPROXY_H

#include "IServer.h"
#include "CMutex.h"

class IClient;
class IInputStream;
class IOutputStream;

class CServerProxy : public IServer {
public:
	CServerProxy(IClient* client,
							IInputStream* adoptedInput,
							IOutputStream* adoptedOutput);
	~CServerProxy();

	// manipulators

	// handle messages.  returns true iff server didn't reject our
	// connection.
	bool				run();

	// accessors

	// get the client
	IClient*			getClient() const;

	// get the client name
	CString				getName() const;

	// get the input and output streams for the server
	IInputStream*		getInputStream() const;
	IOutputStream*		getOutputStream() const;

	// IServer overrides
	virtual void		onError();
	virtual void		onInfoChanged(const CString& clientName,
							const CClientInfo&);
	virtual bool		onGrabClipboard(const CString& clientName,
							ClipboardID, UInt32 seqNum);
	virtual void		onClipboardChanged(ClipboardID,
							UInt32 seqNum, const CString& data);
	virtual void		onKeyDown(KeyID, KeyModifierMask);
	virtual void		onKeyUp(KeyID, KeyModifierMask);
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		onMouseDown(ButtonID);
	virtual void		onMouseUp(ButtonID);
	virtual bool		onMouseMovePrimary(SInt32 x, SInt32 y);
	virtual void		onMouseMoveSecondary(SInt32 dx, SInt32 dy);
	virtual void		onMouseWheel(SInt32 delta);
	virtual void		onScreenSaver(bool activated);

private:
	// if compressing mouse motion then send the last motion now
	void				flushCompressedMouse();

	void				sendInfo(const CClientInfo&);

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
	void				screenSaver();
	void				queryInfo();
	void				infoAcknowledgment();

private:
	CMutex				m_mutex;

	IClient*			m_client;
	IInputStream*		m_input;
	IOutputStream*		m_output;

	bool				m_compressMouse;
	SInt32				m_xMouse, m_yMouse;

	bool				m_ignoreMouse;
};

#endif
