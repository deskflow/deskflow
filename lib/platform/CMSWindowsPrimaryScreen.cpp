/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CMSWindowsPrimaryScreen.h"
#include "CMSWindowsScreen.h"
#include "IPrimaryScreenReceiver.h"
#include "XScreen.h"
#include "CLog.h"
#include "CArchMiscWindows.h"
#include <cstring>

//
// CMSWindowsPrimaryScreen
//

CMSWindowsPrimaryScreen::CMSWindowsPrimaryScreen(
				IScreenReceiver* receiver,
				IPrimaryScreenReceiver* primaryReceiver) :
	CPrimaryScreen(receiver),
	m_receiver(primaryReceiver),
	m_is95Family(CArchMiscWindows::isWindows95Family()),
	m_threadID(0),
	m_window(NULL),
	m_mark(0),
	m_markReceived(0)
{
	assert(m_receiver != NULL);

	// load the hook library
	m_hookLibrary = LoadLibrary("synrgyhk");
	if (m_hookLibrary == NULL) {
		LOG((CLOG_ERR "Failed to load hook library;  synrgyhk.dll is missing"));
		throw XScreenOpenFailure();
	}
	m_setSides  = (SetSidesFunc)GetProcAddress(m_hookLibrary, "setSides");
	m_setZone   = (SetZoneFunc)GetProcAddress(m_hookLibrary, "setZone");
	m_setRelay  = (SetRelayFunc)GetProcAddress(m_hookLibrary, "setRelay");
	m_install   = (InstallFunc)GetProcAddress(m_hookLibrary, "install");
	m_uninstall = (UninstallFunc)GetProcAddress(m_hookLibrary, "uninstall");
	m_init      = (InitFunc)GetProcAddress(m_hookLibrary, "init");
	m_cleanup   = (CleanupFunc)GetProcAddress(m_hookLibrary, "cleanup");
	if (m_setSides  == NULL ||
		m_setZone   == NULL ||
		m_setRelay  == NULL ||
		m_install   == NULL ||
		m_uninstall == NULL ||
		m_init      == NULL ||
		m_cleanup   == NULL) {
		LOG((CLOG_ERR "Invalid hook library;  use a newer synrgyhk.dll"));
		FreeLibrary(m_hookLibrary);
		throw XScreenOpenFailure();
	}

	// create screen
	m_screen = new CMSWindowsScreen(receiver, this);
}

CMSWindowsPrimaryScreen::~CMSWindowsPrimaryScreen()
{
	assert(m_hookLibrary != NULL);
	assert(m_window      == NULL);

	delete m_screen;
	FreeLibrary(m_hookLibrary);
}

void
CMSWindowsPrimaryScreen::reconfigure(UInt32 activeSides)
{
	m_setSides(activeSides);
}

void
CMSWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	// warp mouse
	warpCursorNoFlush(x, y);

	// remove all input events before and including warp
	MSG msg;
	while (PeekMessage(&msg, NULL, SYNERGY_MSG_INPUT_FIRST,
								SYNERGY_MSG_INPUT_LAST, PM_REMOVE)) {
		// do nothing
	}

	// save position as last position
	m_x = x;
	m_y = y;
}

void
CMSWindowsPrimaryScreen::resetOptions()
{
	// no options
}

void
CMSWindowsPrimaryScreen::setOptions(const COptionsList& /*options*/)
{
	// no options
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
	// virtual key table.  the table defines the virtual keys that are
	// mapped to something (including mouse buttons, OEM and kanji keys
	// but not unassigned or undefined keys).
	static const UInt32 s_mappedKeys[] = {
		0xfbff331e,
		0x03ffffff,
		0x3ffffffe,
		0xffffffff,
		0x000300ff,
		0xfc000000,
		0xf8000001,
		0x7ffffe5f
	};

	// check each key.  note that we cannot use GetKeyboardState() here
	// since it reports the state of keys according to key messages
	// that have been pulled off the queue.  in general, we won't get
	// these key messages because they're not for our window.  if any
	// key (or mouse button) is down then we're locked to the screen.
	if (isActive()) {
		// use shadow keyboard state in m_keys
		for (UInt32 i = 0; i < 256; ++i) {
			if ((m_keys[i] & 0x80) != 0) {
				return true;
			}
		}
	}
	else {
		for (UInt32 i = 0; i < 256 / 32; ++i) {
			for (UInt32 b = 1, j = 0; j < 32; b <<= 1, ++j) {
				if ((s_mappedKeys[i] & b) != 0) {
					if (GetAsyncKeyState(i * 32 + j) < 0) {
						return true;
					}
				}
			}
		}
	}

	// not locked
	return false;
}

