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

#include "CMSWindowsScreen.h"
#include "CMSWindowsClipboard.h"
#include "CMSWindowsDesktop.h"
#include "CMSWindowsScreenSaver.h"
#include "CClipboard.h"
#include "IScreenReceiver.h"
#include "IPrimaryScreenReceiver.h"
#include "XScreen.h"
#include "CThread.h"
#include "CLock.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "CString.h"
#include "CStringUtil.h"
#include "TMethodJob.h"
#include "CArch.h"
#include "CArchMiscWindows.h"
#include <cstring>
#include <malloc.h>
#include <tchar.h>

//
// add backwards compatible multihead support (and suppress bogus warning)
//
#pragma warning(push)
#pragma warning(disable: 4706) // assignment within conditional
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#pragma warning(pop)

// these are only defined when WINVER >= 0x0500
#if !defined(SPI_GETMOUSESPEED)
#define SPI_GETMOUSESPEED 112
#endif
#if !defined(SPI_SETMOUSESPEED)
#define SPI_SETMOUSESPEED 113
#endif

// X button stuff
#if !defined(WM_XBUTTONDOWN)
#define WM_XBUTTONDOWN		0x020B
#define WM_XBUTTONUP		0x020C
#define WM_XBUTTONDBLCLK	0x020D
#define WM_NCXBUTTONDOWN	0x00AB
#define WM_NCXBUTTONUP		0x00AC
#define WM_NCXBUTTONDBLCLK	0x00AD
#define MOUSEEVENTF_XDOWN	0x0100
#define MOUSEEVENTF_XUP		0x0200
#define XBUTTON1			0x0001
#define XBUTTON2			0x0002
#endif

// multimedia keys
#if !defined(VK_BROWSER_BACK)
#define VK_BROWSER_BACK			0xA6
#define VK_BROWSER_FORWARD		0xA7
#define VK_BROWSER_REFRESH		0xA8
#define VK_BROWSER_STOP			0xA9
#define VK_BROWSER_SEARCH		0xAA
#define VK_BROWSER_FAVORITES	0xAB
#define VK_BROWSER_HOME			0xAC
#define VK_VOLUME_MUTE			0xAD
#define VK_VOLUME_DOWN			0xAE
#define VK_VOLUME_UP			0xAF
#define VK_MEDIA_NEXT_TRACK		0xB0
#define VK_MEDIA_PREV_TRACK		0xB1
#define VK_MEDIA_STOP			0xB2
#define VK_MEDIA_PLAY_PAUSE		0xB3
#define VK_LAUNCH_MAIL			0xB4
#define VK_LAUNCH_MEDIA_SELECT	0xB5
#define VK_LAUNCH_APP1			0xB6
#define VK_LAUNCH_APP2			0xB7
#endif

//
// CMSWindowsScreen
//

HINSTANCE				CMSWindowsScreen::s_instance = NULL;
CMSWindowsScreen*		CMSWindowsScreen::s_screen = NULL;

CMSWindowsScreen::CMSWindowsScreen(
				IScreenReceiver* receiver,
				IPrimaryScreenReceiver* primaryReceiver) :
	m_isPrimary(primaryReceiver != NULL),
	m_is95Family(CArchMiscWindows::isWindows95Family()),
	m_receiver(receiver),
	m_primaryReceiver(primaryReceiver),
	m_isOnScreen(m_isPrimary),
	m_class(0),
	m_cursor(NULL),
	m_window(NULL),
	m_x(0), m_y(0),
	m_w(0), m_h(0),
	m_xCenter(0), m_yCenter(0),
	m_multimon(false),
	m_xCursor(0), m_yCursor(0),
	m_mark(0),
	m_markReceived(0),
	m_threadID(0),
	m_lastThreadID(0),
	m_timer(0),
	m_oneShotTimer(0),
	m_screensaver(NULL),
	m_screensaverNotify(false),
	m_nextClipboardWindow(NULL),
	m_ownClipboard(false),
	m_desk(NULL),
	m_deskName(),
	m_inaccessibleDesktop(false),
	m_hookLibrary(NULL),
	m_lowLevel(false),
	m_keyState(NULL)
{
	assert(s_screen   == NULL);
	assert(m_receiver != NULL);

	s_screen = this;

	// make sure this thread has a message queue
	MSG dummy;
	PeekMessage(&dummy, NULL, WM_USER, WM_USER, PM_NOREMOVE);
}

CMSWindowsScreen::~CMSWindowsScreen()
{
	assert(s_screen != NULL);
	assert(m_class  == 0);

	s_screen = NULL;
}

void
CMSWindowsScreen::init(HINSTANCE instance)
{
	s_instance = instance;
}

HINSTANCE
CMSWindowsScreen::getInstance()
{
	return s_instance;
}

void
CMSWindowsScreen::open(IKeyState* keyState)
{
	assert(s_instance    != NULL);
	assert(m_class       == 0);
	assert(m_hookLibrary == NULL);

	try {
		// load the hook library
		m_hookLibrary = LoadLibrary("synrgyhk");
		if (m_hookLibrary == NULL) {
			LOG((CLOG_ERR "Failed to load hook library;  synrgyhk.dll is missing"));
			throw XScreenOpenFailure();
		}
		m_setSides  = (SetSidesFunc)GetProcAddress(m_hookLibrary, "setSides");
		m_setZone   = (SetZoneFunc)GetProcAddress(m_hookLibrary, "setZone");
		m_setMode   = (SetModeFunc)GetProcAddress(m_hookLibrary, "setMode");
		m_install   = (InstallFunc)GetProcAddress(m_hookLibrary, "install");
		m_uninstall = (UninstallFunc)GetProcAddress(m_hookLibrary, "uninstall");
		m_init      = (InitFunc)GetProcAddress(m_hookLibrary, "init");
		m_cleanup   = (CleanupFunc)GetProcAddress(m_hookLibrary, "cleanup");
		m_installScreensaver   =
					  (InstallScreenSaverFunc)GetProcAddress(
									m_hookLibrary, "installScreenSaver");
		m_uninstallScreensaver =
					  (UninstallScreenSaverFunc)GetProcAddress(
									m_hookLibrary, "uninstallScreenSaver");
		if (m_setSides             == NULL ||
			m_setZone              == NULL ||
			m_setMode              == NULL ||
			m_install              == NULL ||
			m_uninstall            == NULL ||
			m_init                 == NULL ||
			m_cleanup              == NULL ||
			m_installScreensaver   == NULL ||
			m_uninstallScreensaver == NULL) {
			LOG((CLOG_ERR "Invalid hook library;  use a newer synrgyhk.dll"));
			throw XScreenOpenFailure();
		}

		// save thread id.  this is mainly to ensure that mainLoop()
		// is called by the same thread that called open().  these
		// threads must be the same to get the right message queue.
		m_threadID = GetCurrentThreadId();

		// initialize hook library
		if (m_isPrimary && m_init(m_threadID) == 0) {
			LOG((CLOG_ERR "Cannot initialize hook library;  is synergy already running?"));
			throw XScreenOpenFailure();
		}

		// create the transparent cursor
		m_cursor = createBlankCursor();

		// register a window class
		WNDCLASSEX classInfo;
		classInfo.cbSize        = sizeof(classInfo);
		classInfo.style         = CS_DBLCLKS | CS_NOCLOSE;
		classInfo.lpfnWndProc   = &CMSWindowsScreen::wndProc;
		classInfo.cbClsExtra    = 0;
		classInfo.cbWndExtra    = 0;
		classInfo.hInstance     = s_instance;
		classInfo.hIcon         = NULL;
		classInfo.hCursor       = m_cursor;
		classInfo.hbrBackground = NULL;
		classInfo.lpszMenuName  = NULL;
		classInfo.lpszClassName = "Synergy";
		classInfo.hIconSm       = NULL;
		m_class                 = RegisterClassEx(&classInfo);

		// get screen shape
		updateScreenShape();

		// initialize the screen saver
		m_screensaver = new CMSWindowsScreenSaver();

		// initialize marks
		m_mark         = 0;
		m_markReceived = 0;
	}
	catch (...) {
		close();
		throw;
	}

	// save the IKeyState
	m_keyState = keyState;
}

