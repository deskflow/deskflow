#ifndef CCLIENTPROXY1_0_H
#define CCLIENTPROXY1_0_H

#include "CClientProxy.h"
#include "ProtocolTypes.h"
#include "CMutex.h"

//! Proxy for client implementing protocol version 1.0
class CClientProxy1_0 : public CClientProxy {
public:
	CClientProxy1_0(IServer* server, const CString& name,
							IInputStream* adoptedInput,
							IOutputStream* adoptedOutput);
	~CClientProxy1_0();

	// IClient overrides
	virtual bool		open();
	virtual void		mainLoop();
	virtual void		close();
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool forScreensaver);
	virtual bool		leave();
	virtual void		setClipboard(ClipboardID, const CString&);
	virtual void		grabClipboard(ClipboardID);
	virtual void		setClipboardDirty(ClipboardID, bool);
	virtual void		keyDown(KeyID, KeyModifierMask);
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		keyUp(KeyID, KeyModifierMask);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs);
	virtual void		mouseWheel(SInt32 delta);
	virtual void		screensaver(bool activate);
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const;

private:
	void				recvInfo(bool notify);
	void				recvClipboard();
	void				recvGrabClipboard();

private:
	CMutex				m_mutex;
	CClientInfo			m_info;
	bool				m_clipboardDirty[kClipboardEnd];
};

#endif
