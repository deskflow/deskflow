#ifndef CPRIMARYCLIENT_H
#define CPRIMARYCLIENT_H

#include "IClient.h"
#include "IScreenReceiver.h"
#include "ProtocolTypes.h"

class IClipboard;
class IPrimaryScreen;
class IServer;

class CPrimaryClient : public IScreenReceiver, public IClient {
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

	// IScreenReceiver overrides
	virtual void		onInfoChanged(const CClientInfo&);
	virtual bool		onGrabClipboard(ClipboardID);
	virtual void		onClipboardChanged(ClipboardID, const CString&);

	// IClient overrides
	virtual bool		open();
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
	virtual void		getMousePos(SInt32& x, SInt32& y) const;
	virtual SInt32		getJumpZoneSize() const;

private:
	IServer*			m_server;
	IPrimaryScreen*		m_screen;
	CString				m_name;
	UInt32				m_seqNum;
	CClientInfo			m_info;
	bool				m_clipboardOwner[kClipboardEnd]; // FIXME -- unneeded?
	bool				m_clipboardDirty[kClipboardEnd];
};

#endif
