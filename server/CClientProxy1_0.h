#ifndef CCLIENTPROXY1_0_H
#define CCLIENTPROXY1_0_H

#include "CClientProxy.h"
#include "CMutex.h"

class CClientProxy1_0 : public CClientProxy {
public:
	CClientProxy1_0(IServer* server, const CString& name,
							IInputStream* adoptedInput,
							IOutputStream* adoptedOutput);
	~CClientProxy1_0();

	// IClient overrides
	virtual void		open();
	virtual void		run();
	virtual void		close();
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool screenSaver);
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
	virtual void		screenSaver(bool activate);
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCenter(SInt32& x, SInt32& y) const;
	virtual SInt32		getJumpZoneSize() const;

private:
	void				recvInfo(bool notify);
	void				recvClipboard();
	void				recvGrabClipboard();

private:
	CMutex				m_mutex;
	SInt16				m_x, m_y;
	SInt16				m_w, m_h;
	SInt16				m_zoneSize;
	SInt16				m_cx, m_cy;
	bool				m_clipboardDirty[kClipboardEnd];
};

#endif