IScreen*
CMSWindowsPrimaryScreen::getScreen() const
{
	return m_screen;
}

void
CMSWindowsPrimaryScreen::onScreensaver(bool activated)
{
	m_receiver->onScreensaver(activated);
}

bool
CMSWindowsPrimaryScreen::onPreDispatch(const CEvent* event)
{
	assert(event != NULL);

	// handle event
	const MSG* msg = &event->m_msg;
	switch (msg->message) {
	case SYNERGY_MSG_MARK:
		m_markReceived = msg->wParam;
		return true;

	case SYNERGY_MSG_KEY:
		// ignore message if posted prior to last mark change
		if (!ignore()) {
			KeyModifierMask mask;
			const KeyID key = mapKey(msg->wParam, msg->lParam, &mask);
			if (key != kKeyNone) {
				if ((msg->lParam & 0x80000000) == 0) {
					// key press
					const SInt32 repeat = (SInt32)(msg->lParam & 0xffff);
					if (repeat >= 2) {
						LOG((CLOG_DEBUG1 "event: key repeat key=%d mask=0x%04x count=%d", key, mask, repeat));
						m_receiver->onKeyRepeat(key, mask, repeat);
					}
					else {
						LOG((CLOG_DEBUG1 "event: key press key=%d mask=0x%04x", key, mask));
						m_receiver->onKeyDown(key, mask);
					}

					// update key state
					updateKey(msg->wParam, true);
				}
				else {
					// key release
					LOG((CLOG_DEBUG1 "event: key release key=%d mask=0x%04x", key, mask));
					m_receiver->onKeyUp(key, mask);

					// update key state
					updateKey(msg->wParam, false);
				}
			}
			else {
				LOG((CLOG_DEBUG2 "event: cannot map key wParam=%d lParam=0x%08x", msg->wParam, msg->lParam));
			}
		}
		return true;

	case SYNERGY_MSG_MOUSE_BUTTON:
		// ignore message if posted prior to last mark change
		if (!ignore()) {
			static const int s_vkButton[] = {
				0,				// kButtonNone
				VK_LBUTTON,		// kButtonLeft, etc.
				VK_MBUTTON,
				VK_RBUTTON
			};

			const ButtonID button = mapButton(msg->wParam);
			switch (msg->wParam) {
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
				LOG((CLOG_DEBUG1 "event: button press button=%d", button));
				if (button != kButtonNone) {
					m_receiver->onMouseDown(button);
					m_keys[s_vkButton[button]] |= 0x80;
				}
				break;

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
				LOG((CLOG_DEBUG1 "event: button release button=%d", button));
				if (button != kButtonNone) {
					m_receiver->onMouseUp(button);
					m_keys[s_vkButton[button]] &= ~0x80;
				}
				break;
			}
		}
		return true;

	case SYNERGY_MSG_MOUSE_WHEEL:
		// ignore message if posted prior to last mark change
		if (!ignore()) {
			LOG((CLOG_DEBUG1 "event: button wheel delta=%d %d", msg->wParam, msg->lParam));
			m_receiver->onMouseWheel(msg->wParam);
		}
		return true;

	case SYNERGY_MSG_PRE_WARP:
		{
			// save position to compute delta of next motion
			m_x = static_cast<SInt32>(msg->wParam);
			m_y = static_cast<SInt32>(msg->lParam);

			// we warped the mouse.  discard events until we find the
			// matching post warp event.  see warpCursorNoFlush() for
			// where the events are sent.  we discard the matching
			// post warp event and can be sure we've skipped the warp
			// event.
			MSG msg;
			do {
				GetMessage(&msg, NULL, SYNERGY_MSG_MOUSE_MOVE,
										SYNERGY_MSG_POST_WARP);
			} while (msg.message != SYNERGY_MSG_POST_WARP);

			return true;
		}

	case SYNERGY_MSG_POST_WARP:
		LOG((CLOG_WARN "unmatched post warp"));
		return true;

	case SYNERGY_MSG_MOUSE_MOVE:
		// ignore message if posted prior to last mark change
		if (!ignore()) {
			// compute motion delta (relative to the last known
			// mouse position)
			SInt32 x = static_cast<SInt32>(msg->wParam) - m_x;
			SInt32 y = static_cast<SInt32>(msg->lParam) - m_y;

			// save position to compute delta of next motion
			m_x = static_cast<SInt32>(msg->wParam);
			m_y = static_cast<SInt32>(msg->lParam);

			if (!isActive()) {
				// motion on primary screen
				m_receiver->onMouseMovePrimary(m_x, m_y);
			}
			else {
				// motion on secondary screen.  warp mouse back to
				// center.
				if (x != 0 || y != 0) {
					// back to center
					warpCursorNoFlush(m_xCenter, m_yCenter);

					// send motion
					m_receiver->onMouseMoveSecondary(x, y);
				}
			}
		}
		return true;
	}

	return false;
}

