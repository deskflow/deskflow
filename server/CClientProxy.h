#ifndef CCLIENTPROXY_H
#define CCLIENTPROXY_H

#include "IClient.h"
#include "CString.h"

class IInputStream;
class IOutputStream;
class IServer;

class CClientProxy : public IClient {
public:
	CClientProxy(IServer* server, const CString& name,
							IInputStream* adoptedInput,
							IOutputStream* adoptedOutput);
	~CClientProxy();

	// manipulators

	// accessors

	// get the server
	IServer*			getServer() const;

	// get the input and output streams for the client
	IInputStream*		getInputStream() const;
	IOutputStream*		getOutputStream() const;

	// IClient overrides
	virtual void		open() = 0;
	virtual void		run() = 0;
	virtual void		close() = 0;
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool screenSaver) = 0;
	virtual bool		leave() = 0;
	virtual void		setClipboard(ClipboardID, const CString&) = 0;
	virtual void		grabClipboard(ClipboardID) = 0;
	virtual void		setClipboardDirty(ClipboardID, bool) = 0;
	virtual void		keyDown(KeyID, KeyModifierMask) = 0;
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		keyUp(KeyID, KeyModifierMask) = 0;
	virtual void		mouseDown(ButtonID) = 0;
	virtual void		mouseUp(ButtonID) = 0;
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs) = 0;
	virtual void		mouseWheel(SInt32 delta) = 0;
	virtual void		screenSaver(bool activate) = 0;
	virtual CString		getName() const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const = 0;
	virtual void		getCenter(SInt32& x, SInt32& y) const = 0;
	virtual SInt32		getJumpZoneSize() const = 0;

private:
	IServer*			m_server;
	CString				m_name;
	IInputStream*		m_input;
	IOutputStream*		m_output;
};

#endif
