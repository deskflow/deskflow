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
#include "CMSWindowsEventQueueBuffer.h"
#include "CMSWindowsScreenSaver.h"
#include "CClipboard.h"
#include "XScreen.h"
#include "CLock.h"
#include "CThread.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "CString.h"
#include "CStringUtil.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "TMethodJob.h"
#include "CArch.h"
#include "CArchMiscWindows.h"
#include <cstring>
#include <malloc.h>
#include <tchar.h>

// <unused>; <unused>
#define SYNERGY_MSG_SWITCH			SYNERGY_HOOK_LAST_MSG + 1
// <unused>; <unused>
#define SYNERGY_MSG_ENTER			SYNERGY_HOOK_LAST_MSG + 2
// <unused>; <unused>
#define SYNERGY_MSG_LEAVE			SYNERGY_HOOK_LAST_MSG + 3
// wParam = flags, HIBYTE(lParam) = virtual key, LOBYTE(lParam) = scan code
#define SYNERGY_MSG_FAKE_KEY		SYNERGY_HOOK_LAST_MSG + 4
 // flags, XBUTTON id
#define SYNERGY_MSG_FAKE_BUTTON		SYNERGY_HOOK_LAST_MSG + 5
// x; y
#define SYNERGY_MSG_FAKE_MOVE		SYNERGY_HOOK_LAST_MSG + 6
// delta; <unused>
#define SYNERGY_MSG_FAKE_WHEEL		SYNERGY_HOOK_LAST_MSG + 7
// POINT*; <unused>
#define SYNERGY_MSG_CURSOR_POS		SYNERGY_HOOK_LAST_MSG + 8
// IKeyState*; <unused>
#define SYNERGY_MSG_SYNC_KEYS		SYNERGY_HOOK_LAST_MSG + 9

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
CMSWindowsScreen*		CMSWindowsScreen::s_screen   = NULL;

CMSWindowsScreen::CMSWindowsScreen(bool isPrimary) :
	m_isPrimary(isPrimary),
	m_is95Family(CArchMiscWindows::isWindows95Family()),
	m_isOnScreen(m_isPrimary),
	m_class(0),
	m_cursor(NULL),
	m_window(NULL),
	m_x(0), m_y(0),
	m_w(0), m_h(0),
	m_xCenter(0), m_yCenter(0),
	m_multimon(false),
	m_xCursor(0), m_yCursor(0),
	m_sequenceNumber(0),
	m_mark(0),
	m_markReceived(0),
	m_keyLayout(NULL),
	m_timer(NULL),
	m_screensaver(NULL),
	m_screensaverNotify(false),
	m_nextClipboardWindow(NULL),
	m_ownClipboard(false),
	m_activeDesk(NULL),
	m_activeDeskName(),
	m_hookLibrary(NULL),
	m_keyState(NULL),
	m_mutex(),
	m_deskReady(&m_mutex, false)
{
	assert(s_instance != NULL);
	assert(s_screen   == NULL);

	s_screen = this;
	try {
		m_hookLibrary = openHookLibrary("synrgyhk");
		m_cursor      = createBlankCursor();
		m_class       = createWindowClass();
		m_deskClass   = createDeskWindowClass(m_isPrimary);
		updateScreenShape();
		m_window      = createWindow(m_class, "Synergy");
		m_screensaver = new CMSWindowsScreenSaver();
		LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : ""));
		LOG((CLOG_DEBUG "window is 0x%08x", m_window));
	}
	catch (...) {
		delete m_screensaver;
		destroyWindow(m_window);
		destroyClass(m_deskClass);
		destroyClass(m_class);
		destroyCursor(m_cursor);
		closeHookLibrary(m_hookLibrary);
		m_screensaver = NULL;
		m_class       = 0;
		m_cursor      = NULL;
		m_hookLibrary = NULL;
		s_screen      = NULL;
		throw;
	}

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	// install event handlers
	EVENTQUEUE->adoptHandler(CEvent::kSystem, IEventQueue::getSystemTarget(),
							new TMethodEventJob<CMSWindowsScreen>(this,
								&CMSWindowsScreen::handleSystemEvent));

	// install the platform event queue
	EVENTQUEUE->adoptBuffer(new CMSWindowsEventQueueBuffer);
}