void
CMSWindowsScreen::close()
{
	assert(s_instance != NULL);

	// done with m_keyState
	m_keyState = NULL;

	// done with screen saver
	delete m_screensaver;

	// unregister the window class
	if (m_class != 0) {
		UnregisterClass((LPCTSTR)m_class, s_instance);
	}

	// done with cursor
	if (m_cursor != NULL) {
		DestroyCursor(m_cursor);
	}

	// done with hook library
	if (m_hookLibrary != NULL) {
		if (m_isPrimary) {
			m_cleanup();
		}
		FreeLibrary(m_hookLibrary);
	}

	// reset state
	m_screensaver  = NULL;
	m_cursor       = NULL;
	m_class        = 0;
	m_hookLibrary  = NULL;
	m_threadID     = 0;
}

void
CMSWindowsScreen::enable()
{
	assert(m_isOnScreen == m_isPrimary);

	if (m_isPrimary) {
		// update shadow key state
		m_keyMapper.update(NULL);

		// set jump zones
		m_setZone(m_x, m_y, m_w, m_h, getJumpZoneSize());

		// watch jump zones
		m_setMode(kHOOK_WATCH_JUMP_ZONE);
	}

	// create the window
	if (!switchDesktop(CMSWindowsDesktop::openInputDesktop())) {
		throw XScreenOpenFailure();
	}

	// poll input desktop to see if it changes.  windows doesn't
	// inform us when the desktop has changed but we need to
	// open a new window when that happens so we poll.  this is
	// also used for polling other stuff.
	m_timer = SetTimer(NULL, 0, 200, NULL);
}

void
CMSWindowsScreen::disable()
{
	// remove timers
	if (m_timer != 0) {
		KillTimer(NULL, m_timer);
	}
	if (m_oneShotTimer != 0) {
		KillTimer(NULL, m_oneShotTimer);
	}

	// reset state
	m_timer        = 0;
	m_oneShotTimer = 0;

	// done with window
	switchDesktop(NULL);
	assert(m_window == NULL);
	assert(m_desk   == NULL);
}

void
CMSWindowsScreen::mainLoop()
{
	// must call mainLoop() from same thread as openDesktop()
	assert(m_threadID == GetCurrentThreadId());

	// event loop
	MSG msg;
	for (;;) {
		// wait for an event in a cancellable way
		if (CThread::getCurrentThread().waitForEvent(-1.0) != CThread::kEvent) {
			continue;
		}
		if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			continue;
		}

		// handle quit message
		if (msg.message == WM_QUIT) {
			if (msg.wParam == 0) {
				// force termination
				CThread::getCurrentThread().cancel();
			}
			else {
				// just exit the main loop
				break;
			}
		}

		// dispatch message
		if (!onPreDispatch(msg.hwnd, msg.message, msg.wParam, msg.lParam)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void
CMSWindowsScreen::exitMainLoop()
{
	// close down cleanly
	PostThreadMessage(m_threadID, WM_QUIT, 1, 0);
}

void
CMSWindowsScreen::enter()
{
	if (m_isPrimary) {
		// show the cursor
		showCursor(true);
		m_cursorThread = 0;

		// enable special key sequences on win95 family
		enableSpecialKeys(true);

		// watch jump zones
		m_setMode(kHOOK_WATCH_JUMP_ZONE);

		// all messages prior to now are invalid
		nextMark();
	}
	else {
		// show the cursor
		ShowWindow(m_window, SW_HIDE);
	}

	// now on screen
	m_isOnScreen = true;
}

bool
CMSWindowsScreen::leave()
{
	if (m_isPrimary) {
		// show window
/* XXX
		if (m_lowLevel) {
			SetWindowPos(m_window, HWND_TOPMOST, m_xCenter, m_yCenter, 1, 1,
								SWP_NOACTIVATE);
			ShowWindow(m_window, SW_SHOWNA);
		}
*/
		// update keys
/* XXX
		m_keyMapper.update(NULL);
*/

		// warp to center
		warpCursor(m_xCenter, m_yCenter);

		// disable special key sequences on win95 family
		enableSpecialKeys(false);

		// all messages prior to now are invalid
		nextMark();

		// watch jump zones
		m_setMode(kHOOK_RELAY_EVENTS);

		// hide the cursor if using low level hooks
		if (m_lowLevel) {
			HWND hwnd      = GetForegroundWindow();
			m_cursorThread = GetWindowThreadProcessId(hwnd, NULL);
			showCursor(false);
		}
	}
	else {
		// move hider window under the cursor center
		MoveWindow(m_window, m_xCenter, m_yCenter, 1, 1, FALSE);

		// raise and show the hider window
		ShowWindow(m_window, SW_SHOWNA);

		// warp the mouse to the cursor center
		fakeMouseMove(m_xCenter, m_yCenter);
	}

	// now off screen
	m_isOnScreen = false;

	return true;
}

bool
CMSWindowsScreen::setClipboard(ClipboardID, const IClipboard* src)
{
	CMSWindowsClipboard dst(m_window);
	if (src != NULL) {
		// save clipboard data
		return CClipboard::copy(&dst, src);
	}
	else {
		// assert clipboard ownership
		if (!dst.open(0)) {
			return false;
		}
		dst.empty();
		dst.close();
		return true;
	}
}

void
CMSWindowsScreen::checkClipboards()
{
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
	if (m_ownClipboard && !CMSWindowsClipboard::isOwnedBySynergy()) {
		LOG((CLOG_DEBUG "clipboard changed: lost ownership and no notification received"));
		m_ownClipboard = false;
		m_receiver->onGrabClipboard(kClipboardClipboard);
		m_receiver->onGrabClipboard(kClipboardSelection);
	}
}

void
CMSWindowsScreen::openScreensaver(bool notify)
{
	assert(m_screensaver != NULL);

	m_screensaverNotify = notify;
	if (m_screensaverNotify) {
		m_installScreensaver();
	}
	else {
		m_screensaver->disable();
	}
}

void
CMSWindowsScreen::closeScreensaver()
{
	if (m_screensaver != NULL) {
		if (m_screensaverNotify) {
			m_uninstallScreensaver();
		}
		else {
			m_screensaver->enable();
		}
	}
	m_screensaverNotify = false;
}

void
CMSWindowsScreen::screensaver(bool activate)
{
	assert(m_screensaver != NULL);

	if (activate) {
		m_screensaver->activate();
	}
	else {
		m_screensaver->deactivate();
	}
}

void
CMSWindowsScreen::resetOptions()
{
	// no options
}

void
CMSWindowsScreen::setOptions(const COptionsList&)
{
	// no options
}

void
CMSWindowsScreen::updateKeys()
{
	syncDesktop();
	m_keyMapper.update(m_keyState);
	memset(m_buttons, 0, sizeof(m_buttons));
}

bool
CMSWindowsScreen::isPrimary() const
{
	return m_isPrimary;
}

bool
CMSWindowsScreen::getClipboard(ClipboardID, IClipboard* dst) const
{
	CMSWindowsClipboard src(m_window);
	CClipboard::copy(dst, &src);
	return true;
}

void
CMSWindowsScreen::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	assert(m_class != 0);

	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

void
CMSWindowsScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	POINT pos;
	syncDesktop();
	if (GetCursorPos(&pos)) {
		x = pos.x;
		y = pos.y;
	}
	else {
		x = m_xCenter;
		y = m_yCenter;
	}
}

void
CMSWindowsScreen::reconfigure(UInt32 activeSides)
{
	assert(m_isPrimary);

	m_setSides(activeSides);
}

void
CMSWindowsScreen::warpCursor(SInt32 x, SInt32 y)
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
	m_xCursor = x;
	m_yCursor = y;
}

