#include "CMSWindowsPrimaryScreen.h"
#include "CServer.h"
#include "CMSWindowsClipboard.h"
#include "CPlatform.h"
#include "XScreen.h"
#include "XSynergy.h"
#include "CThread.h"
#include "CLog.h"
#include <cstring>

//
// CMSWindowsPrimaryScreen
//

CMSWindowsPrimaryScreen::CMSWindowsPrimaryScreen() :
	m_server(NULL),
	m_threadID(0),
	m_desk(NULL),
	m_deskName(),
	m_window(NULL),
	m_active(false),
	m_mark(0),
	m_markReceived(0),
	m_nextClipboardWindow(NULL),
	m_clipboardOwner(NULL)
{
	// load the hook library
	m_hookLibrary = LoadLibrary("synrgyhk");
	if (m_hookLibrary == NULL) {
		log((CLOG_ERR "failed to load hook library"));
		throw XScreenOpenFailure();
	}
	m_setZone   = (SetZoneFunc)GetProcAddress(m_hookLibrary, "setZone");
	m_setRelay  = (SetRelayFunc)GetProcAddress(m_hookLibrary, "setRelay");
	m_install   = (InstallFunc)GetProcAddress(m_hookLibrary, "install");
	m_uninstall = (UninstallFunc)GetProcAddress(m_hookLibrary, "uninstall");
	if (m_setZone   == NULL ||
		m_setRelay  == NULL ||
		m_install   == NULL ||
		m_uninstall == NULL) {
		log((CLOG_ERR "invalid hook library"));
		FreeLibrary(m_hookLibrary);
		throw XScreenOpenFailure();
	}

	// detect operating system
	m_is95Family = CPlatform::isWindows95Family();

	// make sure this thread has a message queue
	MSG dummy;
	PeekMessage(&dummy, NULL, WM_USER, WM_USER, PM_NOREMOVE);
}

CMSWindowsPrimaryScreen::~CMSWindowsPrimaryScreen()
{
	assert(m_hookLibrary != NULL);
	assert(m_window      == NULL);

	// done with hook library
	FreeLibrary(m_hookLibrary);
}

void
CMSWindowsPrimaryScreen::run()
{
	// must call run() from same thread as open()
	assert(m_threadID == GetCurrentThreadId());

	// change our priority
	CThread::getCurrentThread().setPriority(-3);

	// poll input desktop to see if it changes (preTranslateMessage()
	// handles WM_TIMER)
	UINT timer = 0;
	if (!m_is95Family) {
		SetTimer(NULL, 0, 200, NULL);
	}

	// run event loop
	log((CLOG_INFO "entering event loop"));
	doRun();
	log((CLOG_INFO "exiting event loop"));

	// remove timer
	if (!m_is95Family) {
		KillTimer(NULL, timer);
	}
}

void
CMSWindowsPrimaryScreen::stop()
{
	doStop();
}

void
CMSWindowsPrimaryScreen::open(CServer* server)
{
	assert(m_server == NULL);
	assert(server   != NULL);

	// set the server
	m_server = server;

	// open the display
	openDisplay();

	// initialize marks
	m_mark         = 0;
	m_markReceived = 0;
	nextMark();

	// send screen info
	SInt32 x, y, w, h;
	getScreenShape(x, y, w, h);
	POINT pos;
	GetCursorPos(&pos);
	m_server->setInfo(x, y, w, h, getJumpZoneSize(), pos.x, pos.y);

	// compute center pixel of primary screen
	m_xCenter = GetSystemMetrics(SM_CXSCREEN) >> 1;
	m_yCenter = GetSystemMetrics(SM_CYSCREEN) >> 1;

	// get keyboard state
	updateKeys();

	// enter the screen
	enterNoWarp();
}

void
CMSWindowsPrimaryScreen::close()
{
	assert(m_server != NULL);

	// close the display
	closeDisplay();

	// done with server
	m_server = NULL;
}

void
CMSWindowsPrimaryScreen::enter(SInt32 x, SInt32 y)
{
	log((CLOG_INFO "entering primary at %d,%d", x, y));
	assert(m_active == true);

	// enter the screen
	enterNoWarp();

	// warp to requested location
	warpCursor(x, y);
}

bool
CMSWindowsPrimaryScreen::leave()
{
	log((CLOG_INFO "leaving primary"));
	assert(m_active == false);

	// all messages prior to now are invalid
	nextMark();

	// save active window, show ours, and grab mouse/keyboard
	if (!onLeave()) {
		return false;
	}

	// relay all mouse and keyboard events
	m_setRelay();

	// get state of keys as we leave
	updateKeys();

	// warp mouse to center of screen
	warpCursor(m_xCenter, m_yCenter);

	// ignore this many mouse motion events (not including the already
	// queued events).  on (at least) the win2k login desktop, one
	// motion event is reported using a position from before the above
	// warpCursor().  i don't know why it does that and other desktops
	// don't have the same problem.  anyway, simply ignoring that event
	// works around it.
	m_mouseMoveIgnore = 1;

	// disable ctrl+alt+del, alt+tab, etc
	if (m_is95Family) {
		DWORD dummy = 0;
		SystemParametersInfo(SPI_SETSCREENSAVERRUNNING, TRUE, &dummy, 0);
	}

	// discard messages until after the warp
	nextMark();

	// local client now active
	m_active = true;

	// if we think we own the clipboard but we don't then somebody
	// grabbed the clipboard on this screen without us knowing.
	// tell the server that this screen grabbed the clipboard.
	//
	// this works around bugs in the clipboard viewer chain.
	// sometimes NT will simply never send WM_DRAWCLIPBOARD
	// messages for no apparent reason and rebooting fixes the
	// problem.  since we don't want a broken clipboard until the
	// next reboot we do this double check.  clipboard ownership
	// won't be reflected on other screens until we leave but at
	// least the clipboard itself will work.
	HWND clipboardOwner = GetClipboardOwner();
	if (m_clipboardOwner != clipboardOwner) {
		try {
			m_clipboardOwner = clipboardOwner;
			if (m_clipboardOwner != m_window) {
				m_server->grabClipboard(kClipboardClipboard);
				m_server->grabClipboard(kClipboardSelection);
			}
		}
		catch (XBadClient&) {
			// ignore
		}
	}

	return true;
}

