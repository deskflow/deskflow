#ifndef CMSWINDOWSPRIMARYSCREEN_H
#define CMSWINDOWSPRIMARYSCREEN_H

#include "CPrimaryScreen.h"
#include "IMSWindowsScreenEventHandler.h"
#include "CSynergyHook.h"
#include "MouseTypes.h"
#include "CString.h"

class CMSWindowsScreen;
class IScreenReceiver;
class IPrimaryScreenReceiver;

//! Microsoft windows primary screen implementation
class CMSWindowsPrimaryScreen :
				public CPrimaryScreen, public IMSWindowsScreenEventHandler {
public:
	typedef bool (CMSWindowsPrimaryScreen::*HookMethod)(int, WPARAM, LPARAM);

	CMSWindowsPrimaryScreen(IScreenReceiver*, IPrimaryScreenReceiver*);
	virtual ~CMSWindowsPrimaryScreen();

	// CPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides);
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual KeyModifierMask	getToggleMask() const;
	virtual bool		isLockedToScreen() const;
	virtual IScreen*	getScreen() const;

	// IMSWindowsScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual SInt32		getJumpZoneSize() const;
	virtual void		postCreateWindow(HWND);
	virtual void		preDestroyWindow(HWND);

protected:
	// CPrimaryScreen overrides
	virtual void		onPreMainLoop();
	virtual void		onPreOpen();
	virtual void		onPostOpen();
	virtual void		onPostClose();
	virtual void		onPreEnter();
	virtual void		onPostEnter();
	virtual void		onPreLeave();
	virtual void		onPostLeave(bool);

	virtual void		createWindow();
	virtual void		destroyWindow();
	virtual bool		showWindow();
	virtual void		hideWindow();
	virtual void		warpCursorToCenter();

	virtual void		updateKeys();

private:
	void				enterNoWarp();

	// warp cursor without discarding queued events
	void				warpCursorNoFlush(SInt32 x, SInt32 y);

	// discard posted messages
	void				nextMark();

	// test if event should be ignored
	bool				ignore() const;

	// key and button queries
	KeyID				mapKey(WPARAM keycode, LPARAM info,
							KeyModifierMask* maskOut);
	ButtonID			mapButton(WPARAM button) const;
	void				updateKey(UINT vkCode, bool press);

private:
	IPrimaryScreenReceiver*	m_receiver;
	CMSWindowsScreen*	m_screen;

	// true if windows 95/98/me
	bool				m_is95Family;

	// the main loop's thread id
	DWORD				m_threadID;

	// our window
	HWND				m_window;

	// used to discard queued messages that are no longer needed
	UInt32				m_mark;
	UInt32				m_markReceived;

	// map of key state
	BYTE				m_keys[256];

	// last mouse position
	SInt32				m_x, m_y;

	// position of center pixel of screen
	SInt32				m_xCenter, m_yCenter;

	// hook library stuff
	HINSTANCE			m_hookLibrary;
	InitFunc			m_init;
	CleanupFunc			m_cleanup;
	InstallFunc			m_install;
	UninstallFunc		m_uninstall;
	SetSidesFunc		m_setSides;
	SetZoneFunc			m_setZone;
	SetRelayFunc		m_setRelay;

	// stuff for restoring active window
	HWND				m_lastForegroundWindow;
	HWND				m_lastActiveWindow;
	DWORD				m_lastActiveThread;
};

#endif
