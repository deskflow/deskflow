#ifndef CPRIMARYCLIENT_H
#define CPRIMARYCLIENT_H

#include "IPrimaryReceiver.h"
#include "IClient.h"

class IClipboard;
class IPrimaryScreen;
class IServer;

class CPrimaryClient : public IPrimaryReceiver, public IClient {
public:
	CPrimaryClient(IServer*, const CString& name);
	~CPrimaryClient();

	// manipulators

	// cause run() to return
	void				stop();

	// called by server when the configuration changes
	void				reconfigure();

	// accessors

	// return the contents of the given clipboard.
	void				getClipboard(ClipboardID, CString&) const;

	// returns true iff the user is locked to the primary screen
	bool				isLockedToScreen() const;

	// returns the state of the toggle keys on the primary screen
	KeyModifierMask		getToggleMask() const;

	// IPrimaryReceiver overrides
	virtual void		onError();
	virtual void		onInfoChanged(SInt32 xScreen, SInt32 yScreen,
							SInt32 wScreen, SInt32 hScreen,
							SInt32 zoneSize,
							SInt32 xMouse, SInt32 yMouse);
	virtual bool		onGrabClipboard(ClipboardID);
	virtual bool		onClipboardChanged(ClipboardID, const CString& data);
	virtual void		onKeyDown(KeyID, KeyModifierMask);
	virtual void		onKeyUp(KeyID, KeyModifierMask);
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		onMouseDown(ButtonID);
	virtual void		onMouseUp(ButtonID);
	virtual bool		onMouseMovePrimary(SInt32 x, SInt32 y);
	virtual void		onMouseMoveSecondary(SInt32 dx, SInt32 dy);
	virtual void		onMouseWheel(SInt32 delta);
	virtual void		onScreenSaver(bool activated);

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
	virtual CString		getName() const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCenter(SInt32& x, SInt32& y) const;
	virtual SInt32		getJumpZoneSize() const;

private:
	IServer*			m_server;
	IPrimaryScreen*		m_screen;
	CString				m_name;
	UInt32				m_seqNum;
	SInt16				m_x, m_y;
	SInt16				m_w, m_h;
	SInt16				m_zoneSize;
	SInt16				m_cx, m_cy;
	bool				m_clipboardOwner[kClipboardEnd];
	bool				m_clipboardDirty[kClipboardEnd];
};

#endif
