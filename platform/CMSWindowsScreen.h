#ifndef CMSWINDOWSSCREEN_H
#define CMSWINDOWSSCREEN_H

#include "IScreen.h"
#include "CSynergyHook.h"
#include "CMutex.h"
#include "CString.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CMSWindowsScreenSaver;
class CThread;

class CEvent {
public:
	MSG					m_msg;
	LRESULT				m_result;
};

class IScreenReceiver;
class IMSWindowsScreenEventHandler;

class CMSWindowsScreen : public IScreen {
public:
	CMSWindowsScreen(IScreenReceiver*, IMSWindowsScreenEventHandler*);
	virtual ~CMSWindowsScreen();

	// manipulators

	static void			init(HINSTANCE);

	// open the desktop and create and return the window.  returns NULL
	// on failure.
	HWND				openDesktop();

	// close the window and desktop
	void				closeDesktop();

	// accessors

	// returns true iff the system appears to have multiple monitors
	bool				isMultimon() const;

	// get the application instance handle
	static HINSTANCE	getInstance();

	// IScreen overrides
	// note -- this class expects the hook DLL to have been loaded
	// and initialized before open() is called.
	void				open();
	void				mainLoop();
	void				exitMainLoop();
	void				close();
	bool				setClipboard(ClipboardID, const IClipboard*);
	void				checkClipboards();
	void				openScreensaver(bool notify);
	void				closeScreensaver();
	void				screensaver(bool activate);
	void				syncDesktop();
	bool				getClipboard(ClipboardID, IClipboard*) const;
	void				getShape(SInt32&, SInt32&, SInt32&, SInt32&) const;
	void				getCursorPos(SInt32&, SInt32&) const;
	void				getCursorCenter(SInt32&, SInt32&) const;

private:
	// update screen size cache
	void				updateScreenShape();

	// internal pre-dispatch event processing
	bool				onPreDispatch(const CEvent* event);

	// internal (post-dispatch) event processing
	bool				onEvent(CEvent* event);

	// create the transparent cursor
	void				createBlankCursor();

	// switch to the given desktop.  this destroys the window and unhooks
	// all hooks, switches the desktop, then creates the window and rehooks
	// all hooks (because you can't switch the thread's desktop if it has
	// any windows or hooks).
	bool				switchDesktop(HDESK desk);

	// get the input desktop.  caller must CloseDesktop() the result.
	// do not call under windows 95/98/me.
	HDESK				openInputDesktop() const;

	// get the desktop's name.  do not call under windows 95/98/me.
	CString				getDesktopName(HDESK) const;

	// returns true iff desk is the current desk.  do not call under
	// windows 95/98/me.
	bool				isCurrentDesktop(HDESK desk) const;

	// our window proc
	static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

private:
	static HINSTANCE	s_instance;

	IScreenReceiver*	m_receiver;
	IMSWindowsScreenEventHandler*	m_eventHandler;

	ATOM				m_class;
	HICON				m_icon;
	HCURSOR				m_cursor;

	// true if windows 95/98/me
	bool				m_is95Family;

	// our window
	HWND				m_window;

	// screen shape
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;

	// true if system appears to have multiple monitors
	bool				m_multimon;

	// the main loop's thread id
	DWORD				m_threadID;

	// the thread id of the last attached thread
	DWORD				m_lastThreadID;

	// clipboard stuff
	HWND				m_nextClipboardWindow;
	HWND				m_clipboardOwner;

	// the timer used to check for desktop switching
	UINT				m_timer;

	// the current desk and it's name
	HDESK				m_desk;
	CString				m_deskName;

	// screen saver stuff
	HINSTANCE					m_hookLibrary;
	InstallScreenSaverFunc		m_installScreensaver;
	UninstallScreenSaverFunc	m_uninstallScreensaver;
	CMSWindowsScreenSaver*		m_screensaver;
	bool						m_screensaverNotify;

	static CMSWindowsScreen*	s_screen;
};

#endif