UInt32
CMSWindowsScreen::addOneShotTimer(double timeout)
{
	// FIXME -- support multiple one-shot timers
	if (m_oneShotTimer != 0) {
		KillTimer(NULL, m_oneShotTimer);
	}
	m_oneShotTimer = SetTimer(NULL, 0,
						static_cast<UINT>(1000.0 * timeout), NULL);
	return 0;
}

SInt32
CMSWindowsScreen::getJumpZoneSize() const
{
	return 1;
}

bool
CMSWindowsScreen::isAnyMouseButtonDown() const
{
	static const char* buttonToName[] = {
		"button 0",
		"Left Button",
		"Middle Button",
		"Right Button",
		"X Button 1",
		"X Button 2"
	};

	for (UInt32 i = 0; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
		if ((m_buttons[i] & 0x80) != 0) {
			LOG((CLOG_DEBUG "locked by \"%s\"", buttonToName[i]));
			return true;
		}
	}

	return false;
}

const char*
CMSWindowsScreen::getKeyName(KeyButton virtualKey) const
{
	return m_keyMapper.getKeyName(virtualKey);
}

void
CMSWindowsScreen::fakeKeyEvent(KeyButton virtualKey, bool press) const
{
	DWORD flags = 0;
	if (m_keyMapper.isExtendedKey(virtualKey)) {
		flags |= KEYEVENTF_EXTENDEDKEY;
	}
	if (!press) {
		flags |= KEYEVENTF_KEYUP;
	}
	const UINT code = m_keyMapper.keyToScanCode(&virtualKey);
	syncDesktop();
	keybd_event(static_cast<BYTE>(virtualKey & 0xffu),
								static_cast<BYTE>(code), flags, 0);
}

bool
CMSWindowsScreen::fakeCtrlAltDel() const
{
	if (!m_is95Family) {
		// to fake ctrl+alt+del on the NT family we broadcast a suitable
		// hotkey to all windows on the winlogon desktop.  however, we
		// the current thread must be on that desktop to do the broadcast
		// and we can't switch just any thread because some own windows
		// or hooks.  so start a new thread to do the real work.
		CThread cad(new CFunctionJob(&CMSWindowsScreen::ctrlAltDelThread));
		cad.wait();
	}
	else {
		// get the sequence of keys to simulate ctrl+alt+del
		IKeyState::Keystrokes keys;
		KeyID key            = kKeyDelete;
		KeyModifierMask mask = KeyModifierControl | KeyModifierAlt;
		if (mapKey(keys, *m_keyState, key, mask, false) == 0) {
			keys.clear();
		}

		// do it
		for (IKeyState::Keystrokes::const_iterator k = keys.begin();
								k != keys.end(); ++k) {
			fakeKeyEvent(k->m_key, k->m_press);
		}
	}
	return true;
}

void
CMSWindowsScreen::fakeMouseButton(ButtonID id, bool press) const
{
	// map button id to button flag
	DWORD data;
	DWORD flags = mapButtonToEvent(id, press, &data);

	// send event
	if (flags != 0) {
		syncDesktop();
		mouse_event(flags, 0, 0, data, 0);
	}
}

