#ifndef CMSWINDOWSPRIMARYSCREEN_H
#define CMSWINDOWSPRIMARYSCREEN_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CMSWindowsScreen.h"
#include "IPrimaryScreen.h"

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
	virtual void		leave();
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

private:
	void				doEnter();

	void				nextMark();

	KeyID				mapKey(WPARAM keycode, LPARAM info,
								KeyModifierMask* maskOut);
	ButtonID			mapButton(WPARAM button) const;
	void				updateKeys();
	void				updateKey(UINT vkCode, bool press);

private:
	CServer*			m_server;
	bool				m_is95Family;
	bool				m_active;
	HWND				m_window;
	HWND				m_nextClipboardWindow;
	HWND				m_clipboardOwner;
	HWND				m_lastForegroundWindow;
	HWND				m_lastActiveWindow;
	DWORD				m_lastActiveThread;
	HINSTANCE			m_hookLibrary;
	UInt32				m_mark;
	UInt32				m_markReceived;
	BYTE				m_keys[256];
	SInt32				m_xCenter, m_yCenter;
};

#endif
