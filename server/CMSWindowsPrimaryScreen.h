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
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		setClipboard(const IClipboard*);
	virtual void		grabClipboard();
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getClipboard(IClipboard*) const;

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
								KeyModifierMask* maskOut) const;
	ButtonID			mapButton(WPARAM button) const;

private:
	CServer*			m_server;
	bool				m_active;
	HWND				m_window;
	HWND				m_nextClipboardWindow;
	HWND				m_clipboardOwner;
	HWND				m_lastActive;
	HINSTANCE			m_hookLibrary;
	UInt32				m_mark;
	UInt32				m_markReceived;
};

#endif