CMSWindowsScreen::~CMSWindowsScreen()
{
	assert(s_screen != NULL);

	disable();
	EVENTQUEUE->adoptBuffer(NULL);
	EVENTQUEUE->removeHandler(CEvent::kSystem, IEventQueue::getSystemTarget());
	removeDesks();
	ChangeClipboardChain(m_window, m_nextClipboardWindow);
	delete m_screensaver;
	destroyWindow(m_window);
	destroyClass(m_deskClass);
	destroyClass(m_class);
	destroyCursor(m_cursor);
	closeHookLibrary(m_hookLibrary);
	s_screen = NULL;
}

void
CMSWindowsScreen::init(HINSTANCE instance)
{
	assert(s_instance == NULL);
	assert(instance   != NULL);

	s_instance = instance;
}

HINSTANCE
CMSWindowsScreen::getInstance()
{
	return s_instance;
}

void
CMSWindowsScreen::setKeyState(IKeyState* keyState)
{
	m_keyState = keyState;
	m_keyMapper.update(m_keyState);
	memset(m_buttons, 0, sizeof(m_buttons));
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

	// set the active desk and (re)install the hooks
	checkDesk();

	// install the desk timer.  this timer periodically checks
	// which desk is active and reinstalls the hooks as necessary.
	// we wouldn't need this if windows notified us of a desktop
	// change but as far as i can tell it doesn't.
	m_timer = EVENTQUEUE->newTimer(0.2, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, m_timer,
							new TMethodEventJob<CMSWindowsScreen>(
								this, &CMSWindowsScreen::handleCheckDesk));
}

void
CMSWindowsScreen::disable()
{
	// remove timer
	if (m_timer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_timer);
		EVENTQUEUE->deleteTimer(m_timer);
		m_timer = NULL;
	}

	// disable hooks
	if (m_isPrimary) {
		m_setMode(kHOOK_DISABLE);
	}
}

void
CMSWindowsScreen::enter()
{
	sendDeskMessage(SYNERGY_MSG_ENTER, 0, 0);
	if (m_isPrimary) {
		// enable special key sequences on win95 family
		enableSpecialKeys(true);

		// watch jump zones
		m_setMode(kHOOK_WATCH_JUMP_ZONE);

		// all messages prior to now are invalid
		nextMark();
	}

	// now on screen
	m_isOnScreen = true;
}

bool
CMSWindowsScreen::leave()
{
	// get keyboard layout of foreground window.  we'll use this
	// keyboard layout for translating keys sent to clients.
	HWND window  = GetForegroundWindow();
	DWORD thread = GetWindowThreadProcessId(window, NULL);
	m_keyLayout  = GetKeyboardLayout(thread);

	// tell the key mapper about the keyboard layout
	m_keyMapper.setKeyLayout(m_keyLayout);

	// tell desk that we're leaving and tell it the keyboard layout
	sendDeskMessage(SYNERGY_MSG_LEAVE, (WPARAM)m_keyLayout, 0);

	if (m_isPrimary) {
/* XXX
		// update keys
		m_keyMapper.update(NULL);
*/

		// warp to center
		warpCursor(m_xCenter, m_yCenter);

		// disable special key sequences on win95 family
		enableSpecialKeys(false);

		// all messages prior to now are invalid
		nextMark();

		// capture events
		m_setMode(kHOOK_RELAY_EVENTS);
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
		sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardClipboard);
		sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardSelection);
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
	sendDeskMessage(SYNERGY_MSG_SYNC_KEYS, 0, 0);
	memset(m_buttons, 0, sizeof(m_buttons));
}

void
CMSWindowsScreen::setSequenceNumber(UInt32 seqNum)
{
	m_sequenceNumber = seqNum;
}

bool
CMSWindowsScreen::isPrimary() const
{
	return m_isPrimary;
}

void*
CMSWindowsScreen::getEventTarget() const
{
	return const_cast<CMSWindowsScreen*>(this);
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
	sendDeskMessage(SYNERGY_MSG_CURSOR_POS,
							reinterpret_cast<WPARAM>(&pos), 0);
	x = pos.x;
	y = pos.y;
}