void
CMSWindowsScreen::fakeMouseMove(SInt32 x, SInt32 y) const
{
	// motion is simple (i.e. it's on the primary monitor) if there
	// is only one monitor.
	bool simple = !m_multimon;
	if (!simple) {
		// also simple if motion is within the primary monitor
		simple = (x >= 0 && x < GetSystemMetrics(SM_CXSCREEN) &&
				  y >= 0 && y < GetSystemMetrics(SM_CYSCREEN));
	}

	// move the mouse directly to target position if motion is simple
	syncDesktop();
	if (simple) {
		// when using absolute positioning with mouse_event(),
		// the normalized device coordinates range over only
		// the primary screen.
		SInt32 w = GetSystemMetrics(SM_CXSCREEN);
		SInt32 h = GetSystemMetrics(SM_CYSCREEN);
		mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
								(DWORD)((65536.0 * x) / w),
								(DWORD)((65536.0 * y) / h),
								0, 0);
	}

	// windows 98 (and Me?) is broken.  you cannot set the absolute
	// position of the mouse except on the primary monitor but you
	// can do relative moves onto any monitor.  this is, in microsoft's
	// words, "by design."  apparently the designers of windows 2000
	// we're a little less lazy and did it right.
	//
	// microsoft recommends in Q193003 to absolute position the cursor
	// somewhere on the primary monitor then relative move to the
	// desired location.  this doesn't work for us because when the
	// user drags a scrollbar, a window, etc. it causes the dragged
	// item to jump back a forth between the position on the primary
	// monitor and the desired position.  while it always ends up in
	// the right place, the effect is disconcerting.
	//
	// instead we'll get the cursor's current position and do just a
	// relative move from there to the desired position.  relative
	// moves are subject to cursor acceleration which we don't want.
	// so we disable acceleration, do the relative move, then restore
	// acceleration.  there's a slight chance we'll end up in the
	// wrong place if the user moves the cursor using this system's
	// mouse while simultaneously moving the mouse on the server
	// system.  that defeats the purpose of synergy so we'll assume
	// that won't happen.  even if it does, the next mouse move will
	// correct the position.
	else {
		// save mouse speed & acceleration
		int oldSpeed[4];
		bool accelChanged =
					SystemParametersInfo(SPI_GETMOUSE,0, oldSpeed, 0) &&
					SystemParametersInfo(SPI_GETMOUSESPEED, 0, oldSpeed + 3, 0);

		// use 1:1 motion
		if (accelChanged) {
			int newSpeed[4] = { 0, 0, 0, 1 };
			accelChanged =
					SystemParametersInfo(SPI_SETMOUSE, 0, newSpeed, 0) ||
					SystemParametersInfo(SPI_SETMOUSESPEED, 0, newSpeed + 3, 0);
		}

		// get current mouse position
		POINT pos;
		GetCursorPos(&pos);

		// move relative to mouse position
		mouse_event(MOUSEEVENTF_MOVE, x - pos.x, y - pos.y, 0, 0);

		// restore mouse speed & acceleration
		if (accelChanged) {
			SystemParametersInfo(SPI_SETMOUSE, 0, oldSpeed, 0);
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, oldSpeed + 3, 0);
		}
	}
}

void
CMSWindowsScreen::fakeMouseWheel(SInt32 delta) const
{
	syncDesktop();
	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0);
}

KeyButton
CMSWindowsScreen::mapKey(IKeyState::Keystrokes& keys,
						const IKeyState& keyState, KeyID id,
						KeyModifierMask desiredMask,
						bool isAutoRepeat) const
{
	return m_keyMapper.mapKey(keys, keyState, id, desiredMask, isAutoRepeat);
}

void
CMSWindowsScreen::updateScreenShape()
{
	// get shape
	m_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	m_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d", m_x, m_y, m_w, m_h));

	// get center for cursor
	m_xCenter = GetSystemMetrics(SM_CXSCREEN) >> 1;
	m_yCenter = GetSystemMetrics(SM_CYSCREEN) >> 1;

	// check for multiple monitors
	m_multimon = (m_w != GetSystemMetrics(SM_CXSCREEN) ||
				  m_h != GetSystemMetrics(SM_CYSCREEN));
}

bool
CMSWindowsScreen::switchDesktop(HDESK desk)
{
	// assume we don't own the clipboard until later
	m_ownClipboard = false;

	// destroy old window
	if (m_window != NULL) {
		// first remove clipboard snooper
		ChangeClipboardChain(m_window, m_nextClipboardWindow);
		m_nextClipboardWindow = NULL;

		// uninstall hooks.  we can't change the thread desktop
		// with hooks installed.
		if (m_isPrimary) {
			m_uninstall();
		}

		// now destroy window.  we can't change the thread desktop
		// with a window.
		DestroyWindow(m_window);
		m_window = NULL;

		// done with desk
		CMSWindowsDesktop::closeDesktop(m_desk);
		m_desk     = NULL;
		m_deskName = "";
	}

	// if no new desktop then we're done
	if (desk == NULL) {
		LOG((CLOG_DEBUG "disconnecting desktop"));
		return true;
	}

	// uninstall screen saver hooks
	if (m_screensaverNotify) {
		m_uninstallScreensaver();
	}

	// set the desktop.  can only do this when there are no windows
	// and hooks on the current desktop owned by this thread.
	if (!CMSWindowsDesktop::setDesktop(desk)) {
		LOG((CLOG_ERR "failed to set desktop: %d", GetLastError()));
		CMSWindowsDesktop::closeDesktop(desk);
		return false;
	}

	// create the window
	m_window = CreateWindowEx(WS_EX_TOPMOST |
									WS_EX_TRANSPARENT |
									WS_EX_TOOLWINDOW,
								(LPCTSTR)m_class,
								"Synergy",
								WS_POPUP,
								0, 0, 1, 1,
								NULL, NULL,
								getInstance(),
								NULL);
	if (m_window == NULL) {
		LOG((CLOG_ERR "failed to create window: %d", GetLastError()));
		CMSWindowsDesktop::closeDesktop(desk);
		return false;
	}

	// reinstall screen saver hooks
	if (m_screensaverNotify) {
		m_installScreensaver();
	}

	if (m_isPrimary) {
		// we don't ever want our window to activate
		EnableWindow(m_window, FALSE);

		// install hooks
		switch (m_install()) {
		case kHOOK_FAILED:
			// FIXME -- can't install hook so we won't work;  report error
			m_lowLevel = false;
			break;

		case kHOOK_OKAY:
			m_lowLevel = false;
			break;

		case kHOOK_OKAY_LL:
			m_lowLevel = true;
			break;
		}

		if (m_isOnScreen) {
			// all messages prior to now are invalid
			// FIXME -- is this necessary;  couldn't we lose key releases?
			nextMark();
		}
	}
	else {
		// update key state
		updateKeys();

		// hide cursor if this screen isn't active
		if (!m_isOnScreen) {
			// move hider window under the cursor center
			MoveWindow(m_window, m_xCenter, m_yCenter, 1, 1, FALSE);

			// raise and show the hider window
			ShowWindow(m_window, SW_SHOWNA);

			// warp the mouse to the cursor center
			fakeMouseMove(m_xCenter, m_yCenter);
		}
	}

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	// check if we own the clipboard
	m_ownClipboard = CMSWindowsClipboard::isOwnedBySynergy();

	// save new desktop
	m_desk     = desk;
	m_deskName = CMSWindowsDesktop::getDesktopName(desk);
	LOG((CLOG_DEBUG "switched to desktop \"%s\" with window 0x%08x", m_deskName.c_str(), (UInt32)m_window));

	return true;
}