bool
CMSWindowsPrimaryScreen::onEvent(CEvent* event)
{
	assert(event != NULL);

	const MSG& msg = event->m_msg;
	switch (msg.message) {
	case WM_DISPLAYCHANGE:
		// recompute center pixel of primary screen
		m_screen->getCursorCenter(m_xCenter, m_yCenter);

		// warp mouse to center if active
		if (isActive()) {
			warpCursorToCenter();
		}

		// tell hook about resize if not active
		else {
			SInt32 x, y, w, h;
			m_screen->getShape(x, y, w, h);
			m_setZone(x, y, w, h, getJumpZoneSize());
		}
		return true;
	}

	return false;
}

SInt32
CMSWindowsPrimaryScreen::getJumpZoneSize() const
{
	return 1;
}

void
CMSWindowsPrimaryScreen::postCreateWindow(HWND window)
{
	// save window
	m_window = window;

	// install hooks
	m_install();

	// resize window
	// note -- we use a fullscreen window to grab input.  it should
	// be possible to use a 1x1 window but i've run into problems
	// with losing keyboard input (focus?) in that case.
	// unfortunately, hiding the full screen window (when entering
	// the screen) causes all other windows to redraw.
	SInt32 x, y, w, h;
	m_screen->getShape(x, y, w, h);
	MoveWindow(m_window, x, y, w, h, FALSE);

	if (isActive()) {
		// hide the cursor
		showWindow();
	}
	else {
		// watch jump zones
		m_setRelay(false);

		// all messages prior to now are invalid
		nextMark();
	}
}

void
CMSWindowsPrimaryScreen::preDestroyWindow(HWND)
{
	// hide the window if it's visible
	if (isActive()) {
		hideWindow();
	}

	// uninstall hooks
	m_uninstall();
}

void
CMSWindowsPrimaryScreen::onPreMainLoop()
{
	// must call mainLoop() from same thread as open()
	assert(m_threadID == GetCurrentThreadId());
	assert(m_window   != NULL);
}

void
CMSWindowsPrimaryScreen::onPreOpen()
{
	assert(m_window == NULL);

	// initialize hook library
	m_threadID = GetCurrentThreadId();
	if (m_init(m_threadID) == 0) {
		LOG((CLOG_ERR "Cannot initialize hook library;  is synergy already running?"));
		throw XScreenOpenFailure();
	}
}

void
CMSWindowsPrimaryScreen::onPostOpen()
{
	// get cursor info
	m_screen->getCursorPos(m_x, m_y);
	m_screen->getCursorCenter(m_xCenter, m_yCenter);

	// set jump zones
	SInt32 x, y, w, h;
	m_screen->getShape(x, y, w, h);
	m_setZone(x, y, w, h, getJumpZoneSize());

	// initialize marks
	m_mark         = 0;
	m_markReceived = 0;
	nextMark();
}

void
CMSWindowsPrimaryScreen::onPostClose()
{
	m_cleanup();
	m_threadID = 0;
}

void
CMSWindowsPrimaryScreen::onPreEnter()
{
	assert(m_window != NULL);

	// enable ctrl+alt+del, alt+tab, etc
	if (m_is95Family) {
		DWORD dummy = 0;
		SystemParametersInfo(SPI_SETSCREENSAVERRUNNING, FALSE, &dummy, 0);
	}

	// watch jump zones
	m_setRelay(false);
}

void
CMSWindowsPrimaryScreen::onPostEnter()
{
	// all messages prior to now are invalid
	nextMark();
}

void
CMSWindowsPrimaryScreen::onPreLeave()
{
	assert(m_window != NULL);

	// all messages prior to now are invalid
	nextMark();
}

void
CMSWindowsPrimaryScreen::onPostLeave(bool success)
{
	if (success) {
		// relay all mouse and keyboard events
		m_setRelay(true);

		// disable ctrl+alt+del, alt+tab, etc
		if (m_is95Family) {
			DWORD dummy = 0;
			SystemParametersInfo(SPI_SETSCREENSAVERRUNNING, TRUE, &dummy, 0);
		}
	}
}

void
CMSWindowsPrimaryScreen::createWindow()
{
	// open the desktop and the window
	m_window = m_screen->openDesktop();
	if (m_window == NULL) {
		throw XScreenOpenFailure();
	}

	// note -- we use a fullscreen window to grab input.  it should
	// be possible to use a 1x1 window but i've run into problems
	// with losing keyboard input (focus?) in that case.
	// unfortunately, hiding the full screen window (when entering
	// the scren causes all other windows to redraw).
	SInt32 x, y, w, h;
	m_screen->getShape(x, y, w, h);
	MoveWindow(m_window, x, y, w, h, FALSE);
}