void
CMSWindowsScreen::reconfigure(UInt32 activeSides)
{
	assert(m_isPrimary);

	LOG((CLOG_DEBUG "active sides: %x", activeSides));
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

void
CMSWindowsScreen::getCursorCenter(SInt32& x, SInt32& y) const
{
	x = m_xCenter;
	y = m_yCenter;
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
	UINT code = m_keyMapper.keyToScanCode(&virtualKey);
	sendDeskMessage(SYNERGY_MSG_FAKE_KEY, flags,
							MAKEWORD(static_cast<BYTE>(code),
								static_cast<BYTE>(virtualKey & 0xffu)));
}

bool
CMSWindowsScreen::fakeCtrlAltDel() const
{
	if (!m_is95Family) {
		// to fake ctrl+alt+del on the NT family we broadcast a suitable
		// hotkey to all windows on the winlogon desktop.  however, the
		// current thread must be on that desktop to do the broadcast
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
	DWORD data;
	DWORD flags = mapButtonToEvent(id, press, &data);
	sendDeskMessage(SYNERGY_MSG_FAKE_BUTTON, flags, data);
}

void
CMSWindowsScreen::fakeMouseMove(SInt32 x, SInt32 y) const
{
	sendDeskMessage(SYNERGY_MSG_FAKE_MOVE,
							static_cast<WPARAM>(x),
							static_cast<LPARAM>(y));
}

void
CMSWindowsScreen::fakeMouseWheel(SInt32 delta) const
{
	sendDeskMessage(SYNERGY_MSG_FAKE_WHEEL, delta, 0);
}

KeyButton
CMSWindowsScreen::mapKey(IKeyState::Keystrokes& keys,
						const IKeyState& keyState, KeyID id,
						KeyModifierMask desiredMask,
						bool isAutoRepeat) const
{
	return m_keyMapper.mapKey(keys, keyState, id, desiredMask, isAutoRepeat);
}

HINSTANCE
CMSWindowsScreen::openHookLibrary(const char* name)
{
	// load the hook library
	HINSTANCE hookLibrary = LoadLibrary(name);
	if (hookLibrary == NULL) {
		LOG((CLOG_ERR "Failed to load hook library;  %s.dll is missing", name));
		throw XScreenOpenFailure();
	}

	// look up functions
	m_setSides  = (SetSidesFunc)GetProcAddress(hookLibrary, "setSides");
	m_setZone   = (SetZoneFunc)GetProcAddress(hookLibrary, "setZone");
	m_setMode   = (SetModeFunc)GetProcAddress(hookLibrary, "setMode");
	m_install   = (InstallFunc)GetProcAddress(hookLibrary, "install");
	m_uninstall = (UninstallFunc)GetProcAddress(hookLibrary, "uninstall");
	m_init      = (InitFunc)GetProcAddress(hookLibrary, "init");
	m_cleanup   = (CleanupFunc)GetProcAddress(hookLibrary, "cleanup");
	m_installScreensaver   =
				  (InstallScreenSaverFunc)GetProcAddress(
								hookLibrary, "installScreenSaver");
	m_uninstallScreensaver =
				  (UninstallScreenSaverFunc)GetProcAddress(
								hookLibrary, "uninstallScreenSaver");
	if (m_setSides             == NULL ||
		m_setZone              == NULL ||
		m_setMode              == NULL ||
		m_install              == NULL ||
		m_uninstall            == NULL ||
		m_init                 == NULL ||
		m_cleanup              == NULL ||
		m_installScreensaver   == NULL ||
		m_uninstallScreensaver == NULL) {
		LOG((CLOG_ERR "Invalid hook library;  use a newer %s.dll", name));
		throw XScreenOpenFailure();
	}

	// initialize hook library
	if (m_init(GetCurrentThreadId()) == 0) {
		LOG((CLOG_ERR "Cannot initialize hook library;  is synergy already running?"));
		throw XScreenOpenFailure();
	}

	return hookLibrary;
}

void
CMSWindowsScreen::closeHookLibrary(HINSTANCE hookLibrary) const
{
	if (hookLibrary != NULL) {
		m_cleanup();
		FreeLibrary(hookLibrary);
	}
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
CMSWindowsScreen::destroyCursor(HCURSOR cursor) const
{
	if (cursor != NULL) {
		DestroyCursor(cursor);
	}
}

ATOM
CMSWindowsScreen::createWindowClass() const
{
	WNDCLASSEX classInfo;
	classInfo.cbSize        = sizeof(classInfo);
	classInfo.style         = CS_DBLCLKS | CS_NOCLOSE;
	classInfo.lpfnWndProc   = &CMSWindowsScreen::wndProc;
	classInfo.cbClsExtra    = 0;
	classInfo.cbWndExtra    = 0;
	classInfo.hInstance     = s_instance;
	classInfo.hIcon         = NULL;
	classInfo.hCursor       = NULL;
	classInfo.hbrBackground = NULL;
	classInfo.lpszMenuName  = NULL;
	classInfo.lpszClassName = "Synergy";
	classInfo.hIconSm       = NULL;
	return RegisterClassEx(&classInfo);
}

ATOM
CMSWindowsScreen::createDeskWindowClass(bool isPrimary) const
{
	WNDCLASSEX classInfo;
	classInfo.cbSize        = sizeof(classInfo);
	classInfo.style         = CS_DBLCLKS | CS_NOCLOSE;
	classInfo.lpfnWndProc   = isPrimary ?
								&CMSWindowsScreen::primaryDeskProc :
								&CMSWindowsScreen::secondaryDeskProc;
	classInfo.cbClsExtra    = 0;
	classInfo.cbWndExtra    = 0;
	classInfo.hInstance     = s_instance;
	classInfo.hIcon         = NULL;
	classInfo.hCursor       = m_cursor;
	classInfo.hbrBackground = NULL;
	classInfo.lpszMenuName  = NULL;
	classInfo.lpszClassName = "SynergyDesk";
	classInfo.hIconSm       = NULL;
	return RegisterClassEx(&classInfo);
}

void
CMSWindowsScreen::destroyClass(ATOM windowClass) const
{
	if (windowClass != 0) {
		UnregisterClass((LPCTSTR)windowClass, s_instance);
	}
}

HWND
CMSWindowsScreen::createWindow(ATOM windowClass, const char* name) const
{
	HWND window = CreateWindowEx(WS_EX_TOPMOST |
									WS_EX_TRANSPARENT |
									WS_EX_TOOLWINDOW,
								(LPCTSTR)windowClass,
								name,
								WS_POPUP,
								0, 0, 1, 1,
								NULL, NULL,
								s_instance,
								NULL);
	if (window == NULL) {
		LOG((CLOG_ERR "failed to create window: %d", GetLastError()));
		throw XScreenOpenFailure();
	}
	return window;
}

void
CMSWindowsScreen::destroyWindow(HWND hwnd) const
{
	if (hwnd != NULL) {
		DestroyWindow(hwnd);
	}
}

void
CMSWindowsScreen::sendEvent(CEvent::Type type, void* data)
{
	EVENTQUEUE->addEvent(CEvent(type, getEventTarget(), data));
}

void
CMSWindowsScreen::sendClipboardEvent(CEvent::Type type, ClipboardID id)
{
	CClipboardInfo* info   = (CClipboardInfo*)malloc(sizeof(CClipboardInfo));
	info->m_id             = id;
	info->m_sequenceNumber = m_sequenceNumber;
	sendEvent(type, info);
}

void
CMSWindowsScreen::handleSystemEvent(const CEvent& event, void*)
{
	MSG* msg = reinterpret_cast<MSG*>(event.getData());
	assert(msg != NULL);

	if (CArchMiscWindows::processDialog(msg)) {
		return;
	}
	if (onPreDispatch(msg->hwnd, msg->message, msg->wParam, msg->lParam)) {
		return;
	}
	TranslateMessage(msg);
	DispatchMessage(msg);
}

bool
CMSWindowsScreen::onPreDispatch(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam)
{
	// handle event
	switch (message) {
	case SYNERGY_MSG_SCREEN_SAVER:
		return onScreensaver(wParam != 0);
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
			sendEvent(getKeyUpEvent(), CKeyInfo::alloc(key, mask, button, 1));
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
			sendEvent(getKeyUpEvent(), CKeyInfo::alloc(key, mask, button, 1));
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
CMSWindowsScreen::onEvent(HWND, UINT msg,
				WPARAM wParam, LPARAM lParam, LRESULT* result)
{
	switch (msg) {
	case WM_QUERYENDSESSION:
		if (m_is95Family) {
			*result = TRUE;
			return true;
		}
		break;

	case WM_ENDSESSION:
		if (m_is95Family) {
			if (wParam == TRUE && lParam == 0) {
				EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
			}
			return true;
		}
		break;

	case WM_DRAWCLIPBOARD:
		LOG((CLOG_DEBUG "clipboard was taken"));

		// first pass on the message
		if (m_nextClipboardWindow != NULL) {
			SendMessage(m_nextClipboardWindow, msg, wParam, lParam);
		}

		// now handle the message
		return onClipboardChange();

	case WM_CHANGECBCHAIN:
		if (m_nextClipboardWindow == (HWND)wParam) {
			m_nextClipboardWindow = (HWND)lParam;
			LOG((CLOG_DEBUG "clipboard chain: new next: 0x%08x", m_nextClipboardWindow));
		}
		else if (m_nextClipboardWindow != NULL) {
			LOG((CLOG_DEBUG "clipboard chain: forward: %d 0x%08x 0x%08x", msg, wParam, lParam));
			SendMessage(m_nextClipboardWindow, msg, wParam, lParam);
		}
		return true;

	case WM_DISPLAYCHANGE:
		return onDisplayChange();
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
	WPARAM charAndVirtKey = wParam;
	wParam &= 0xffu;

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
		const KeyID key = m_keyMapper.mapKeyFromEvent(
							charAndVirtKey, lParam, &mask, &altgr);
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
						sendEvent(getKeyUpEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
					}
					if (ctrlR) {
						key      = kKeyControl_R;
						button   = VK_RCONTROL;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						sendEvent(getKeyUpEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
					}
					if (altL) {
						key      = kKeyAlt_L;
						button   = VK_LMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						sendEvent(getKeyUpEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
					}
					if (altR) {
						key      = kKeyAlt_R;
						button   = VK_RMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						sendEvent(getKeyUpEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
					}
				}

				// send key
				const bool wasDown = ((lParam & 0x40000000) != 0);
				SInt32 repeat      = (SInt32)(lParam & 0xffff);
				if (!wasDown) {
					LOG((CLOG_DEBUG1 "event: key press key=%d mask=0x%04x button=0x%04x", key, mask, button));
					sendEvent(getKeyDownEvent(),
							CKeyInfo::alloc(key, mask, button, 1));
					if (repeat > 0) {
						--repeat;
					}
				}
				if (repeat >= 1) {
					LOG((CLOG_DEBUG1 "event: key repeat key=%d mask=0x%04x count=%d button=0x%04x", key, mask, repeat, button));
					sendEvent(getKeyRepeatEvent(),
							CKeyInfo::alloc(key, mask, button, repeat));
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
						sendEvent(getKeyDownEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
						mask2 |= KeyModifierControl;
					}
					if (ctrlR) {
						key    = kKeyControl_R;
						button = VK_RCONTROL;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						sendEvent(getKeyDownEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
						mask2 |= KeyModifierControl;
					}
					if (altL) {
						key    = kKeyAlt_L;
						button = VK_LMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						sendEvent(getKeyDownEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
						mask2 |= KeyModifierAlt;
					}
					if (altR) {
						key    = kKeyAlt_R;
						button = VK_RMENU;
						scanCode = m_keyMapper.keyToScanCode(&button);
						button   = static_cast<KeyButton>(scanCode);
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
						sendEvent(getKeyDownEvent(),
							CKeyInfo::alloc(key, mask2, button, 1));
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
					sendEvent(getKeyDownEvent(),
							CKeyInfo::alloc(key, mask, button, 1));
					m_keyMapper.updateKey(static_cast<KeyButton>(wParam), true);
				}

				// do key up
				LOG((CLOG_DEBUG1 "event: key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
				sendEvent(getKeyUpEvent(),
							CKeyInfo::alloc(key, mask, button, 1));
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
				sendEvent(getButtonDownEvent(), CButtonInfo::alloc(button));
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
				sendEvent(getButtonUpEvent(), CButtonInfo::alloc(button));
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
		sendEvent(getMotionOnPrimaryEvent(),
							CMotionInfo::alloc(m_xCursor, m_yCursor));
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
			sendEvent(getMotionOnSecondaryEvent(), CMotionInfo::alloc(x, y));
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
		sendEvent(getWheelEvent(), CWheelInfo::alloc(delta));
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
			sendEvent(getScreensaverActivatedEvent());
		}
	}
	else {
		sendEvent(getScreensaverDeactivatedEvent());
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

		// send new screen info
		sendEvent(getShapeChangedEvent());
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
			sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardClipboard);
			sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardSelection);
		}
	}
	else {
		LOG((CLOG_DEBUG "clipboard changed: synergy owned"));
		m_ownClipboard = true;
	}

	return true;
}

void
CMSWindowsScreen::warpCursorNoFlush(SInt32 x, SInt32 y)
{
	// send an event that we can recognize before the mouse warp
	PostThreadMessage(GetCurrentThreadId(), SYNERGY_MSG_PRE_WARP, x, y);

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
	PostThreadMessage(GetCurrentThreadId(), SYNERGY_MSG_POST_WARP, 0, 0);
}

void
CMSWindowsScreen::nextMark()
{
	// next mark
	++m_mark;

	// mark point in message queue where the mark was changed
	PostThreadMessage(GetCurrentThreadId(), SYNERGY_MSG_MARK, m_mark, 0);
}

bool
CMSWindowsScreen::ignore() const
{
	return (m_mark != m_markReceived);
}

void
CMSWindowsScreen::updateScreenShape()
{
	// get shape
	m_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	m_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// get center for cursor
	m_xCenter = GetSystemMetrics(SM_CXSCREEN) >> 1;
	m_yCenter = GetSystemMetrics(SM_CYSCREEN) >> 1;

	// check for multiple monitors
	m_multimon = (m_w != GetSystemMetrics(SM_CXSCREEN) ||
				  m_h != GetSystemMetrics(SM_CYSCREEN));
	LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : ""));
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

LRESULT CALLBACK
CMSWindowsScreen::primaryDeskProc(
				HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// FIXME
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
CMSWindowsScreen::secondaryDeskProc(
				HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_MOUSEMOVE:
		// hide window
		ShowWindow(hwnd, SW_HIDE);
		break;

	case WM_ACTIVATE:
		// hide window
		if (LOWORD(wParam) == WA_INACTIVE) {
			ShowWindow(hwnd, SW_HIDE);
		}
		break;

	case WM_ACTIVATEAPP:
		// hide window
		if (!(BOOL)wParam) {
			ShowWindow(hwnd, SW_HIDE);
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void
CMSWindowsScreen::deskMouseMove(SInt32 x, SInt32 y) const
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
	if (simple) {
		// when using absolute positioning with mouse_event(),
		// the normalized device coordinates range over only
		// the primary screen.
		SInt32 w = GetSystemMetrics(SM_CXSCREEN);
		SInt32 h = GetSystemMetrics(SM_CYSCREEN);
		mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
								(DWORD)((65535.0f * x) / (w - 1) + 0.5f),
								(DWORD)((65535.0f * y) / (h - 1) + 0.5f),
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
	// item to jump back and forth between the position on the primary
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

		// move relative to mouse position
		POINT pos;
		GetCursorPos(&pos);
		mouse_event(MOUSEEVENTF_MOVE, x - pos.x, y - pos.y, 0, 0);

		// restore mouse speed & acceleration
		if (accelChanged) {
			SystemParametersInfo(SPI_SETMOUSE, 0, oldSpeed, 0);
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, oldSpeed + 3, 0);
		}
	}
}

void
CMSWindowsScreen::deskEnter(CDesk* desk)
{
	if (m_isPrimary) {
		ShowCursor(TRUE);
	}
	ShowWindow(desk->m_window, SW_HIDE);
}

void
CMSWindowsScreen::deskLeave(CDesk* desk, HKL keyLayout)
{
	if (m_isPrimary) {
		// map a window to hide the cursor and to use whatever keyboard
		// layout we choose rather than the keyboard layout of the last
		// active window.
		int x, y, w, h;
		if (desk->m_lowLevel) {
			// with a low level hook the cursor will never budge so
			// just a 1x1 window is sufficient.
			x = m_xCenter;
			y = m_yCenter;
			w = 1;
			h = 1;
		}
		else {
			// with regular hooks the cursor will jitter as it's moved
			// by the user then back to the center by us.  to be sure
			// we never lose it, cover all the monitors with the window.
			x = m_x;
			y = m_y;
			w = m_w;
			h = m_h;
		}
		SetWindowPos(desk->m_window, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
		ShowWindow(desk->m_window, SW_SHOW);
		ShowCursor(FALSE);

		// switch to requested keyboard layout
		ActivateKeyboardLayout(keyLayout, 0);
	}
	else {
		// move hider window under the cursor center
		MoveWindow(desk->m_window, m_xCenter, m_yCenter, 1, 1, FALSE);

		// raise and show the hider window
		ShowWindow(desk->m_window, SW_SHOWNA);

		// warp the mouse to the cursor center
		deskMouseMove(m_xCenter, m_yCenter);
	}
}

void
CMSWindowsScreen::deskThread(void* vdesk)
{
	MSG msg;

	// use given desktop for this thread
	CDesk* desk      = reinterpret_cast<CDesk*>(vdesk);
	desk->m_threadID = GetCurrentThreadId();
	desk->m_window   = NULL;
	if (desk->m_desk != NULL && SetThreadDesktop(desk->m_desk) != 0) {
		// create a message queue
		PeekMessage(&msg, NULL, 0,0, PM_NOREMOVE);

		// create a window.  we use this window to hide the cursor.
		try {
			desk->m_window = createWindow(m_deskClass, "SynergyDesk");
			LOG((CLOG_DEBUG "desk %s window is 0x%08x", desk->m_name.c_str(), desk->m_window));
		}
		catch (...) {
			// ignore
			LOG((CLOG_DEBUG "can't create desk window for %s", desk->m_name.c_str()));
		}

		// a window on the primary screen should never activate
		if (m_isPrimary && desk->m_window != NULL) {
			EnableWindow(desk->m_window, FALSE);
		}
	}

	// tell main thread that we're ready
	{
		CLock lock(&m_mutex);
		m_deskReady = true;
		m_deskReady.broadcast();
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		switch (msg.message) {
		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;

		case SYNERGY_MSG_SWITCH:
			if (m_isPrimary) {
				m_uninstall();
				if (m_screensaverNotify) {
					m_uninstallScreensaver();
					m_installScreensaver();
				}
				switch (m_install()) {
				case kHOOK_FAILED:
					// we won't work on this desk
					desk->m_lowLevel = false;
					break;

				case kHOOK_OKAY:
					desk->m_lowLevel = false;
					break;

				case kHOOK_OKAY_LL:
					desk->m_lowLevel = true;
					break;
				}
			}
			break;

		case SYNERGY_MSG_ENTER:
			deskEnter(desk);
			break;

		case SYNERGY_MSG_LEAVE:
			deskLeave(desk, (HKL)msg.wParam);
			break;

		case SYNERGY_MSG_FAKE_KEY:
			keybd_event(HIBYTE(msg.lParam), LOBYTE(msg.lParam), msg.wParam, 0);
			break;

		case SYNERGY_MSG_FAKE_BUTTON:
			if (msg.wParam != 0) {
				mouse_event(msg.wParam, 0, 0, msg.lParam, 0);
			}
			break;

		case SYNERGY_MSG_FAKE_MOVE:
			deskMouseMove(static_cast<SInt32>(msg.wParam),
							static_cast<SInt32>(msg.lParam));
			break;

		case SYNERGY_MSG_FAKE_WHEEL:
			mouse_event(MOUSEEVENTF_WHEEL, 0, 0, msg.wParam, 0);
			break;

		case SYNERGY_MSG_CURSOR_POS: {
			POINT* pos = reinterpret_cast<POINT*>(msg.wParam);
			if (!GetCursorPos(pos)) {
				pos->x = m_xCenter;
				pos->y = m_yCenter;
			}
			break;
		}

		case SYNERGY_MSG_SYNC_KEYS:
			m_keyMapper.update(m_keyState);
			break;
		}

		// notify that message was processed
		CLock lock(&m_mutex);
		m_deskReady = true;
		m_deskReady.broadcast();
	}

	// clean up
	if (desk->m_window != NULL) {
		DestroyWindow(desk->m_window);
	}
	if (desk->m_desk != NULL) {
		CMSWindowsDesktop::closeDesktop(desk->m_desk);
	}
}

CMSWindowsScreen::CDesk*
CMSWindowsScreen::addDesk(const CString& name, HDESK hdesk)
{
	CDesk* desk      = new CDesk;
	desk->m_name     = name;
	desk->m_desk     = hdesk;
	desk->m_targetID = GetCurrentThreadId();
	desk->m_thread   = new CThread(new TMethodJob<CMSWindowsScreen>(
						this, &CMSWindowsScreen::deskThread, desk));
	waitForDesk();
	m_desks.insert(std::make_pair(name, desk));
	return desk;
}

void
CMSWindowsScreen::removeDesks()
{
	for (CDesks::iterator index = m_desks.begin();
							index != m_desks.end(); ++index) {
		CDesk* desk = index->second;
		PostThreadMessage(desk->m_threadID, WM_QUIT, 0, 0);
		desk->m_thread->wait();
		delete desk->m_thread;
		delete desk;
	}
	m_desks.clear();
}

void
CMSWindowsScreen::checkDesk()
{
	// get current desktop.  if we already know about it then return.
	CDesk* desk;
	HDESK hdesk  = CMSWindowsDesktop::openInputDesktop();
	CString name = CMSWindowsDesktop::getDesktopName(hdesk);
	CDesks::const_iterator index = m_desks.find(name);
	if (index == m_desks.end()) {
		desk = addDesk(name, hdesk);
		// hold on to hdesk until thread exits so the desk can't
		// be removed by the system
	}
	else {
		CMSWindowsDesktop::closeDesktop(hdesk);
		desk = index->second;
	}

	// if active desktop changed then tell the old and new desk threads
	// about the change.  don't switch desktops when the screensaver is
	// active becaue we'd most likely switch to the screensaver desktop
	// which would have the side effect of forcing the screensaver to
	// stop.
	// FIXME -- really not switch if screensaver is active?
	if (name != m_activeDeskName && !m_screensaver->isActive()) {
		// show cursor on previous desk
		if (!m_isOnScreen) {
			sendDeskMessage(SYNERGY_MSG_ENTER, 0, 0);
		}

		// check for desk accessibility change.  we don't get events
		// from an inaccessible desktop so when we switch from an
		// inaccessible desktop to an accessible one we have to
		// update the keyboard state.
		LOG((CLOG_DEBUG "switched to desk \"%s\"", name.c_str()));
		bool isAccessible = isDeskAccessible(desk);
		if (isDeskAccessible(m_activeDesk) != isAccessible) {
			if (isAccessible) {
				LOG((CLOG_DEBUG "desktop is now accessible"));
				updateKeys();
			}
			else {
				LOG((CLOG_DEBUG "desktop is now inaccessible"));
			}
		}

		// switch desk
		m_activeDesk     = desk;
		m_activeDeskName = name;
		sendDeskMessage(SYNERGY_MSG_SWITCH, 0, 0);

		// hide cursor on new desk
		if (!m_isOnScreen) {
			sendDeskMessage(SYNERGY_MSG_LEAVE, (WPARAM)m_keyLayout, 0);
		}
	}
}

// FIXME -- may want some of following when we switch desks.  calling
// nextMark() for isPrimary may lead to loss of events.  updateKeys()
// is to catch any key events lost between switching and detecting the
// switch.  neither are strictly necessary.
/*
	if (m_isPrimary) {
		if (m_isOnScreen) {
			// all messages prior to now are invalid
			// FIXME -- is this necessary;  couldn't we lose key releases?
			nextMark();
		}
	}
	else {
		// update key state
		updateKeys();
	}
*/

bool
CMSWindowsScreen::isDeskAccessible(const CDesk* desk) const
{
	return (desk != NULL && desk->m_desk != NULL);
}

void
CMSWindowsScreen::sendDeskMessage(UINT msg, WPARAM wParam, LPARAM lParam) const
{
	if (m_activeDesk != NULL && m_activeDesk->m_window != NULL) {
		PostThreadMessage(m_activeDesk->m_threadID, msg, wParam, lParam);
		waitForDesk();
	}
}

void
CMSWindowsScreen::waitForDesk() const
{
	CMSWindowsScreen* self = const_cast<CMSWindowsScreen*>(this);

	CLock lock(&m_mutex);
	while (!(bool)m_deskReady) {
		m_deskReady.wait();
	}
	self->m_deskReady = false;
}

void
CMSWindowsScreen::handleCheckDesk(const CEvent&, void*)
{
	checkDesk();
}