void
CMSWindowsScreen::syncDesktop() const
{
	// change calling thread's desktop
	if (!CMSWindowsDesktop::setDesktop(m_desk)) {
//		LOG((CLOG_WARN "failed to set desktop: %d", GetLastError()));
	}

	// attach input queues if not already attached.  this has a habit
	// of sucking up more and more CPU each time it's called (even if
	// the threads are already attached).  since we only expect one
	// thread to call this more than once we can save just the last
	// attached thread.
	DWORD threadID = GetCurrentThreadId();
	if (threadID != m_lastThreadID && threadID != m_threadID) {
		CMSWindowsScreen* self = const_cast<CMSWindowsScreen*>(this);
		self->m_lastThreadID = threadID;
		AttachThreadInput(threadID, m_threadID, TRUE);
	}
}

bool
CMSWindowsScreen::onPreDispatch(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam)
{
	// handle event
	switch (message) {
	case SYNERGY_MSG_SCREEN_SAVER:
		return onScreensaver(wParam != 0);

	case WM_TIMER:
		return onTimer(static_cast<UINT>(wParam));
	}

	if (m_isPrimary) {
		return onPreDispatchPrimary(hwnd, message, wParam, lParam);
	}

	return false;
}

bool
CMSWindowsScreen::onPreDispatchPrimary(HWND,
				UINT message, WPARAM wParam, LPARAM lParam)
{
	// check if windows key is up but we think it's down.  if so then
	// synthesize a key release for it.  we have to do this because
	// if the user presses and releases a windows key without pressing
	// any other key while it's down then windows will eat the key
	// release.  if we don't detect that and synthesize the release
	// then the user will be locked to the screen and the client won't
	// take the usual windows key release action (which on windows is
	// to show the start menu).
	//
	// we can use GetKeyState() to check the state of the windows keys
	// because, event though the key release is not reported to us,
	// the event is processed and the keyboard state updated by the
	// system.  since the key could go up at any time we'll check the
	// state on every event.  only check on windows 95 family since
	// NT family reports the key release as usual.  obviously we skip
	// this if the event is for the windows key itself.
	if (m_is95Family) {
		if (m_keyMapper.isPressed(VK_LWIN) &&
			(GetAsyncKeyState(VK_LWIN) & 0x8000) == 0 &&
			!(message == SYNERGY_MSG_KEY && wParam == VK_LWIN)) {
			// compute appropriate parameters for fake event
			WPARAM wParam = VK_LWIN;
			LPARAM lParam = 0xc1000000;
			lParam |= (0x00ff0000 & (MapVirtualKey(wParam, 0) << 24));

			// process as if it were a key up
			KeyModifierMask mask;
			KeyButton button = static_cast<KeyButton>(
								(lParam & 0x00ff0000u) >> 16);
			KeyID key = m_keyMapper.mapKeyFromEvent(wParam,
													lParam, &mask, NULL);
			LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
			m_primaryReceiver->onKeyUp(key, mask, button);
			m_keyMapper.updateKey(static_cast<KeyButton>(wParam), false);
		}
		if (m_keyMapper.isPressed(VK_RWIN) &&
			(GetAsyncKeyState(VK_RWIN) & 0x8000) == 0 &&
			!(message == SYNERGY_MSG_KEY && wParam == VK_RWIN)) {
			// compute appropriate parameters for fake event
			WPARAM wParam = VK_RWIN;
			LPARAM lParam = 0xc1000000;
			lParam |= (0x00ff0000 & (MapVirtualKey(wParam, 0) << 24));

			// process as if it were a key up
			KeyModifierMask mask;
			KeyButton button = static_cast<KeyButton>(
								(lParam & 0x00ff0000u) >> 16);
			KeyID key = m_keyMapper.mapKeyFromEvent(wParam,
													lParam, &mask, NULL);
			LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
			m_primaryReceiver->onKeyUp(key, mask, button);
			m_keyMapper.updateKey(static_cast<KeyButton>(wParam), false);
		}
	}

	// handle event
	switch (message) {
	case SYNERGY_MSG_MARK:
		return onMark(static_cast<UInt32>(wParam));

	case SYNERGY_MSG_KEY:
		return onKey(wParam, lParam);

	case SYNERGY_MSG_MOUSE_BUTTON:
		return onMouseButton(wParam, lParam);

	case SYNERGY_MSG_MOUSE_MOVE:
		return onMouseMove(static_cast<SInt32>(wParam),
							static_cast<SInt32>(lParam));

	case SYNERGY_MSG_MOUSE_WHEEL:
		return onMouseWheel(static_cast<SInt32>(wParam));

	case SYNERGY_MSG_PRE_WARP:
		{
			// save position to compute delta of next motion
			m_xCursor = static_cast<SInt32>(wParam);
			m_yCursor = static_cast<SInt32>(lParam);

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
		}
		return true;

	case SYNERGY_MSG_POST_WARP:
		LOG((CLOG_WARN "unmatched post warp"));
		return true;
	}

	return false;
}

bool
CMSWindowsScreen::onEvent(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
	switch (message) {
	case WM_QUERYENDSESSION:
		if (m_is95Family) {
			*result = TRUE;
			return true;
		}
		break;

	case WM_ENDSESSION:
		if (m_is95Family) {
			if (wParam == TRUE && lParam == 0) {
				exitMainLoop();
			}
			return true;
		}
		break;

	case WM_PAINT:
		ValidateRect(hwnd, NULL);
		return true;

	case WM_DRAWCLIPBOARD:
		LOG((CLOG_DEBUG "clipboard was taken"));

		// first pass on the message
		if (m_nextClipboardWindow != NULL) {
			SendMessage(m_nextClipboardWindow, message, wParam, lParam);
		}

		// now handle the message
		return onClipboardChange();

	case WM_CHANGECBCHAIN:
		if (m_nextClipboardWindow == (HWND)wParam) {
			m_nextClipboardWindow = (HWND)lParam;
			LOG((CLOG_DEBUG "clipboard chain: new next: 0x%08x", m_nextClipboardWindow));
		}
		else if (m_nextClipboardWindow != NULL) {
			LOG((CLOG_DEBUG "clipboard chain: forward: %d 0x%08x 0x%08x", message, wParam, lParam));
			SendMessage(m_nextClipboardWindow, message, wParam, lParam);
		}
		return true;

	case WM_DISPLAYCHANGE:
		return onDisplayChange();

	case WM_ACTIVATEAPP:
		return onActivate(wParam != FALSE);
	}

	return false;
}