void
CMSWindowsPrimaryScreen::destroyWindow()
{
	// close the desktop and the window
	m_screen->closeDesktop();
	m_window = NULL;
}

bool
CMSWindowsPrimaryScreen::showWindow()
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
	DWORD myThread         = GetCurrentThreadId();
	if (m_lastActiveThread != 0) {
		if (myThread != m_lastActiveThread) {
			if (AttachThreadInput(myThread, m_lastActiveThread, TRUE)) {
				m_lastActiveWindow = GetActiveWindow();
				AttachThreadInput(myThread, m_lastActiveThread, FALSE);
			}
		}
	}

	// show our window
	ShowWindow(m_window, SW_SHOW);

	// force our window to the foreground.  this is necessary to
	// capture input but is complicated by microsoft's misguided
	// attempt to prevent applications from changing the
	// foreground window.  (the user should be in control of that
	// under normal circumstances but there are exceptions;  the
	// good folks at microsoft, after abusing the previously
	// available ability to switch foreground tasks in many of
	// their apps, changed the behavior to prevent it.  maybe
	// it was easier than fixing the applications.)
	//
	// anyway, simply calling SetForegroundWindow() doesn't work
	// unless there is no foreground window or we already are the
	// foreground window.  so we AttachThreadInput() to the
	// foreground process then call SetForegroundWindow();  that
	// makes Windows think the foreground process changed the
	// foreground window which is allowed since the foreground
	// is "voluntarily" yielding control.  then we unattach the
	// thread input and go about our business.
	//
	// unfortunately, this still doesn't work for console windows
	// on the windows 95 family.  if a console is the foreground
	// app on the server when the user leaves the server screen
	// then the keyboard will not be captured by synergy.
	if (m_lastActiveThread != myThread) {
		if (m_lastActiveThread != 0) {
			AttachThreadInput(myThread, m_lastActiveThread, TRUE);
		}
		SetForegroundWindow(m_window);
		if (m_lastActiveThread != 0) {
			AttachThreadInput(myThread, m_lastActiveThread, FALSE);
		}
	}

	// get keyboard input and capture mouse
	SetActiveWindow(m_window);
	SetFocus(m_window);
	SetCapture(m_window);

	return true;
}

void
CMSWindowsPrimaryScreen::hideWindow()
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

	// hide the window.  do not wait for it, though, since ShowWindow()
	// waits for the event loop to process the show-window event, but
	// that thread may need to lock the mutex that this thread has
	// already locked.  in particular, that deadlock will occur unless
	// we use the asynchronous version of show window when a client
	// disconnects:  thread A will lock the mutex and enter the primary
	// screen which warps the mouse and calls this method while thread B
	// will handle the mouse warp event and call methods that try to
	// lock the mutex.  thread A owns the mutex and is waiting for the
	// event loop, thread B owns the event loop and is waiting for the
	// mutex causing deadlock.
	ShowWindowAsync(m_window, SW_HIDE);
}

void
CMSWindowsPrimaryScreen::warpCursorToCenter()
{
	warpCursor(m_xCenter, m_yCenter);
}