void
CMSWindowsPrimaryScreen::onConfigure()
{
	if ((m_is95Family || m_desk != NULL) && !m_active) {
		SInt32 x, y, w, h;
		getScreenShape(x, y, w, h);
		m_setZone(m_server->getActivePrimarySides(),
								x, y, w, h, getJumpZoneSize());
	}
}

void
CMSWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	// set the cursor position without generating an event
	SetCursorPos(x, y);
}

void
CMSWindowsPrimaryScreen::setClipboard(ClipboardID /*id*/,
				const IClipboard* src)
{
	assert(m_window != NULL);

	CMSWindowsClipboard dst(m_window);
	CClipboard::copy(&dst, src);
}

void
CMSWindowsPrimaryScreen::grabClipboard(ClipboardID /*id*/)
{
	assert(m_window != NULL);

	CMSWindowsClipboard clipboard(m_window);
	if (clipboard.open(0)) {
		clipboard.close();
	}
}

void
CMSWindowsPrimaryScreen::getShape(
				SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	getScreenShape(x, y, w, h);
}

SInt32
CMSWindowsPrimaryScreen::getJumpZoneSize() const
{
	return 1;
}

void
CMSWindowsPrimaryScreen::getClipboard(ClipboardID /*id*/,
				IClipboard* dst) const
{
	assert(m_window != NULL);

	CMSWindowsClipboard src(m_window);
	CClipboard::copy(dst, &src);
}

KeyModifierMask
CMSWindowsPrimaryScreen::getToggleMask() const
{
	KeyModifierMask mask = 0;
	if ((GetKeyState(VK_CAPITAL) & 0x01) != 0)
		mask |= KeyModifierCapsLock;
	if ((GetKeyState(VK_NUMLOCK) & 0x01) != 0)
		mask |= KeyModifierNumLock;
	if ((GetKeyState(VK_SCROLL) & 0x01) != 0)
		mask |= KeyModifierScrollLock;
	return mask;
}

bool
CMSWindowsPrimaryScreen::isLockedToScreen() const
{
	// check buttons
	if (GetAsyncKeyState(VK_LBUTTON) < 0 ||
		GetAsyncKeyState(VK_MBUTTON) < 0 ||
		GetAsyncKeyState(VK_RBUTTON) < 0) {
		return true;
	}

	// check keys
	BYTE keys[256];
	if (GetKeyboardState(keys)) {
		for (unsigned int i = 0; i < sizeof(keys); ++i) {
			if ((keys[i] & 0x80) != 0) {
				return true;
			}
		}
	}

	// not locked
	return false;
}

void
CMSWindowsPrimaryScreen::onOpenDisplay()
{
	assert(m_window == NULL);
	assert(m_server != NULL);

	// save thread id.  we'll need to pass this to the hook library.
	m_threadID = GetCurrentThreadId();

	// get the input desktop and switch to it
	if (m_is95Family) {
		if (!openDesktop()) {
			throw XScreenOpenFailure();
		}
	}
	else {
		if (!switchDesktop(openInputDesktop())) {
			throw XScreenOpenFailure();
		}
	}
}

void
CMSWindowsPrimaryScreen::onCloseDisplay()
{
	// disconnect from desktop
	if (m_is95Family) {
		closeDesktop();
	}
	else {
		switchDesktop(NULL);
	}

	// clear thread id
	m_threadID = 0;

	assert(m_window == NULL);
	assert(m_desk   == NULL);
}

bool
CMSWindowsPrimaryScreen::onPreTranslate(MSG* msg)
{
	// handle event
	switch (msg->message) {
	case SYNERGY_MSG_MARK:
		m_markReceived = msg->wParam;
		return true;

	case SYNERGY_MSG_KEY:
		// ignore message if posted prior to last mark change
		if (m_markReceived == m_mark) {
			KeyModifierMask mask;
			const KeyID key = mapKey(msg->wParam, msg->lParam, &mask);
			if (key != kKeyNone) {
				if ((msg->lParam & 0x80000000) == 0) {
					// key press
					const SInt32 repeat = (SInt32)(msg->lParam & 0xffff);
					if (repeat >= 2) {
						log((CLOG_DEBUG1 "event: key repeat key=%d mask=0x%04x count=%d", key, mask, repeat));
						m_server->onKeyRepeat(key, mask, repeat);
					}
					else {
						log((CLOG_DEBUG1 "event: key press key=%d mask=0x%04x", key, mask));
						m_server->onKeyDown(key, mask);
					}

					// update key state
					updateKey(msg->wParam, true);
				}
				else {
					// key release
					log((CLOG_DEBUG1 "event: key release key=%d mask=0x%04x", key, mask));
					m_server->onKeyUp(key, mask);

					// update key state
					updateKey(msg->wParam, false);
				}
			}
			else {
				log((CLOG_DEBUG2 "event: cannot map key wParam=%d lParam=0x%08x", msg->wParam, msg->lParam));
			}
		}
		return true;

	case SYNERGY_MSG_MOUSE_BUTTON:
		// ignore message if posted prior to last mark change
		if (m_markReceived == m_mark) {
			const ButtonID button = mapButton(msg->wParam);
			switch (msg->wParam) {
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
				log((CLOG_DEBUG1 "event: button press button=%d", button));
				if (button != kButtonNone) {
					m_server->onMouseDown(button);
				}
				break;

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
				log((CLOG_DEBUG1 "event: button release button=%d", button));
				if (button != kButtonNone) {
					m_server->onMouseUp(button);
				}
				break;
			}
		}
		return true;

	case SYNERGY_MSG_MOUSE_WHEEL:
		// ignore message if posted prior to last mark change
		if (m_markReceived == m_mark) {
			log((CLOG_ERR "event: button wheel delta=%d %d", msg->wParam, msg->lParam));
			m_server->onMouseWheel(msg->wParam);
		}
		return true;

	case SYNERGY_MSG_MOUSE_MOVE:
		// ignore message if posted prior to last mark change
		if (m_markReceived == m_mark) {
			SInt32 x = static_cast<SInt32>(msg->wParam);
			SInt32 y = static_cast<SInt32>(msg->lParam);
			if (!m_active) {
				m_server->onMouseMovePrimary(x, y);
			}
			else {
				// get mouse deltas
				x -= m_xCenter;
				y -= m_yCenter;

				// ignore if the mouse didn't move or we're ignoring
				// motion.
				if (m_mouseMoveIgnore == 0) {
					if (x != 0 && y != 0) {
						// warp mouse back to center
						warpCursor(m_xCenter, m_yCenter);

						// send motion
						m_server->onMouseMoveSecondary(x, y);
					}
				}
				else {
					// ignored one more motion event
					--m_mouseMoveIgnore;
				}
			}
		}
		return true;

	case WM_TIMER:
		// if current desktop is not the input desktop then switch to it
		if (!m_is95Family) {
			HDESK desk = openInputDesktop();
			if (desk != NULL) {
				if (isCurrentDesktop(desk)) {
					CloseDesktop(desk);
				}
				else {
					switchDesktop(desk);
				}
			}
		}
		return true;
	}

	return false;
}

