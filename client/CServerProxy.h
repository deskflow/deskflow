#ifndef CSERVERPROXY_H
#define CSERVERPROXY_H

#include "IScreenReceiver.h"
#include "CMutex.h"

class IClient;
class IInputStream;
class IOutputStream;

class CServerProxy : public IScreenReceiver {
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

	// IScreenReceiver overrides
	virtual void		onInfoChanged(const CClientInfo&);
	virtual bool		onGrabClipboard(ClipboardID);
	virtual void		onClipboardChanged(ClipboardID, const CString& data);

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
	void				screensaver();
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
};

#endif
