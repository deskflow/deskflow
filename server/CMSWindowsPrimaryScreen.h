#ifndef CMSWINDOWSPRIMARYSCREEN_H
#define CMSWINDOWSPRIMARYSCREEN_H

#include "CMSWindowsScreen.h"
#include "IPrimaryScreen.h"
#include "MouseTypes.h"
#include "CString.h"
#include "CSynergyHook.h"

class CMSWindowsPrimaryScreen : public CMSWindowsScreen, public IPrimaryScreen {
public:
	typedef bool (CMSWindowsPrimaryScreen::*HookMethod)(int, WPARAM, LPARAM);

	CMSWindowsPrimaryScreen();
	virtual ~CMSWindowsPrimaryScreen();

	// IPrimaryScreen overrides
	virtual void		run();
	virtual void		stop();
	virtual void		open(CServer*);
	virtual void		close();
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual bool		leave();
	virtual void		onConfigure();
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		setClipboard(ClipboardID, const IClipboard*);
	virtual void		grabClipboard(ClipboardID);
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getClipboard(ClipboardID, IClipboard*) const;
	virtual KeyModifierMask	getToggleMask() const;
	virtual bool		isLockedToScreen() const;

protected:
	// CMSWindowsScreen overrides
	virtual bool		onPreTranslate(MSG*);
	virtual LRESULT		onEvent(HWND, UINT, WPARAM, LPARAM);
	virtual void		onOpenDisplay();
	virtual void		onCloseDisplay();
	virtual CString		getCurrentDesktopName() const;

private:
	void				enterNoWarp();
	void				onEnter();
	bool				onLeave();

	// discard posted messages
	void				nextMark();

	// open/close desktop (for windows 95/98/me)
	bool				openDesktop();
	void				closeDesktop();

	// make desk the thread desktop (for windows NT/2000/XP)
	bool				switchDesktop(HDESK desk);

	// key and button queries
	KeyID				mapKey(WPARAM keycode, LPARAM info,
								KeyModifierMask* maskOut);
	ButtonID			mapButton(WPARAM button) const;
	void				updateKeys();
	void				updateKey(UINT vkCode, bool press);

private:
	CServer*			m_server;

	// true if windows 95/98/me
	bool				m_is95Family;

	// the main loop's thread id
	DWORD				m_threadID;

	// the current desk and it's name
	HDESK				m_desk;
	CString				m_deskName;

	// our window (for getting clipboard changes)
	HWND				m_window;

	// m_active is true the hooks are relaying events
	bool				m_active;

	// used to discard queued messages that are no longer needed
	UInt32				m_mark;
	UInt32				m_markReceived;

	// clipboard stuff
	HWND				m_nextClipboardWindow;
	HWND				m_clipboardOwner;

	// map of key state
	BYTE				m_keys[256];

	// position of center pixel of screen
	SInt32				m_xCenter, m_yCenter;

	// used to ignore mouse motion
	SInt32				m_mouseMoveIgnore;

	// hook library stuff
	HINSTANCE			m_hookLibrary;
	InstallFunc			m_install;
	UninstallFunc		m_uninstall;
	SetZoneFunc			m_setZone;
	SetRelayFunc		m_setRelay;

	// stuff for restoring active window
	HWND				m_lastForegroundWindow;
	HWND				m_lastActiveWindow;
	DWORD				m_lastActiveThread;
};

#endif