bool
CMSWindowsScreen::onMark(UInt32 mark)
{
	m_markReceived = mark;
	return true;
}

bool
CMSWindowsScreen::onKey(WPARAM wParam, LPARAM lParam)
{
	// ignore message if posted prior to last mark change
	if (!ignore()) {
		// check for ctrl+alt+del emulation
		if ((wParam == VK_PAUSE || wParam == VK_CANCEL) &&
			(m_keyMapper.isPressed(VK_CONTROL) &&
			m_keyMapper.isPressed(VK_MENU))) {
			LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
			wParam  = VK_DELETE;
			lParam &= 0xffff0000;
			lParam |= 0x00000001;
		}

		// process key normally
		bool altgr;
		KeyModifierMask mask;
		const KeyID key = m_keyMapper.mapKeyFromEvent(wParam,
													lParam, &mask, &altgr);
		KeyButton button = static_cast<KeyButton>(
							(lParam & 0x00ff0000u) >> 16);
		if (key != kKeyNone && key != kKeyMultiKey) {
			if ((lParam & 0x80000000) == 0) {
				// key press

				// if AltGr required for this key then make sure
				// the ctrl and alt keys are *not* down on the
				// client.  windows simulates AltGr with ctrl and
				// alt for some inexplicable reason and clients
				// will get confused if they see mode switch and
				// ctrl and alt.  we'll also need to put ctrl and
				// alt back the way they were after we simulate
				// the key.
				bool ctrlL = m_keyMapper.isPressed(VK_LCONTROL);
				bool ctrlR = m_keyMapper.isPressed(VK_RCONTROL);
				bool altL  = m_keyMapper.isPressed(VK_LMENU);
				bool altR  = m_keyMapper.isPressed(VK_RMENU);
				if (altgr) {
					KeyID key;
					KeyButton button;
					UINT scanCode;
					KeyModifierMask mask2 = (mask &
										~(KeyModifierControl |
										KeyModifierAlt |
										KeyModifierModeSwitch));
					if (ctrlL) {
						key      = kKeyControl_L;
						button   = VK_LCONTROL;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyUp(key, mask2, button);
					}
					if (ctrlR) {
						key      = kKeyControl_R;
						button   = VK_RCONTROL;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyUp(key, mask2, button);
					}
					if (altL) {
						key      = kKeyAlt_L;
						button   = VK_LMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyUp(key, mask2, button);
					}
					if (altR) {
						key      = kKeyAlt_R;
						button   = VK_RMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyUp(key, mask2, button);
					}
				}

				// send key
				const bool wasDown = ((lParam & 0x40000000) != 0);
				SInt32 repeat      = (SInt32)(lParam & 0xffff);
				if (!wasDown) {
					LOG((CLOG_DEBUG1 "event: key press key=%d mask=0x%04x button=0x%04x", key, mask, button));
					m_primaryReceiver->onKeyDown(key, mask, button);
					if (repeat > 0) {
						--repeat;
					}
				}
				if (repeat >= 1) {
					LOG((CLOG_DEBUG1 "event: key repeat key=%d mask=0x%04x count=%d button=0x%04x", key, mask, repeat, button));
					m_primaryReceiver->onKeyRepeat(key, mask, repeat, button);
				}

				// restore ctrl and alt state
				if (altgr) {
					KeyID key;
					KeyButton button;
					UINT scanCode;
					KeyModifierMask mask2 = (mask &
										~(KeyModifierControl |
										KeyModifierAlt |
										KeyModifierModeSwitch));
					if (ctrlL) {
						key    = kKeyControl_L;
						button = VK_LCONTROL;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyDown(key, mask2, button);
						mask2 |= KeyModifierControl;
					}
					if (ctrlR) {
						key    = kKeyControl_R;
						button = VK_RCONTROL;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyDown(key, mask2, button);
						mask2 |= KeyModifierControl;
					}
					if (altL) {
						key    = kKeyAlt_L;
						button = VK_LMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyDown(key, mask2, button);
						mask2 |= KeyModifierAlt;
					}
					if (altR) {
						key    = kKeyAlt_R;
						button = VK_RMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						m_primaryReceiver->onKeyDown(key, mask2, button);
						mask2 |= KeyModifierAlt;
					}
				}
			}
			else {
				// key release.  if the key isn't down according to
				// our table then we never got the key press event
				// for it.  if it's not a modifier key then we'll
				// synthesize the press first.  only do this on
				// the windows 95 family, which eats certain special
				// keys like alt+tab, ctrl+esc, etc.
				if (m_is95Family && !isModifier(wParam) &&
					m_keyMapper.isPressed(static_cast<KeyButton>(wParam))) {
					LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask, button));
					m_primaryReceiver->onKeyDown(key, mask, button);
					m_keyMapper.updateKey(static_cast<KeyButton>(wParam), true);
				}

				// do key up
				LOG((CLOG_DEBUG1 "event: key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
				m_primaryReceiver->onKeyUp(key, mask, button);
			}
		}
		else {
			LOG((CLOG_DEBUG2 "event: cannot map key wParam=%d lParam=0x%08x", wParam, lParam));
		}
	}

	// keep our shadow key state up to date
	m_keyMapper.updateKey(static_cast<KeyButton>(wParam),
							((lParam & 0x80000000) == 0));

	return true;
}

bool
CMSWindowsScreen::onMouseButton(WPARAM wParam, LPARAM lParam)
{
	// get which button
	bool pressed = false;
	const ButtonID button = mapButtonFromEvent(wParam, lParam);

	// ignore message if posted prior to last mark change
	if (!ignore()) {
		switch (wParam) {
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		case WM_NCLBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCXBUTTONDOWN:
		case WM_NCLBUTTONDBLCLK:
		case WM_NCMBUTTONDBLCLK:
		case WM_NCRBUTTONDBLCLK:
		case WM_NCXBUTTONDBLCLK:
			LOG((CLOG_DEBUG1 "event: button press button=%d", button));
			if (button != kButtonNone) {
				m_primaryReceiver->onMouseDown(button);
			}
			pressed = true;
			break;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_XBUTTONUP:
		case WM_NCLBUTTONUP:
		case WM_NCMBUTTONUP:
		case WM_NCRBUTTONUP:
		case WM_NCXBUTTONUP:
			LOG((CLOG_DEBUG1 "event: button release button=%d", button));
			if (button != kButtonNone) {
				m_primaryReceiver->onMouseUp(button);
			}
			pressed = false;
			break;
		}
	}

	// keep our shadow key state up to date
	if (button >= kButtonLeft && button <= kButtonExtra0 + 1) {
		if (pressed) {
			m_buttons[button] |= 0x80;
		}
		else {
			m_buttons[button] &= ~0x80;
		}
	}

	return true;
}