LRESULT
CMSWindowsPrimaryScreen::onEvent(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_QUERYENDSESSION:
		if (m_is95Family) {
			return TRUE;
		}
		break;

	case WM_ENDSESSION:
		if (m_is95Family) {
			if (wParam == TRUE && lParam == 0) {
				stop();
			}
			return 0;
		}
		break;

	case WM_PAINT:
		ValidateRect(hwnd, NULL);
		return 0;

	case WM_DRAWCLIPBOARD:
		log((CLOG_DEBUG "clipboard was taken"));

		// first pass it on
		if (m_nextClipboardWindow != NULL) {
			SendMessage(m_nextClipboardWindow, msg, wParam, lParam);
		}

		// now notify server that somebody changed the clipboard.
		// skip that if we're the new owner.
		try {
			m_clipboardOwner = GetClipboardOwner();
			if (m_clipboardOwner != m_window) {
				m_server->grabClipboard(kClipboardClipboard);
				m_server->grabClipboard(kClipboardSelection);
			}
		}
		catch (XBadClient&) {
			// ignore.  this can happen if we receive this event
			// before we've fully started up.
		}
		return 0;

	case WM_CHANGECBCHAIN:
		if (m_nextClipboardWindow == (HWND)wParam) {
			m_nextClipboardWindow = (HWND)lParam;
		}
		else if (m_nextClipboardWindow != NULL) {
			SendMessage(m_nextClipboardWindow, msg, wParam, lParam);
		}
		return 0;

	case WM_DISPLAYCHANGE:
		{
			// screen resolution may have changed
			SInt32 xOld, yOld, wOld, hOld;
			getScreenShape(xOld, yOld, wOld, hOld);
			updateScreenShape();
			SInt32 x, y, w, h;
			getScreenShape(x, y, w, h);

			// do nothing if resolution hasn't changed
			if (x != xOld || y != yOld || w != wOld || h != hOld) {
				// recompute center pixel of primary screen
				m_xCenter = GetSystemMetrics(SM_CXSCREEN) >> 1;
				m_yCenter = GetSystemMetrics(SM_CYSCREEN) >> 1;

				// warp mouse to center if active
				if (m_active) {
					warpCursor(m_xCenter, m_yCenter);
				}

				// tell hook about resize if not active
				else {
					onConfigure();
				}

				// send new screen info
				POINT pos;
				GetCursorPos(&pos);
				m_server->setInfo(x, y, w, h, getJumpZoneSize(), pos.x, pos.y);
			}

			return 0;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void
CMSWindowsPrimaryScreen::enterNoWarp()
{
	// not active anymore
	m_active = false;

	// reset motion ignore count
	m_mouseMoveIgnore = 0;

	// enable ctrl+alt+del, alt+tab, etc
	if (m_is95Family) {
		DWORD dummy = 0;
		SystemParametersInfo(SPI_SETSCREENSAVERRUNNING, FALSE, &dummy, 0);
	}

	// install jump zones
	onConfigure();

	// restore active window and hide our window
	onEnter();

	// all messages prior to now are invalid
	nextMark();
}

void
CMSWindowsPrimaryScreen::onEnter()
{
	// restore the active window and hide our window.  we can only set
	// the active window for another thread if we first attach our input
	// to that thread.
	ReleaseCapture();
	if (m_lastActiveWindow != NULL) {
		DWORD myThread = GetCurrentThreadId();
		if (AttachThreadInput(myThread, m_lastActiveThread, TRUE)) {
			// FIXME -- shouldn't raise window if X-Mouse is enabled
			// but i have no idea how to do that or check if enabled.
			SetActiveWindow(m_lastActiveWindow);
			AttachThreadInput(myThread, m_lastActiveThread, FALSE);
		}
	}
	ShowWindow(m_window, SW_HIDE);
}

bool
CMSWindowsPrimaryScreen::onLeave()
{
	// remember the active window before we leave.  GetActiveWindow()
	// will only return the active window for the thread's queue (i.e.
	// our app) but we need the globally active window.  get that by
	// attaching input to the foreground window's thread then calling
	// GetActiveWindow() and then detaching our input.
	m_lastActiveWindow     = NULL;
	m_lastForegroundWindow = GetForegroundWindow();
	m_lastActiveThread     = GetWindowThreadProcessId(
								m_lastForegroundWindow, NULL);
	if (m_lastActiveThread != 0) {
		DWORD myThread = GetCurrentThreadId();
		if (AttachThreadInput(myThread, m_lastActiveThread, TRUE)) {
			m_lastActiveWindow = GetActiveWindow();
			AttachThreadInput(myThread, m_lastActiveThread, FALSE);
		}
	}

	// show our window
	ShowWindow(m_window, SW_SHOW);

	// get keyboard input and capture mouse
	SetActiveWindow(m_window);
	SetFocus(m_window);
	SetCapture(m_window);

	return true;
}

void
CMSWindowsPrimaryScreen::nextMark()
{
	// next mark
	++m_mark;

	// mark point in message queue where the mark was changed
	PostThreadMessage(m_threadID, SYNERGY_MSG_MARK, m_mark, 0);
}

bool
CMSWindowsPrimaryScreen::openDesktop()
{
	// install hooks
	m_install(m_threadID);

	// note -- we use a fullscreen window to grab input.  it should
	// be possible to use a 1x1 window but i've run into problems
	// with losing keyboard input (focus?) in that case.
	// unfortunately, hiding the full screen window (when entering
	// the scren causes all other windows to redraw).
	SInt32 x, y, w, h;
	getScreenShape(x, y, w, h);

	// create the window
	m_window = CreateWindowEx(WS_EX_TOPMOST |
									WS_EX_TRANSPARENT |
									WS_EX_TOOLWINDOW,
								(LPCTSTR)getClass(),
								"Synergy",
								WS_POPUP,
								x, y, w, h, NULL, NULL,
								getInstance(),
								NULL);
	if (m_window == NULL) {
		log((CLOG_ERR "failed to create window: %d", GetLastError()));
		m_uninstall();
		return false;
	}

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	return true;
}

void
CMSWindowsPrimaryScreen::closeDesktop()
{
	// destroy old window
	if (m_window != NULL) {
		// restore active window and hide ours
		if (m_active) {
			onEnter();
		}

		// first remove clipboard snooper
		ChangeClipboardChain(m_window, m_nextClipboardWindow);
		m_nextClipboardWindow = NULL;

		// we no longer own the clipboard
		if (m_clipboardOwner == m_window) {
			m_clipboardOwner = NULL;
		}

		// now destroy window
		DestroyWindow(m_window);
		m_window = NULL;
	}

	// unhook
	m_uninstall();
}

bool
CMSWindowsPrimaryScreen::switchDesktop(HDESK desk)
{
	// did we own the clipboard?
	bool ownClipboard = (m_clipboardOwner == m_window);

	// destroy old window
	if (m_window != NULL) {
		// restore active window and hide ours
		if (m_active) {
			onEnter();
		}

		// first remove clipboard snooper
		ChangeClipboardChain(m_window, m_nextClipboardWindow);
		m_nextClipboardWindow = NULL;

		// we no longer own the clipboard
		if (ownClipboard) {
			m_clipboardOwner = NULL;
		}

		// now destroy window
		DestroyWindow(m_window);
		m_window = NULL;
	}

	// unhook
	if (m_desk != NULL) {
		m_uninstall();
		CloseDesktop(m_desk);
		m_desk     = NULL;
		m_deskName = "";
	}

	// if no new desktop then we're done
	if (desk == NULL) {
		log((CLOG_INFO "disconnecting desktop"));
		return true;
	}

	// set the desktop.  can only do this when there are no windows
	// and hooks on the current desktop owned by this thread.
	if (SetThreadDesktop(desk) == 0) {
		log((CLOG_ERR "failed to set desktop: %d", GetLastError()));
		CloseDesktop(desk);
		return false;
	}

	// install hooks
	m_install(m_threadID);

	// note -- we use a fullscreen window to grab input.  it should
	// be possible to use a 1x1 window but i've run into problems
	// with losing keyboard input (focus?) in that case.
	// unfortunately, hiding the full screen window (when entering
	// the scren causes all other windows to redraw).
	SInt32 x, y, w, h;
	getScreenShape(x, y, w, h);

	// create the window
	m_window = CreateWindowEx(WS_EX_TOPMOST |
									WS_EX_TRANSPARENT |
									WS_EX_TOOLWINDOW,
								(LPCTSTR)getClass(),
								"Synergy",
								WS_POPUP,
								x, y, w, h, NULL, NULL,
								getInstance(),
								NULL);
	if (m_window == NULL) {
		log((CLOG_ERR "failed to create window: %d", GetLastError()));
		m_uninstall();
		CloseDesktop(desk);
		return false;
	}

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	// reassert clipboard ownership
	if (ownClipboard) {
		// FIXME
	}

	// save new desktop
	m_desk     = desk;
	m_deskName = getDesktopName(m_desk);
	log((CLOG_INFO "switched to desktop %s", m_deskName.c_str()));

	// get active window and show ours
	if (m_active) {
		onLeave();
	}
	else {
		// set jump zones
		onConfigure();

		// all messages prior to now are invalid
		nextMark();
	}

	return true;
}

CString
CMSWindowsPrimaryScreen::getCurrentDesktopName() const
{
	return m_deskName;
}

static const KeyID		g_virtualKey[] =
{
	/* 0x00 */ kKeyNone,	// reserved
	/* 0x01 */ kKeyNone,	// VK_LBUTTON
	/* 0x02 */ kKeyNone,	// VK_RBUTTON
	/* 0x03 */ 0xff6b,		// VK_CANCEL		XK_Break
	/* 0x04 */ kKeyNone,	// VK_MBUTTON
	/* 0x05 */ kKeyNone,	// undefined
	/* 0x06 */ kKeyNone,	// undefined
	/* 0x07 */ kKeyNone,	// undefined
	/* 0x08 */ 0xff08,		// VK_BACK			XK_Backspace
	/* 0x09 */ 0xff09,		// VK_TAB			VK_Tab
	/* 0x0a */ kKeyNone,	// undefined
	/* 0x0b */ kKeyNone,	// undefined
	/* 0x0c */ 0xff0b,		// VK_CLEAR			XK_Clear
	/* 0x0d */ 0xff0d,		// VK_RETURN		XK_Return
	/* 0x0e */ kKeyNone,	// undefined
	/* 0x0f */ kKeyNone,	// undefined
	/* 0x10 */ 0xffe1,		// VK_SHIFT			XK_Shift_L
	/* 0x11 */ 0xffe3,		// VK_CONTROL		XK_Control_L
	/* 0x12 */ 0xffe9,		// VK_MENU			XK_Alt_L
	/* 0x13 */ 0xff13,		// VK_PAUSE			XK_Pause
	/* 0x14 */ 0xffe5,		// VK_CAPITAL		XK_Caps_Lock
	/* 0x15 */ kKeyNone,	// VK_KANA			
	/* 0x16 */ kKeyNone,	// VK_HANGUL		
	/* 0x17 */ kKeyNone,	// VK_JUNJA			
	/* 0x18 */ kKeyNone,	// VK_FINAL			
	/* 0x19 */ kKeyNone,	// VK_KANJI			
	/* 0x1a */ kKeyNone,	// undefined
	/* 0x1b */ 0xff1b,		// VK_ESCAPE		XK_Escape
	/* 0x1c */ kKeyNone,	// VK_CONVERT		
	/* 0x1d */ kKeyNone,	// VK_NONCONVERT	
	/* 0x1e */ kKeyNone,	// VK_ACCEPT		
	/* 0x1f */ kKeyNone,	// VK_MODECHANGE	
	/* 0x20 */ 0x0020,		// VK_SPACE			XK_space
	/* 0x21 */ 0xff55,		// VK_PRIOR			XK_Prior
	/* 0x22 */ 0xff56,		// VK_NEXT			XK_Next
	/* 0x23 */ 0xff57,		// VK_END			XK_End
	/* 0x24 */ 0xff50,		// VK_HOME			XK_Home
	/* 0x25 */ 0xff51,		// VK_LEFT			XK_Left
	/* 0x26 */ 0xff52,		// VK_UP			XK_Up
	/* 0x27 */ 0xff53,		// VK_RIGHT			XK_Right
	/* 0x28 */ 0xff54,		// VK_DOWN			XK_Down
	/* 0x29 */ 0xff60,		// VK_SELECT		XK_Select
	/* 0x2a */ kKeyNone,	// VK_PRINT			
	/* 0x2b */ 0xff62,		// VK_EXECUTE		XK_Execute
	/* 0x2c */ 0xff61,		// VK_SNAPSHOT		XK_Print
	/* 0x2d */ 0xff63,		// VK_INSERT		XK_Insert
	/* 0x2e */ 0xffff,		// VK_DELETE		XK_Delete
	/* 0x2f */ 0xff6a,		// VK_HELP			XK_Help
	/* 0x30 */ kKeyNone,	// VK_0				XK_0
	/* 0x31 */ kKeyNone,	// VK_1				XK_1
	/* 0x32 */ kKeyNone,	// VK_2				XK_2
	/* 0x33 */ kKeyNone,	// VK_3				XK_3
	/* 0x34 */ kKeyNone,	// VK_4				XK_4
	/* 0x35 */ kKeyNone,	// VK_5				XK_5
	/* 0x36 */ kKeyNone,	// VK_6				XK_6
	/* 0x37 */ kKeyNone,	// VK_7				XK_7
	/* 0x38 */ kKeyNone,	// VK_8				XK_8
	/* 0x39 */ kKeyNone,	// VK_9				XK_9
	/* 0x3a */ kKeyNone,	// undefined
	/* 0x3b */ kKeyNone,	// undefined
	/* 0x3c */ kKeyNone,	// undefined
	/* 0x3d */ kKeyNone,	// undefined
	/* 0x3e */ kKeyNone,	// undefined
	/* 0x3f */ kKeyNone,	// undefined
	/* 0x40 */ kKeyNone,	// undefined
	/* 0x41 */ kKeyNone,	// VK_A				XK_A
	/* 0x42 */ kKeyNone,	// VK_B				XK_B
	/* 0x43 */ kKeyNone,	// VK_C				XK_C
	/* 0x44 */ kKeyNone,	// VK_D				XK_D
	/* 0x45 */ kKeyNone,	// VK_E				XK_E
	/* 0x46 */ kKeyNone,	// VK_F				XK_F
	/* 0x47 */ kKeyNone,	// VK_G				XK_G
	/* 0x48 */ kKeyNone,	// VK_H				XK_H
	/* 0x49 */ kKeyNone,	// VK_I				XK_I
	/* 0x4a */ kKeyNone,	// VK_J				XK_J
	/* 0x4b */ kKeyNone,	// VK_K				XK_K
	/* 0x4c */ kKeyNone,	// VK_L				XK_L
	/* 0x4d */ kKeyNone,	// VK_M				XK_M
	/* 0x4e */ kKeyNone,	// VK_N				XK_N
	/* 0x4f */ kKeyNone,	// VK_O				XK_O
	/* 0x50 */ kKeyNone,	// VK_P				XK_P
	/* 0x51 */ kKeyNone,	// VK_Q				XK_Q
	/* 0x52 */ kKeyNone,	// VK_R				XK_R
	/* 0x53 */ kKeyNone,	// VK_S				XK_S
	/* 0x54 */ kKeyNone,	// VK_T				XK_T
	/* 0x55 */ kKeyNone,	// VK_U				XK_U
	/* 0x56 */ kKeyNone,	// VK_V				XK_V
	/* 0x57 */ kKeyNone,	// VK_W				XK_W
	/* 0x58 */ kKeyNone,	// VK_X				XK_X
	/* 0x59 */ kKeyNone,	// VK_Y				XK_Y
	/* 0x5a */ kKeyNone,	// VK_Z				XK_Z
	/* 0x5b */ 0xffe7,		// VK_LWIN			XK_Meta_L
	/* 0x5c */ 0xffe8,		// VK_RWIN			XK_Meta_R
	/* 0x5d */ 0xff67,		// VK_APPS			XK_Menu
	/* 0x5e */ kKeyNone,	// undefined
	/* 0x5f */ kKeyNone,	// undefined
	/* 0x60 */ 0xffb0,		// VK_NUMPAD0		XK_KP_0
	/* 0x61 */ 0xffb1,		// VK_NUMPAD1		XK_KP_1
	/* 0x62 */ 0xffb2,		// VK_NUMPAD2		XK_KP_2
	/* 0x63 */ 0xffb3,		// VK_NUMPAD3		XK_KP_3
	/* 0x64 */ 0xffb4,		// VK_NUMPAD4		XK_KP_4
	/* 0x65 */ 0xffb5,		// VK_NUMPAD5		XK_KP_5
	/* 0x66 */ 0xffb6,		// VK_NUMPAD6		XK_KP_6
	/* 0x67 */ 0xffb7,		// VK_NUMPAD7		XK_KP_7
	/* 0x68 */ 0xffb8,		// VK_NUMPAD8		XK_KP_8
	/* 0x69 */ 0xffb9,		// VK_NUMPAD9		XK_KP_9
	/* 0x6a */ 0xffaa,		// VK_MULTIPLY		XK_KP_Multiply
	/* 0x6b */ 0xffab,		// VK_ADD			XK_KP_Add
	/* 0x6c */ 0xffac,		// VK_SEPARATOR		XK_KP_Separator
	/* 0x6d */ 0xffad,		// VK_SUBTRACT		XK_KP_Subtract
	/* 0x6e */ 0xffae,		// VK_DECIMAL		XK_KP_Decimal
	/* 0x6f */ 0xffaf,		// VK_DIVIDE		XK_KP_Divide
	/* 0x70 */ 0xffbe,		// VK_F1			XK_F1
	/* 0x71 */ 0xffbf,		// VK_F2			XK_F2
	/* 0x72 */ 0xffc0,		// VK_F3			XK_F3
	/* 0x73 */ 0xffc1,		// VK_F4			XK_F4
	/* 0x74 */ 0xffc2,		// VK_F5			XK_F5
	/* 0x75 */ 0xffc3,		// VK_F6			XK_F6
	/* 0x76 */ 0xffc4,		// VK_F7			XK_F7
	/* 0x77 */ 0xffc5,		// VK_F8			XK_F8
	/* 0x78 */ 0xffc6,		// VK_F9			XK_F9
	/* 0x79 */ 0xffc7,		// VK_F10			XK_F10
	/* 0x7a */ 0xffc8,		// VK_F11			XK_F11
	/* 0x7b */ 0xffc9,		// VK_F12			XK_F12
	/* 0x7c */ 0xffca,		// VK_F13			XK_F13
	/* 0x7d */ 0xffcb,		// VK_F14			XK_F14
	/* 0x7e */ 0xffcc,		// VK_F15			XK_F15
	/* 0x7f */ 0xffcd,		// VK_F16			XK_F16
	/* 0x80 */ 0xffce,		// VK_F17			XK_F17
	/* 0x81 */ 0xffcf,		// VK_F18			XK_F18
	/* 0x82 */ 0xffd0,		// VK_F19			XK_F19
	/* 0x83 */ 0xffd1,		// VK_F20			XK_F20
	/* 0x84 */ 0xffd2,		// VK_F21			XK_F21
	/* 0x85 */ 0xffd3,		// VK_F22			XK_F22
	/* 0x86 */ 0xffd4,		// VK_F23			XK_F23
	/* 0x87 */ 0xffd5,		// VK_F24			XK_F24
	/* 0x88 */ kKeyNone,	// unassigned
	/* 0x89 */ kKeyNone,	// unassigned
	/* 0x8a */ kKeyNone,	// unassigned
	/* 0x8b */ kKeyNone,	// unassigned
	/* 0x8c */ kKeyNone,	// unassigned
	/* 0x8d */ kKeyNone,	// unassigned
	/* 0x8e */ kKeyNone,	// unassigned
	/* 0x8f */ kKeyNone,	// unassigned
	/* 0x90 */ 0xff7f,		// VK_NUMLOCK		XK_Num_Lock
	/* 0x91 */ 0xff14,		// VK_SCROLL		XK_Scroll_Lock
	/* 0x92 */ kKeyNone,	// unassigned
	/* 0x93 */ kKeyNone,	// unassigned
	/* 0x94 */ kKeyNone,	// unassigned
	/* 0x95 */ kKeyNone,	// unassigned
	/* 0x96 */ kKeyNone,	// unassigned
	/* 0x97 */ kKeyNone,	// unassigned
	/* 0x98 */ kKeyNone,	// unassigned
	/* 0x99 */ kKeyNone,	// unassigned
	/* 0x9a */ kKeyNone,	// unassigned
	/* 0x9b */ kKeyNone,	// unassigned
	/* 0x9c */ kKeyNone,	// unassigned
	/* 0x9d */ kKeyNone,	// unassigned
	/* 0x9e */ kKeyNone,	// unassigned
	/* 0x9f */ kKeyNone,	// unassigned
	/* 0xa0 */ 0xffe1,		// VK_LSHIFT		XK_Shift_L
	/* 0xa1 */ 0xffe2,		// VK_RSHIFT		XK_Shift_R
	/* 0xa2 */ 0xffe3,		// VK_LCONTROL		XK_Control_L
	/* 0xa3 */ 0xffe4,		// VK_RCONTROL		XK_Control_R
	/* 0xa4 */ 0xffe9,		// VK_LMENU			XK_Alt_L
	/* 0xa5 */ 0xffea,		// VK_RMENU			XK_Alt_R
	/* 0xa6 */ kKeyNone,	// unassigned
	/* 0xa7 */ kKeyNone,	// unassigned
	/* 0xa8 */ kKeyNone,	// unassigned
	/* 0xa9 */ kKeyNone,	// unassigned
	/* 0xaa */ kKeyNone,	// unassigned
	/* 0xab */ kKeyNone,	// unassigned
	/* 0xac */ kKeyNone,	// unassigned
	/* 0xad */ kKeyNone,	// unassigned
	/* 0xae */ kKeyNone,	// unassigned
	/* 0xaf */ kKeyNone,	// unassigned
	/* 0xb0 */ kKeyNone,	// unassigned
	/* 0xb1 */ kKeyNone,	// unassigned
	/* 0xb2 */ kKeyNone,	// unassigned
	/* 0xb3 */ kKeyNone,	// unassigned
	/* 0xb4 */ kKeyNone,	// unassigned
	/* 0xb5 */ kKeyNone,	// unassigned
	/* 0xb6 */ kKeyNone,	// unassigned
	/* 0xb7 */ kKeyNone,	// unassigned
	/* 0xb8 */ kKeyNone,	// unassigned
	/* 0xb9 */ kKeyNone,	// unassigned
	/* 0xba */ kKeyNone,	// OEM specific
	/* 0xbb */ kKeyNone,	// OEM specific
	/* 0xbc */ kKeyNone,	// OEM specific
	/* 0xbd */ kKeyNone,	// OEM specific
	/* 0xbe */ kKeyNone,	// OEM specific
	/* 0xbf */ kKeyNone,	// OEM specific
	/* 0xc0 */ kKeyNone,	// OEM specific
	/* 0xc1 */ kKeyNone,	// unassigned
	/* 0xc2 */ kKeyNone,	// unassigned
	/* 0xc3 */ kKeyNone,	// unassigned
	/* 0xc4 */ kKeyNone,	// unassigned
	/* 0xc5 */ kKeyNone,	// unassigned
	/* 0xc6 */ kKeyNone,	// unassigned
	/* 0xc7 */ kKeyNone,	// unassigned
	/* 0xc8 */ kKeyNone,	// unassigned
	/* 0xc9 */ kKeyNone,	// unassigned
	/* 0xca */ kKeyNone,	// unassigned
	/* 0xcb */ kKeyNone,	// unassigned
	/* 0xcc */ kKeyNone,	// unassigned
	/* 0xcd */ kKeyNone,	// unassigned
	/* 0xce */ kKeyNone,	// unassigned
	/* 0xcf */ kKeyNone,	// unassigned
	/* 0xd0 */ kKeyNone,	// unassigned
	/* 0xd1 */ kKeyNone,	// unassigned
	/* 0xd2 */ kKeyNone,	// unassigned
	/* 0xd3 */ kKeyNone,	// unassigned
	/* 0xd4 */ kKeyNone,	// unassigned
	/* 0xd5 */ kKeyNone,	// unassigned
	/* 0xd6 */ kKeyNone,	// unassigned
	/* 0xd7 */ kKeyNone,	// unassigned
	/* 0xd8 */ kKeyNone,	// unassigned
	/* 0xd9 */ kKeyNone,	// unassigned
	/* 0xda */ kKeyNone,	// unassigned
	/* 0xdb */ kKeyNone,	// OEM specific
	/* 0xdc */ kKeyNone,	// OEM specific
	/* 0xdd */ kKeyNone,	// OEM specific
	/* 0xde */ kKeyNone,	// OEM specific
	/* 0xdf */ kKeyNone,	// OEM specific
	/* 0xe0 */ kKeyNone,	// OEM specific
	/* 0xe1 */ kKeyNone,	// OEM specific
	/* 0xe2 */ kKeyNone,	// OEM specific
	/* 0xe3 */ kKeyNone,	// OEM specific
	/* 0xe4 */ kKeyNone,	// OEM specific
	/* 0xe5 */ kKeyNone,	// unassigned
	/* 0xe6 */ kKeyNone,	// OEM specific
	/* 0xe7 */ kKeyNone,	// unassigned
	/* 0xe8 */ kKeyNone,	// unassigned
	/* 0xe9 */ kKeyNone,	// OEM specific
	/* 0xea */ kKeyNone,	// OEM specific
	/* 0xeb */ kKeyNone,	// OEM specific
	/* 0xec */ kKeyNone,	// OEM specific
	/* 0xed */ kKeyNone,	// OEM specific
	/* 0xee */ kKeyNone,	// OEM specific
	/* 0xef */ kKeyNone,	// OEM specific
	/* 0xf0 */ kKeyNone,	// OEM specific
	/* 0xf1 */ kKeyNone,	// OEM specific
	/* 0xf2 */ kKeyNone,	// OEM specific
	/* 0xf3 */ kKeyNone,	// OEM specific
	/* 0xf4 */ kKeyNone,	// OEM specific
	/* 0xf5 */ kKeyNone,	// OEM specific
	/* 0xf6 */ kKeyNone,	// VK_ATTN			
	/* 0xf7 */ kKeyNone,	// VK_CRSEL			
	/* 0xf8 */ kKeyNone,	// VK_EXSEL			
	/* 0xf9 */ kKeyNone,	// VK_EREOF			
	/* 0xfa */ kKeyNone,	// VK_PLAY			
	/* 0xfb */ kKeyNone,	// VK_ZOOM			
	/* 0xfc */ kKeyNone,	// reserved
	/* 0xfd */ kKeyNone,	// VK_PA1			
	/* 0xfe */ kKeyNone,	// VK_OEM_CLEAR		
	/* 0xff */ kKeyNone		// reserved
};

KeyID
CMSWindowsPrimaryScreen::mapKey(
	WPARAM vkCode,
	LPARAM info,
	KeyModifierMask* maskOut)
{
	// note:  known microsoft bugs
	//  Q72583 -- MapVirtualKey() maps keypad keys incorrectly
	//    95,98: num pad vk code -> invalid scan code
	//    95,98,NT4: num pad scan code -> bad vk code except
	//      SEPARATOR, MULTIPLY, SUBTRACT, ADD

	static const KeyID XK_Multi_key = 0xff20;

	assert(maskOut != NULL);

	// map modifier key
	KeyModifierMask mask = 0;
	if (((m_keys[VK_LSHIFT] |
		  m_keys[VK_RSHIFT] |
		  m_keys[VK_SHIFT]) & 0x80) != 0) {
		mask |= KeyModifierShift;
	}
	if (((m_keys[VK_LCONTROL] |
		  m_keys[VK_RCONTROL] |
		  m_keys[VK_CONTROL]) & 0x80) != 0) {
		mask |= KeyModifierControl;
	}
	if (((m_keys[VK_LMENU] |
		  m_keys[VK_RMENU] |
		  m_keys[VK_MENU]) & 0x80) != 0) {
		mask |= KeyModifierAlt;
	}
	if (((m_keys[VK_LWIN] |
		  m_keys[VK_RWIN]) & 0x80) != 0) {
		mask |= KeyModifierMeta;
	}
	if ((m_keys[VK_CAPITAL] & 0x01) != 0) {
		mask |= KeyModifierCapsLock;
	}
	if ((m_keys[VK_NUMLOCK] & 0x01) != 0) {
		mask |= KeyModifierNumLock;
	}
	if ((m_keys[VK_SCROLL] & 0x01) != 0) {
		mask |= KeyModifierScrollLock;
	}
	*maskOut = mask;
	log((CLOG_DEBUG2 "key in vk=%d info=0x%08x mask=0x%04x", vkCode, info, mask));

	// get the scan code
	UINT scanCode = static_cast<UINT>((info & 0xff0000) >> 16);

	// convert virtual key to one that distinguishes between left and
	// right for keys that have left/right versions.  known scan codes
	// that don't have left/right versions are passed through unchanged.
	// unknown scan codes return 0.
	UINT vkCode2 = MapVirtualKey(scanCode, 3);

	// work around bug Q72583 (bad num pad conversion in MapVirtualKey())
	if (vkCode >= VK_NUMPAD0 && vkCode <= VK_DIVIDE) {
		vkCode2 = vkCode;
	}

	// MapVirtualKey() appears to map VK_LWIN, VK_RWIN, VK_APPS to
	// some other meaningless virtual key.  work around that bug.
	else if (vkCode >= VK_LWIN && vkCode <= VK_APPS) {
		vkCode2 = vkCode;
	}

	// if MapVirtualKey failed then use original virtual key
	else if (vkCode2 == 0) {
		vkCode2 = vkCode;
	}

	// sadly, win32 will not distinguish between the left and right
	// control and alt keys using the above function.  however, we
	// can check for those:  if bit 24 of info is set then the key
	// is a "extended" key, such as the right control and right alt
	// keys.
	if ((info & 0x1000000) != 0) {
		switch (vkCode2) {
		case VK_CONTROL:
		case VK_LCONTROL:
			vkCode2 = VK_RCONTROL;
			break;

		case VK_MENU:
		case VK_LMENU:
			vkCode2 = VK_RMENU;
			break;
		}
	}

	// use left/right distinguishing virtual key
	vkCode = vkCode2;
	log((CLOG_DEBUG1 "key vk=%d scan=%d", vkCode, scanCode));

	// handle some keys via table lookup
	KeyID id = g_virtualKey[vkCode];
	if (id != kKeyNone) {
		return id;
	}

	// check for dead keys
	if (MapVirtualKey(vkCode, 2) >= 0x8000) {
		return XK_Multi_key;
	}

	// ToAscii() maps ctrl+letter to the corresponding control code
	// and ctrl+backspace to delete.  if we've got a control code or
	// delete then do ToAscii() again but without the control state.
	// ToAscii() interprets the control modifier state which we don't
	// want.  so save the control state then clear it.
	BYTE lControl       = m_keys[VK_LCONTROL];
	BYTE rControl       = m_keys[VK_RCONTROL];
	BYTE control        = m_keys[VK_CONTROL];
	m_keys[VK_LCONTROL] = 0;
	m_keys[VK_RCONTROL] = 0;
	m_keys[VK_CONTROL]  = 0;

	// convert to ascii
	WORD ascii;
	int result = ToAscii(vkCode, scanCode, m_keys, &ascii, 0);

	// restore control state
	m_keys[VK_LCONTROL] = lControl;
	m_keys[VK_RCONTROL] = rControl;
	m_keys[VK_CONTROL]  = control;

	// if result is less than zero then it was a dead key.  that key
	// is remembered by the keyboard which we don't want.  remove it
	// by calling ToAscii() again with arbitrary arguments.
	if (result < 0) {
		ToAscii(vkCode, scanCode, m_keys, &ascii, 0);
		return XK_Multi_key;
	}

	// if result is 1 then the key was succesfully converted
	else if (result == 1) {
		return static_cast<KeyID>(ascii & 0x00ff);
	}

	// if result is 2 then a previous dead key could not be composed.
	// put the old dead key back.
	else if (result == 2) {
		// get the scan code of the dead key and the shift state
		// required to generate it.
		vkCode = VkKeyScan(static_cast<TCHAR>(ascii & 0x00ff));

		// set shift state required to generate key
		BYTE keys[256];
		memset(keys, 0, sizeof(keys));
// FIXME -- surely these masks should be different in each if expression
		if (vkCode & 0x0100) {
			keys[VK_SHIFT] = 0x80;
		}
		if (vkCode & 0x0100) {
			keys[VK_CONTROL] = 0x80;
		}
		if (vkCode & 0x0100) {
			keys[VK_MENU] = 0x80;
		}

		// strip shift state off of virtual key code
		vkCode &= 0x00ff;

		// get the scan code for the key
		scanCode = MapVirtualKey(vkCode, 0);

		// put it back
		ToAscii(vkCode, scanCode, keys, &ascii, 0);
		return XK_Multi_key;
	}

	// cannot convert key
	return kKeyNone;
}

ButtonID
CMSWindowsPrimaryScreen::mapButton(WPARAM button) const
{
	switch (button) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		return kButtonLeft;

	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		return kButtonMiddle;

	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		return kButtonRight;

	default:
		return kButtonNone;
	}
}

void
CMSWindowsPrimaryScreen::updateKeys()
{
	// not using GetKeyboardState() because that doesn't seem to give
	// up-to-date results.  i don't know why that is or why GetKeyState()
	// should give different results.

	// clear key state
	memset(m_keys, 0, sizeof(m_keys));

	// we only care about the modifier key states
	m_keys[VK_LSHIFT]   = static_cast<BYTE>(GetKeyState(VK_LSHIFT));
	m_keys[VK_RSHIFT]   = static_cast<BYTE>(GetKeyState(VK_RSHIFT));
	m_keys[VK_SHIFT]    = static_cast<BYTE>(GetKeyState(VK_SHIFT));
	m_keys[VK_LCONTROL] = static_cast<BYTE>(GetKeyState(VK_LCONTROL));
	m_keys[VK_RCONTROL] = static_cast<BYTE>(GetKeyState(VK_RCONTROL));
	m_keys[VK_CONTROL]  = static_cast<BYTE>(GetKeyState(VK_CONTROL));
	m_keys[VK_LMENU]    = static_cast<BYTE>(GetKeyState(VK_LMENU));
	m_keys[VK_RMENU]    = static_cast<BYTE>(GetKeyState(VK_RMENU));
	m_keys[VK_MENU]     = static_cast<BYTE>(GetKeyState(VK_MENU));
	m_keys[VK_LWIN]     = static_cast<BYTE>(GetKeyState(VK_LWIN));
	m_keys[VK_RWIN]     = static_cast<BYTE>(GetKeyState(VK_RWIN));
	m_keys[VK_APPS]     = static_cast<BYTE>(GetKeyState(VK_APPS));
	m_keys[VK_CAPITAL]  = static_cast<BYTE>(GetKeyState(VK_CAPITAL));
	m_keys[VK_NUMLOCK]  = static_cast<BYTE>(GetKeyState(VK_NUMLOCK));
	m_keys[VK_SCROLL]   = static_cast<BYTE>(GetKeyState(VK_SCROLL));
}

void
CMSWindowsPrimaryScreen::updateKey(UINT vkCode, bool press)
{
	if (press) {
		switch (vkCode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_SHIFT:
			m_keys[vkCode]     |= 0x80;
			m_keys[VK_SHIFT]   |= 0x80;
			break;

		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_CONTROL:
			m_keys[vkCode]     |= 0x80;
			m_keys[VK_CONTROL] |= 0x80;
			break;

		case VK_LMENU:
		case VK_RMENU:
		case VK_MENU:
			m_keys[vkCode]     |= 0x80;
			m_keys[VK_MENU]    |= 0x80;
			break;

		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
			m_keys[vkCode]     |= 0x80;
			break;

		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
			// toggle keys
			m_keys[vkCode]     |= 0x80;
			break;
		}
	}
	else {
		switch (vkCode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_SHIFT:
			m_keys[vkCode]     &= ~0x80;
			if (((m_keys[VK_LSHIFT] | m_keys[VK_RSHIFT]) & 0x80) == 0) {
				m_keys[VK_SHIFT] &= ~0x80;
			}
			break;

		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_CONTROL:
			m_keys[vkCode]     &= ~0x80;
			if (((m_keys[VK_LCONTROL] | m_keys[VK_RCONTROL]) & 0x80) == 0) {
				m_keys[VK_CONTROL] &= ~0x80;
			}
			break;

		case VK_LMENU:
		case VK_RMENU:
		case VK_MENU:
			m_keys[vkCode]     &= ~0x80;
			if (((m_keys[VK_LMENU] | m_keys[VK_RMENU]) & 0x80) == 0) {
				m_keys[VK_MENU] &= ~0x80;
			}
			break;

		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
			m_keys[vkCode]     &= ~0x80;
			break;

		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
			// toggle keys
			m_keys[vkCode] &= ~0x80;
			m_keys[vkCode] ^=  0x01;
			break;
		}
	}
}