void
CMSWindowsPrimaryScreen::warpCursorNoFlush(SInt32 x, SInt32 y)
{
	// send an event that we can recognize before the mouse warp
	PostThreadMessage(m_threadID, SYNERGY_MSG_PRE_WARP, x, y);

	// warp mouse.  hopefully this inserts a mouse motion event
	// between the previous message and the following message.
	SetCursorPos(x, y);

	// send an event that we can recognize after the mouse warp
	PostThreadMessage(m_threadID, SYNERGY_MSG_POST_WARP, 0, 0);
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
CMSWindowsPrimaryScreen::ignore() const
{
	return (m_mark != m_markReceived);
}

// map virtual keys to synergy key enumeration.  use extended keyboard
// bit to distinguish some keys.
static const KeyID		g_virtualKey[][2] =
{
	/* 0x00 */ kKeyNone,		kKeyNone,		// reserved
	/* 0x01 */ kKeyNone,		kKeyNone,		// VK_LBUTTON
	/* 0x02 */ kKeyNone,		kKeyNone,		// VK_RBUTTON
	/* 0x03 */ kKeyNone,		kKeyBreak,		// VK_CANCEL
	/* 0x04 */ kKeyNone,		kKeyNone,		// VK_MBUTTON
	/* 0x05 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x06 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x07 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x08 */ kKeyBackSpace,	kKeyNone,		// VK_BACK
	/* 0x09 */ kKeyTab,			kKeyNone,		// VK_TAB
	/* 0x0a */ kKeyNone,		kKeyNone,		// undefined
	/* 0x0b */ kKeyNone,		kKeyNone,		// undefined
	/* 0x0c */ kKeyClear,		kKeyClear,		// VK_CLEAR
	/* 0x0d */ kKeyReturn,		kKeyKP_Enter,	// VK_RETURN
	/* 0x0e */ kKeyNone,		kKeyNone,		// undefined
	/* 0x0f */ kKeyNone,		kKeyNone,		// undefined
	/* 0x10 */ kKeyShift_L,		kKeyShift_R,	// VK_SHIFT
	/* 0x11 */ kKeyControl_L,	kKeyControl_R,	// VK_CONTROL
	/* 0x12 */ kKeyAlt_L,		kKeyAlt_R,		// VK_MENU
	/* 0x13 */ kKeyPause,		kKeyNone,		// VK_PAUSE
	/* 0x14 */ kKeyCapsLock,	kKeyNone,		// VK_CAPITAL
	/* 0x15 */ kKeyNone,		kKeyNone,		// VK_KANA			
	/* 0x16 */ kKeyNone,		kKeyNone,		// VK_HANGUL		
	/* 0x17 */ kKeyNone,		kKeyNone,		// VK_JUNJA			
	/* 0x18 */ kKeyNone,		kKeyNone,		// VK_FINAL			
	/* 0x19 */ kKeyNone,		kKeyNone,		// VK_KANJI			
	/* 0x1a */ kKeyNone,		kKeyNone,		// undefined
	/* 0x1b */ kKeyEscape,		kKeyNone,		// VK_ESCAPE
	/* 0x1c */ kKeyNone,		kKeyNone,		// VK_CONVERT		
	/* 0x1d */ kKeyNone,		kKeyNone,		// VK_NONCONVERT	
	/* 0x1e */ kKeyNone,		kKeyNone,		// VK_ACCEPT		
	/* 0x1f */ kKeyNone,		kKeyNone,		// VK_MODECHANGE	
	/* 0x20 */ 0x0020,			kKeyNone,		// VK_SPACE
	/* 0x21 */ kKeyKP_PageUp,	kKeyPageUp,		// VK_PRIOR
	/* 0x22 */ kKeyKP_PageDown,	kKeyPageDown,	// VK_NEXT
	/* 0x23 */ kKeyKP_End,		kKeyEnd,		// VK_END
	/* 0x24 */ kKeyKP_Home,		kKeyHome,		// VK_HOME
	/* 0x25 */ kKeyKP_Left,		kKeyLeft,		// VK_LEFT
	/* 0x26 */ kKeyKP_Up,		kKeyUp,			// VK_UP
	/* 0x27 */ kKeyKP_Right,	kKeyRight,		// VK_RIGHT
	/* 0x28 */ kKeyKP_Down,		kKeyDown,		// VK_DOWN
	/* 0x29 */ kKeySelect,		kKeySelect,		// VK_SELECT
	/* 0x2a */ kKeyNone,		kKeyNone,		// VK_PRINT
	/* 0x2b */ kKeyExecute,		kKeyExecute,	// VK_EXECUTE
	/* 0x2c */ kKeyPrint,		kKeyPrint,		// VK_SNAPSHOT
	/* 0x2d */ kKeyKP_Insert,	kKeyInsert,		// VK_INSERT
	/* 0x2e */ kKeyKP_Delete,	kKeyDelete,		// VK_DELETE
	/* 0x2f */ kKeyHelp,		kKeyHelp,		// VK_HELP
	/* 0x30 */ kKeyNone,		kKeyNone,		// VK_0
	/* 0x31 */ kKeyNone,		kKeyNone,		// VK_1
	/* 0x32 */ kKeyNone,		kKeyNone,		// VK_2
	/* 0x33 */ kKeyNone,		kKeyNone,		// VK_3
	/* 0x34 */ kKeyNone,		kKeyNone,		// VK_4
	/* 0x35 */ kKeyNone,		kKeyNone,		// VK_5
	/* 0x36 */ kKeyNone,		kKeyNone,		// VK_6
	/* 0x37 */ kKeyNone,		kKeyNone,		// VK_7
	/* 0x38 */ kKeyNone,		kKeyNone,		// VK_8
	/* 0x39 */ kKeyNone,		kKeyNone,		// VK_9
	/* 0x3a */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3b */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3c */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3d */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3e */ kKeyNone,		kKeyNone,		// undefined
	/* 0x3f */ kKeyNone,		kKeyNone,		// undefined
	/* 0x40 */ kKeyNone,		kKeyNone,		// undefined
	/* 0x41 */ kKeyNone,		kKeyNone,		// VK_A
	/* 0x42 */ kKeyNone,		kKeyNone,		// VK_B
	/* 0x43 */ kKeyNone,		kKeyNone,		// VK_C
	/* 0x44 */ kKeyNone,		kKeyNone,		// VK_D
	/* 0x45 */ kKeyNone,		kKeyNone,		// VK_E
	/* 0x46 */ kKeyNone,		kKeyNone,		// VK_F
	/* 0x47 */ kKeyNone,		kKeyNone,		// VK_G
	/* 0x48 */ kKeyNone,		kKeyNone,		// VK_H
	/* 0x49 */ kKeyNone,		kKeyNone,		// VK_I
	/* 0x4a */ kKeyNone,		kKeyNone,		// VK_J
	/* 0x4b */ kKeyNone,		kKeyNone,		// VK_K
	/* 0x4c */ kKeyNone,		kKeyNone,		// VK_L
	/* 0x4d */ kKeyNone,		kKeyNone,		// VK_M
	/* 0x4e */ kKeyNone,		kKeyNone,		// VK_N
	/* 0x4f */ kKeyNone,		kKeyNone,		// VK_O
	/* 0x50 */ kKeyNone,		kKeyNone,		// VK_P
	/* 0x51 */ kKeyNone,		kKeyNone,		// VK_Q
	/* 0x52 */ kKeyNone,		kKeyNone,		// VK_R
	/* 0x53 */ kKeyNone,		kKeyNone,		// VK_S
	/* 0x54 */ kKeyNone,		kKeyNone,		// VK_T
	/* 0x55 */ kKeyNone,		kKeyNone,		// VK_U
	/* 0x56 */ kKeyNone,		kKeyNone,		// VK_V
	/* 0x57 */ kKeyNone,		kKeyNone,		// VK_W
	/* 0x58 */ kKeyNone,		kKeyNone,		// VK_X
	/* 0x59 */ kKeyNone,		kKeyNone,		// VK_Y
	/* 0x5a */ kKeyNone,		kKeyNone,		// VK_Z
	/* 0x5b */ kKeyNone,		kKeySuper_L,	// VK_LWIN
	/* 0x5c */ kKeyNone,		kKeySuper_R,	// VK_RWIN
	/* 0x5d */ kKeyMenu,		kKeyMenu,		// VK_APPS
	/* 0x5e */ kKeyNone,		kKeyNone,		// undefined
	/* 0x5f */ kKeyNone,		kKeyNone,		// undefined
	/* 0x60 */ kKeyKP_0,		kKeyNone,		// VK_NUMPAD0
	/* 0x61 */ kKeyKP_1,		kKeyNone,		// VK_NUMPAD1
	/* 0x62 */ kKeyKP_2,		kKeyNone,		// VK_NUMPAD2
	/* 0x63 */ kKeyKP_3,		kKeyNone,		// VK_NUMPAD3
	/* 0x64 */ kKeyKP_4,		kKeyNone,		// VK_NUMPAD4
	/* 0x65 */ kKeyKP_5,		kKeyNone,		// VK_NUMPAD5
	/* 0x66 */ kKeyKP_6,		kKeyNone,		// VK_NUMPAD6
	/* 0x67 */ kKeyKP_7,		kKeyNone,		// VK_NUMPAD7
	/* 0x68 */ kKeyKP_8,		kKeyNone,		// VK_NUMPAD8
	/* 0x69 */ kKeyKP_9,		kKeyNone,		// VK_NUMPAD9
	/* 0x6a */ kKeyKP_Multiply,	kKeyNone,		// VK_MULTIPLY
	/* 0x6b */ kKeyKP_Add,		kKeyNone,		// VK_ADD
	/* 0x6c */ kKeyKP_Separator,kKeyKP_Separator,// VK_SEPARATOR
	/* 0x6d */ kKeyKP_Subtract,	kKeyNone,		// VK_SUBTRACT
	/* 0x6e */ kKeyKP_Decimal,	kKeyNone,		// VK_DECIMAL
	/* 0x6f */ kKeyNone,		kKeyKP_Divide,	// VK_DIVIDE
	/* 0x70 */ kKeyF1,			kKeyNone,		// VK_F1
	/* 0x71 */ kKeyF2,			kKeyNone,		// VK_F2
	/* 0x72 */ kKeyF3,			kKeyNone,		// VK_F3
	/* 0x73 */ kKeyF4,			kKeyNone,		// VK_F4
	/* 0x74 */ kKeyF5,			kKeyNone,		// VK_F5
	/* 0x75 */ kKeyF6,			kKeyNone,		// VK_F6
	/* 0x76 */ kKeyF7,			kKeyNone,		// VK_F7
	/* 0x77 */ kKeyF8,			kKeyNone,		// VK_F8
	/* 0x78 */ kKeyF9,			kKeyNone,		// VK_F9
	/* 0x79 */ kKeyF10,			kKeyNone,		// VK_F10
	/* 0x7a */ kKeyF11,			kKeyNone,		// VK_F11
	/* 0x7b */ kKeyF12,			kKeyNone,		// VK_F12
	/* 0x7c */ kKeyF13,			kKeyF13,		// VK_F13
	/* 0x7d */ kKeyF14,			kKeyF14,		// VK_F14
	/* 0x7e */ kKeyF15,			kKeyF15,		// VK_F15
	/* 0x7f */ kKeyF16,			kKeyF16,		// VK_F16
	/* 0x80 */ kKeyF17,			kKeyF17,		// VK_F17
	/* 0x81 */ kKeyF18,			kKeyF18,		// VK_F18
	/* 0x82 */ kKeyF19,			kKeyF19,		// VK_F19
	/* 0x83 */ kKeyF20,			kKeyF20,		// VK_F20
	/* 0x84 */ kKeyF21,			kKeyF21,		// VK_F21
	/* 0x85 */ kKeyF22,			kKeyF22,		// VK_F22
	/* 0x86 */ kKeyF23,			kKeyF23,		// VK_F23
	/* 0x87 */ kKeyF24,			kKeyF24,		// VK_F24
	/* 0x88 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x89 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8a */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8b */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8c */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8d */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8e */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x8f */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x90 */ kKeyNumLock,		kKeyNumLock,	// VK_NUMLOCK
	/* 0x91 */ kKeyScrollLock,	kKeyNone,		// VK_SCROLL
	/* 0x92 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x93 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x94 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x95 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x96 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x97 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x98 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x99 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9a */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9b */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9c */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9d */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9e */ kKeyNone,		kKeyNone,		// unassigned
	/* 0x9f */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xa0 */ kKeyShift_L,		kKeyShift_L,	// VK_LSHIFT
	/* 0xa1 */ kKeyShift_R,		kKeyShift_R,	// VK_RSHIFT
	/* 0xa2 */ kKeyControl_L,	kKeyControl_L,	// VK_LCONTROL
	/* 0xa3 */ kKeyControl_R,	kKeyControl_R,	// VK_RCONTROL
	/* 0xa4 */ kKeyAlt_L,		kKeyAlt_L,		// VK_LMENU
	/* 0xa5 */ kKeyAlt_R,		kKeyAlt_R,		// VK_RMENU
	/* 0xa6 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xa7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xa8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xa9 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xaa */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xab */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xac */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xad */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xae */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xaf */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb0 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb1 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb2 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb3 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb4 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb5 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb6 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xb9 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xba */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbb */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbc */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbd */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbe */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xbf */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xc0 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xc1 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc2 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc3 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc4 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc5 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc6 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xc9 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xca */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcb */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcc */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcd */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xce */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xcf */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd0 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd1 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd2 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd3 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd4 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd5 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd6 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xd9 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xda */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xdb */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xdc */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xdd */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xde */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xdf */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe0 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe1 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe2 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe3 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe4 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe5 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xe6 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xe7 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xe8 */ kKeyNone,		kKeyNone,		// unassigned
	/* 0xe9 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xea */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xeb */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xec */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xed */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xee */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xef */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf0 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf1 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf2 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf3 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf4 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf5 */ kKeyNone,		kKeyNone,		// OEM specific
	/* 0xf6 */ kKeyNone,		kKeyNone,		// VK_ATTN			
	/* 0xf7 */ kKeyNone,		kKeyNone,		// VK_CRSEL			
	/* 0xf8 */ kKeyNone,		kKeyNone,		// VK_EXSEL			
	/* 0xf9 */ kKeyNone,		kKeyNone,		// VK_EREOF			
	/* 0xfa */ kKeyNone,		kKeyNone,		// VK_PLAY			
	/* 0xfb */ kKeyNone,		kKeyNone,		// VK_ZOOM			
	/* 0xfc */ kKeyNone,		kKeyNone,		// reserved
	/* 0xfd */ kKeyNone,		kKeyNone,		// VK_PA1			
	/* 0xfe */ kKeyNone,		kKeyNone,		// VK_OEM_CLEAR		
	/* 0xff */ kKeyNone,		kKeyNone		// reserved
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
	if ((m_keys[VK_RMENU] & 0x80) != 0) {
		// right alt => AltGr on windows
		mask |= KeyModifierModeSwitch;
	}
	else if (((m_keys[VK_LMENU] |
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
	// ctrl+alt => AltGr on windows
/* don't convert ctrl+alt to mode switch.  if we do that then we can
 * never send ctrl+alt+[key] from windows to some platform that
 * doesn't treat ctrl+alt as mode switch (i.e. all other platforms).
 * instead, let windows clients automatically treat ctrl+alt as
 * AltGr and let other clients use ctrl+alt as is.  the right alt
 * key serves as a mode switch key.
	if ((mask & (KeyModifierControl | KeyModifierAlt)) ==
				(KeyModifierControl | KeyModifierAlt)) {
		mask |= KeyModifierModeSwitch;
		mask &= ~(KeyModifierControl | KeyModifierAlt);
	}
*/
	*maskOut = mask;
	LOG((CLOG_DEBUG2 "key in vk=%d info=0x%08x mask=0x%04x", vkCode, info, mask));

	// get the scan code and the extended keyboard flag
	UINT scanCode = static_cast<UINT>((info & 0x00ff0000u) >> 16);
	int extended  = ((info & 0x01000000) == 0) ? 0 : 1;
	LOG((CLOG_DEBUG1 "key vk=%d ext=%d scan=%d", vkCode, extended, scanCode));

	// handle some keys via table lookup
	KeyID id = g_virtualKey[vkCode][extended];
	if (id != kKeyNone) {
		return id;
	}

	// check for dead keys
	if (MapVirtualKey(vkCode, 2) >= 0x8000) {
		return kKeyMultiKey;
	}

	// save the control state then clear it.  ToAscii() maps ctrl+letter
	// to the corresponding control code and ctrl+backspace to delete.
	// we don't want that translation so we clear the control modifier
	// state.  however, if we want to simulate AltGr (which is ctrl+alt)
	// then we must not clear it.
	BYTE lControl = m_keys[VK_LCONTROL];
	BYTE rControl = m_keys[VK_RCONTROL];
	BYTE control  = m_keys[VK_CONTROL];
	if ((mask & KeyModifierModeSwitch) == 0) {
		m_keys[VK_LCONTROL] = 0;
		m_keys[VK_RCONTROL] = 0;
		m_keys[VK_CONTROL]  = 0;
	}

	// convert to ascii
	// FIXME -- support unicode
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
		return kKeyMultiKey;
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
		if (vkCode & 0x0100) {
			keys[VK_SHIFT]   = 0x80;
		}
		if (vkCode & 0x0200) {
			keys[VK_CONTROL] = 0x80;
		}
		if (vkCode & 0x0400) {
			keys[VK_MENU]    = 0x80;
		}

		// strip shift state off of virtual key code
		vkCode &= 0x00ff;

		// get the scan code for the key
		scanCode = MapVirtualKey(vkCode, 0);

		// put it back
		ToAscii(vkCode, scanCode, keys, &ascii, 0);
		return kKeyMultiKey;
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

	// we only care about the modifier key states.  other keys and the
	// mouse buttons should be up.
	m_keys[VK_LSHIFT]   = static_cast<BYTE>(GetKeyState(VK_LSHIFT) & 0x80);
	m_keys[VK_RSHIFT]   = static_cast<BYTE>(GetKeyState(VK_RSHIFT) & 0x80);
	m_keys[VK_SHIFT]    = static_cast<BYTE>(GetKeyState(VK_SHIFT) & 0x80);
	m_keys[VK_LCONTROL] = static_cast<BYTE>(GetKeyState(VK_LCONTROL) & 0x80);
	m_keys[VK_RCONTROL] = static_cast<BYTE>(GetKeyState(VK_RCONTROL) & 0x80);
	m_keys[VK_CONTROL]  = static_cast<BYTE>(GetKeyState(VK_CONTROL) & 0x80);
	m_keys[VK_LMENU]    = static_cast<BYTE>(GetKeyState(VK_LMENU) & 0x80);
	m_keys[VK_RMENU]    = static_cast<BYTE>(GetKeyState(VK_RMENU) & 0x80);
	m_keys[VK_MENU]     = static_cast<BYTE>(GetKeyState(VK_MENU) & 0x80);
	m_keys[VK_LWIN]     = static_cast<BYTE>(GetKeyState(VK_LWIN) & 0x80);
	m_keys[VK_RWIN]     = static_cast<BYTE>(GetKeyState(VK_RWIN) & 0x80);
	m_keys[VK_APPS]     = static_cast<BYTE>(GetKeyState(VK_APPS) & 0x80);
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

		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
			// toggle keys
			m_keys[vkCode]     |= 0x80;
			break;

		default:
		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
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

		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
			// toggle keys
			m_keys[vkCode]     &= ~0x80;
			m_keys[vkCode]     ^=  0x01;
			break;

		default:
		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
			m_keys[vkCode]     &= ~0x80;
			break;
		}
	}
}