bool
CMSWindowsScreen::onMouseMove(SInt32 mx, SInt32 my)
{
	// compute motion delta (relative to the last known
	// mouse position)
	SInt32 x = mx - m_xCursor;
	SInt32 y = my - m_yCursor;

	// ignore if the mouse didn't move or if message posted prior
	// to last mark change.
	if (ignore() || (x == 0 && y == 0)) {
		return true;
	}

	// save position to compute delta of next motion
	m_xCursor = mx;
	m_yCursor = my;

	if (m_isOnScreen) {
		// motion on primary screen
		m_primaryReceiver->onMouseMovePrimary(m_xCursor, m_yCursor);
	}
	else {
		// motion on secondary screen.  warp mouse back to
		// center.
		warpCursorNoFlush(m_xCenter, m_yCenter);

		// examine the motion.  if it's about the distance
		// from the center of the screen to an edge then
		// it's probably a bogus motion that we want to
		// ignore (see warpCursorNoFlush() for a further
		// description).
		static SInt32 bogusZoneSize = 10;
		if (-x + bogusZoneSize > m_xCenter - m_x ||
			 x + bogusZoneSize > m_x + m_w - m_xCenter ||
			-y + bogusZoneSize > m_yCenter - m_y ||
			 y + bogusZoneSize > m_y + m_h - m_yCenter) {
			LOG((CLOG_DEBUG "dropped bogus motion %+d,%+d", x, y));
		}
		else {
			// send motion
			m_primaryReceiver->onMouseMoveSecondary(x, y);
		}
	}

	return true;
}

bool
CMSWindowsScreen::onMouseWheel(SInt32 delta)
{
	// ignore message if posted prior to last mark change
	if (!ignore()) {
		LOG((CLOG_DEBUG1 "event: button wheel delta=%d", delta));
		m_primaryReceiver->onMouseWheel(delta);
	}
	return true;
}

bool
CMSWindowsScreen::onScreensaver(bool activated)
{
	// ignore this message if there are any other screen saver
	// messages already in the queue.  this is important because
	// our checkStarted() function has a deliberate delay, so it
	// can't respond to events at full CPU speed and will fall
	// behind if a lot of screen saver events are generated.
	// that can easily happen because windows will continually
	// send SC_SCREENSAVE until the screen saver starts, even if
	// the screen saver is disabled!
	MSG msg;
	if (PeekMessage(&msg, NULL, SYNERGY_MSG_SCREEN_SAVER,
						SYNERGY_MSG_SCREEN_SAVER, PM_NOREMOVE)) {
		return true;
	}

	if (activated) {
		if (m_screensaver->checkStarted(SYNERGY_MSG_SCREEN_SAVER, FALSE, 0)) {
			m_primaryReceiver->onScreensaver(true);
		}
	}
	else {
		m_primaryReceiver->onScreensaver(false);
	}

	return true;
}

bool
CMSWindowsScreen::onTimer(UINT timerID)
{
	if (timerID == m_timer) {
		// if current desktop is not the input desktop then switch to it.
		HDESK desk = CMSWindowsDesktop::openInputDesktop();
		if (desk == m_desk ||
			CMSWindowsDesktop::getDesktopName(desk) == m_deskName ||
			m_screensaver->isActive()) {
			// same desktop or screensaver is active.  don't switch
			// desktops when the screensaver is active.  we'd most
			// likely switch to the screensaver desktop which would
			// have the side effect of forcing the screensaver to stop.
			CMSWindowsDesktop::closeDesktop(desk);
		}
		else {
			switchDesktop(desk);
		}

		// if the desktop was inaccessible and isn't anymore then
		// update our key state.
		if (desk != NULL && m_inaccessibleDesktop) {
			LOG((CLOG_DEBUG "desktop is now accessible"));
			m_inaccessibleDesktop = false;
			updateKeys();
		}

		// note if desktop was accessible but isn't anymore
		else if (desk == NULL && !m_inaccessibleDesktop) {
			m_inaccessibleDesktop = true;
			LOG((CLOG_DEBUG "desktop is now inaccessible"));
		}
	}

	else if (timerID == m_oneShotTimer) {
		// one shot timer expired
		KillTimer(NULL, m_oneShotTimer);
		m_oneShotTimer = 0;
		m_primaryReceiver->onOneShotTimerExpired(0);
	}

	return true;
}

bool
CMSWindowsScreen::onDisplayChange()
{
	// screen resolution may have changed.  save old shape.
	SInt32 xOld = m_x, yOld = m_y, wOld = m_w, hOld = m_h;

	// update shape
	updateScreenShape();

	// do nothing if resolution hasn't changed
	if (xOld != m_x || yOld != m_y || wOld != m_w || hOld != m_h) {
		if (m_isPrimary) {
			// warp mouse to center if off screen
			if (!m_isOnScreen) {
				warpCursor(m_xCenter, m_yCenter);
			}

			// tell hook about resize if on screen
			else {
				m_setZone(m_x, m_y, m_w, m_h, getJumpZoneSize());
			}
		}

		// collect new screen info
		CClientInfo info;
		info.m_x        = m_x;
		info.m_y        = m_y;
		info.m_w        = m_w;
		info.m_h        = m_h;
		info.m_zoneSize = getJumpZoneSize();
		getCursorPos(info.m_mx, info.m_my);

		// send new screen info
		m_receiver->onInfoChanged(info);
	}

	return true;
}

bool
CMSWindowsScreen::onClipboardChange()
{
	// now notify client that somebody changed the clipboard (unless
	// we're the owner).
	if (!CMSWindowsClipboard::isOwnedBySynergy()) {
		LOG((CLOG_DEBUG "clipboard changed: foreign owned"));
		if (m_ownClipboard) {
			LOG((CLOG_DEBUG "clipboard changed: lost ownership"));
			m_ownClipboard = false;
			m_receiver->onGrabClipboard(kClipboardClipboard);
			m_receiver->onGrabClipboard(kClipboardSelection);
		}
	}
	else {
		LOG((CLOG_DEBUG "clipboard changed: synergy owned"));
		m_ownClipboard = true;
	}

	return true;
}

bool
CMSWindowsScreen::onActivate(bool activated)
{
	if (!m_isPrimary && activated) {
		// some other app activated.  hide the hider window.
		ShowWindow(m_window, SW_HIDE);
	}

	return false;
}

void
CMSWindowsScreen::warpCursorNoFlush(SInt32 x, SInt32 y)
{
	// send an event that we can recognize before the mouse warp
	PostThreadMessage(m_threadID, SYNERGY_MSG_PRE_WARP, x, y);

	// warp mouse.  hopefully this inserts a mouse motion event
	// between the previous message and the following message.
	SetCursorPos(x, y);

	// yield the CPU.  there's a race condition when warping:
	//   a hardware mouse event occurs
	//   the mouse hook is not called because that process doesn't have the CPU
	//   we send PRE_WARP, SetCursorPos(), send POST_WARP
	//   we process all of those events and update m_x, m_y
	//   we finish our time slice
	//   the hook is called
	//   the hook sends us a mouse event from the pre-warp position
	//   we get the CPU
	//   we compute a bogus warp
	// we need the hook to process all mouse events that occur
	// before we warp before we do the warp but i'm not sure how
	// to guarantee that.  yielding the CPU here may reduce the
	// chance of undesired behavior.  we'll also check for very
	// large motions that look suspiciously like about half width
	// or height of the screen.
	ARCH->sleep(0.0);

	// send an event that we can recognize after the mouse warp
	PostThreadMessage(m_threadID, SYNERGY_MSG_POST_WARP, 0, 0);
}

void
CMSWindowsScreen::nextMark()
{
	// next mark
	++m_mark;

	// mark point in message queue where the mark was changed
	PostThreadMessage(m_threadID, SYNERGY_MSG_MARK, m_mark, 0);
}

bool
CMSWindowsScreen::ignore() const
{
	return (m_mark != m_markReceived);
}

HCURSOR
CMSWindowsScreen::createBlankCursor() const
{
	// create a transparent cursor
	int cw = GetSystemMetrics(SM_CXCURSOR);
	int ch = GetSystemMetrics(SM_CYCURSOR);
	UInt8* cursorAND = new UInt8[ch * ((cw + 31) >> 2)];
	UInt8* cursorXOR = new UInt8[ch * ((cw + 31) >> 2)];
	memset(cursorAND, 0xff, ch * ((cw + 31) >> 2));
	memset(cursorXOR, 0x00, ch * ((cw + 31) >> 2));
	HCURSOR c = CreateCursor(s_instance, 0, 0, cw, ch, cursorAND, cursorXOR);
	delete[] cursorXOR;
	delete[] cursorAND;
	return c;
}

void
CMSWindowsScreen::showCursor(bool show) const
{
	if (m_cursorThread != 0) {
		if (m_threadID != m_cursorThread) {
			AttachThreadInput(m_threadID, m_cursorThread, TRUE);
		}
		ShowCursor(show ? TRUE : FALSE);
		if (m_threadID != m_cursorThread) {
			AttachThreadInput(m_threadID, m_cursorThread, FALSE);
		}
	}
}

void
CMSWindowsScreen::enableSpecialKeys(bool enable) const
{
	// enable/disable ctrl+alt+del, alt+tab, etc on win95 family.
	// since the win95 family doesn't support low-level hooks, we
	// use this undocumented feature to suppress normal handling
	// of certain key combinations.
	if (m_is95Family) {
		DWORD dummy = 0;
		SystemParametersInfo(SPI_SETSCREENSAVERRUNNING,
							enable ? FALSE : TRUE, &dummy, 0);
	}
}

DWORD
CMSWindowsScreen::mapButtonToEvent(ButtonID button,
				bool press, DWORD* inData) const
{
	DWORD dummy;
	DWORD* data = (inData != NULL) ? inData : &dummy;

	// the system will swap the meaning of left/right for us if
	// the user has configured a left-handed mouse but we don't
	// want it to swap since we want the handedness of the
	// server's mouse.  so pre-swap for a left-handed mouse.
	if (GetSystemMetrics(SM_SWAPBUTTON)) {
		switch (button) {
		case kButtonLeft:
			button = kButtonRight;
			break;

		case kButtonRight:
			button = kButtonLeft;
			break;
		}
	}

	// map button id to button flag and button data
	*data = 0;
	switch (button) {
	case kButtonLeft:
		return press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;

	case kButtonMiddle:
		return press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;

	case kButtonRight:
		return press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;

	case kButtonExtra0 + 0:
		*data = XBUTTON1;
		return press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;

	case kButtonExtra0 + 1:
		*data = XBUTTON2;
		return press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;

	default:
		return 0;
	}
}

ButtonID
CMSWindowsScreen::mapButtonFromEvent(WPARAM msg, LPARAM button) const
{
	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCLBUTTONUP:
		return kButtonLeft;

	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONUP:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCMBUTTONUP:
		return kButtonMiddle;

	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONUP:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONUP:
		return kButtonRight;

	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
	case WM_XBUTTONUP:
	case WM_NCXBUTTONDOWN:
	case WM_NCXBUTTONDBLCLK:
	case WM_NCXBUTTONUP:
		switch (button) {
		case XBUTTON1:
			return kButtonExtra0 + 0;

		case XBUTTON2:
			return kButtonExtra0 + 1;
		}
		return kButtonNone;

	default:
		return kButtonNone;
	}
}

bool
CMSWindowsScreen::isModifier(UINT vkCode) const
{
	switch (vkCode) {
	case VK_LSHIFT:
	case VK_RSHIFT:
	case VK_SHIFT:
	case VK_LCONTROL:
	case VK_RCONTROL:
	case VK_CONTROL:
	case VK_LMENU:
	case VK_RMENU:
	case VK_MENU:
	case VK_CAPITAL:
	case VK_NUMLOCK:
	case VK_SCROLL:
	case VK_LWIN:
	case VK_RWIN:
		return true;

	default:
		return false;
	}
}

void
CMSWindowsScreen::ctrlAltDelThread(void*)
{
	// get the Winlogon desktop at whatever privilege we can
	HDESK desk = OpenDesktop("Winlogon", 0, FALSE, MAXIMUM_ALLOWED);
	if (desk != NULL) {
		if (SetThreadDesktop(desk)) {
			PostMessage(HWND_BROADCAST, WM_HOTKEY, 0,
						MAKELPARAM(MOD_CONTROL | MOD_ALT, VK_DELETE));
		}
		else {
			LOG((CLOG_DEBUG "can't switch to Winlogon desk: %d", GetLastError()));
		}
		CloseDesktop(desk);
	}
	else {
		LOG((CLOG_DEBUG "can't open Winlogon desk: %d", GetLastError()));
	}
}

LRESULT CALLBACK
CMSWindowsScreen::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	assert(s_screen != NULL);

	LRESULT result = 0;
	if (!s_screen->onEvent(hwnd, msg, wParam, lParam, &result)) {
		result = DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return result;
}
