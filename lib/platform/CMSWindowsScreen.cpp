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
#include "CMSWindowsDesks.h"
#include "CMSWindowsEventQueueBuffer.h"
#include "CMSWindowsKeyState.h"
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
#include <string.h>
#include <pbt.h>

//
// add backwards compatible multihead support (and suppress bogus warning).
// this isn't supported on MinGW yet AFAICT.
//
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4706) // assignment within conditional
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#pragma warning(pop)
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
#if !defined(VK_XBUTTON1)
#define VK_XBUTTON1			0x05
#define VK_XBUTTON2			0x06
#endif

// WM_POWERBROADCAST stuff
#if !defined(PBT_APMRESUMEAUTOMATIC)
#define PBT_APMRESUMEAUTOMATIC	0x0012
#endif

//
// CMSWindowsScreen
//

HINSTANCE				CMSWindowsScreen::s_instance = NULL;
CMSWindowsScreen*		CMSWindowsScreen::s_screen   = NULL;

CMSWindowsScreen::CMSWindowsScreen(bool isPrimary,
				IJob* suspend, IJob* resume) :
	m_isPrimary(isPrimary),
	m_is95Family(CArchMiscWindows::isWindows95Family()),
	m_isOnScreen(m_isPrimary),
	m_class(0),
	m_x(0), m_y(0),
	m_w(0), m_h(0),
	m_xCenter(0), m_yCenter(0),
	m_multimon(false),
	m_xCursor(0), m_yCursor(0),
	m_sequenceNumber(0),
	m_mark(0),
	m_markReceived(0),
	m_keyLayout(NULL),
	m_fixTimer(NULL),
	m_screensaver(NULL),
	m_screensaverNotify(false),
	m_screensaverActive(false),
	m_window(NULL),
	m_nextClipboardWindow(NULL),
	m_ownClipboard(false),
	m_desks(NULL),
	m_hookLibrary(NULL),
	m_init(NULL),
	m_cleanup(NULL),
	m_setSides(NULL),
	m_setZone(NULL),
	m_setMode(NULL),
	m_keyState(NULL),
	m_suspend(suspend),
	m_resume(resume),
	m_hasMouse(GetSystemMetrics(SM_MOUSEPRESENT) != 0),
	m_showingMouse(false)
{
	assert(s_instance != NULL);
	assert(s_screen   == NULL);

	s_screen = this;
	try {
		if (m_isPrimary) {
			m_hookLibrary = openHookLibrary("synrgyhk");
		}
		m_screensaver = new CMSWindowsScreenSaver();
		m_desks       = new CMSWindowsDesks(m_isPrimary,
							m_hookLibrary, m_screensaver,
							new TMethodJob<CMSWindowsScreen>(this,
								&CMSWindowsScreen::updateKeysCB));
		m_keyState    = new CMSWindowsKeyState(m_desks);
		updateScreenShape();
		m_class       = createWindowClass();
		m_window      = createWindow(m_class, "Synergy");
		forceShowCursor();
		LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : ""));
		LOG((CLOG_DEBUG "window is 0x%08x", m_window));
	}
	catch (...) {
		delete m_keyState;
		delete m_desks;
		delete m_screensaver;
		destroyWindow(m_window);
		destroyClass(m_class);
		closeHookLibrary(m_hookLibrary);
		delete m_suspend;
		delete m_resume;
		s_screen = NULL;
		throw;
	}

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
	delete m_keyState;
	delete m_desks;
	delete m_screensaver;
	destroyWindow(m_window);
	destroyClass(m_class);
	closeHookLibrary(m_hookLibrary);
	delete m_suspend;
	delete m_resume;
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
CMSWindowsScreen::enable()
{
	assert(m_isOnScreen == m_isPrimary);

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	// track the active desk and (re)install the hooks
	m_desks->enable();

	if (m_isPrimary) {
		// set jump zones
		m_setZone(m_x, m_y, m_w, m_h, getJumpZoneSize());

		// watch jump zones
		m_setMode(kHOOK_WATCH_JUMP_ZONE);
	}
	else {
		// prevent the system from entering power saving modes.  if
		// it did we'd be forced to disconnect from the server and
		// the server would not be able to wake us up.
		CArchMiscWindows::addBusyState(CArchMiscWindows::kSYSTEM);
	}
}

void
CMSWindowsScreen::disable()
{
	// stop tracking the active desk
	m_desks->disable();

	if (m_isPrimary) {
		// disable hooks
		m_setMode(kHOOK_DISABLE);

		// enable special key sequences on win95 family
		enableSpecialKeys(true);
	}
	else {
		// allow the system to enter power saving mode
		CArchMiscWindows::removeBusyState(CArchMiscWindows::kSYSTEM |
							CArchMiscWindows::kDISPLAY);
	}

	// uninstall fix key timer
	if (m_fixTimer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_fixTimer);
		EVENTQUEUE->deleteTimer(m_fixTimer);
		m_fixTimer = NULL;
	}

	// stop snooping the clipboard
	ChangeClipboardChain(m_window, m_nextClipboardWindow);
	m_nextClipboardWindow = NULL;

	m_isOnScreen = m_isPrimary;
	forceShowCursor();
}

void
CMSWindowsScreen::enter()
{
	m_desks->enter();
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
	forceShowCursor();
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
	m_keyState->setKeyLayout(m_keyLayout);

	// tell desk that we're leaving and tell it the keyboard layout
	m_desks->leave(m_keyLayout);

	if (m_isPrimary) {
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
	forceShowCursor();

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
		m_desks->installScreensaverHooks(true);
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
			m_desks->installScreensaverHooks(false);
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
	m_desks->getCursorPos(x, y);
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
		"<invalid>",
		"Left Button",
		"Middle Button",
		"Right Button",
		"X Button 1",
		"X Button 2"
	};

	for (UInt32 i = 1; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
		if (m_buttons[i]) {
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

void
CMSWindowsScreen::fakeMouseButton(ButtonID id, bool press) const
{
	m_desks->fakeMouseButton(id, press);
}

void
CMSWindowsScreen::fakeMouseMove(SInt32 x, SInt32 y) const
{
	m_desks->fakeMouseMove(x, y);
}

void
CMSWindowsScreen::fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const
{
	m_desks->fakeMouseRelativeMove(dx, dy);
}

void
CMSWindowsScreen::fakeMouseWheel(SInt32 delta) const
{
	m_desks->fakeMouseWheel(delta);
}

void
CMSWindowsScreen::updateKeys()
{
	m_desks->updateKeys();
}

void
CMSWindowsScreen::fakeKeyDown(KeyID id, KeyModifierMask mask,
				KeyButton button)
{
	CPlatformScreen::fakeKeyDown(id, mask, button);
	updateForceShowCursor();
}

void
CMSWindowsScreen::fakeKeyRepeat(KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	CPlatformScreen::fakeKeyRepeat(id, mask, count, button);
	updateForceShowCursor();
}

void
CMSWindowsScreen::fakeKeyUp(KeyButton button)
{
	CPlatformScreen::fakeKeyUp(button);
	updateForceShowCursor();
}

void
CMSWindowsScreen::fakeToggle(KeyModifierMask modifier)
{
	CPlatformScreen::fakeToggle(modifier);
	updateForceShowCursor();
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
	m_init      = (InitFunc)GetProcAddress(hookLibrary, "init");
	m_cleanup   = (CleanupFunc)GetProcAddress(hookLibrary, "cleanup");
	if (m_setSides             == NULL ||
		m_setZone              == NULL ||
		m_setMode              == NULL ||
		m_init                 == NULL ||
		m_cleanup              == NULL) {
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

void
CMSWindowsScreen::destroyClass(ATOM windowClass) const
{
	if (windowClass != 0) {
		UnregisterClass(reinterpret_cast<LPCTSTR>(windowClass), s_instance);
	}
}

HWND
CMSWindowsScreen::createWindow(ATOM windowClass, const char* name) const
{
	HWND window = CreateWindowEx(WS_EX_TOPMOST |
									WS_EX_TRANSPARENT |
									WS_EX_TOOLWINDOW,
								reinterpret_cast<LPCTSTR>(windowClass),
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

void
CMSWindowsScreen::updateButtons()
{
	int numButtons               = GetSystemMetrics(SM_CMOUSEBUTTONS);
	m_buttons[kButtonNone]       = false;
	m_buttons[kButtonLeft]       = (GetKeyState(VK_LBUTTON)  < 0);
	m_buttons[kButtonRight]      = (GetKeyState(VK_RBUTTON)  < 0);
	m_buttons[kButtonMiddle]     = (GetKeyState(VK_MBUTTON)  < 0);
	m_buttons[kButtonExtra0 + 0] = (numButtons >= 4) &&
								   (GetKeyState(VK_XBUTTON1) < 0);
	m_buttons[kButtonExtra0 + 1] = (numButtons >= 5) &&
								   (GetKeyState(VK_XBUTTON2) < 0);
}

IKeyState*
CMSWindowsScreen::getKeyState() const
{
	return m_keyState;
}

bool
CMSWindowsScreen::onPreDispatch(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam)
{
	// handle event
	switch (message) {
	case SYNERGY_MSG_SCREEN_SAVER:
		return onScreensaver(wParam != 0);

	case SYNERGY_MSG_DEBUG:
		LOG((CLOG_DEBUG1 "hook: 0x%08x 0x%08x", wParam, lParam));
		return true;
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

	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC:
		case PBT_APMRESUMECRITICAL:
		case PBT_APMRESUMESUSPEND:
			if (m_resume != NULL) {
				m_resume->run();
			}
			break;

		case PBT_APMSUSPEND:
			if (m_suspend != NULL) {
				m_suspend->run();
			}
			break;
		}
		*result = TRUE;
		return true;

	case WM_DEVICECHANGE:
		forceShowCursor();
		break;

	case WM_SETTINGCHANGE:
		if (wParam == SPI_SETMOUSEKEYS) {
			forceShowCursor();
		}
		break;
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
	LOG((CLOG_DEBUG1 "event: Key char=%d, vk=0x%02x, lParam=0x%08x", (wParam & 0xff00u) >> 8, wParam & 0xffu, lParam));

	// fix up key state
	fixKeys();

	// get key info
	KeyButton button = (KeyButton)((lParam & 0x01ff0000) >> 16);
	bool down        = ((lParam & 0xc0000000u) == 0x00000000u);
	bool up          = ((lParam & 0x80000000u) == 0x80000000u);
	bool wasDown     = isKeyDown(button);

	// the windows keys are a royal pain on the windows 95 family.
	// the system eats the key up events if and only if the windows
	// key wasn't combined with another key, i.e. it was tapped.
	// fixKeys() and scheduleFixKeys() are all about synthesizing
	// the missing key up.  but even windows itself gets a little
	// confused and sets bit 30 in lParam if you tap the windows
	// key twice.  that bit means the key was previously down and
	// that makes some sense since the up event was missing.
	// anyway, on the windows 95 family we forget about windows
	// key repeats and treat anything that's not a key down as a
	// key up.
	if (m_is95Family &&
		((wParam & 0xffu) == VK_LWIN || (wParam & 0xffu) == VK_RWIN)) {
		down = !up;
	}

	// update key state.  ignore key repeats.
	if (down) {
		m_keyState->setKeyDown(button, true);
	}
	else if (up) {
		m_keyState->setKeyDown(button, false);
	}

	// schedule a timer if we need to fix keys later
	scheduleFixKeys();

	// special case:  we detect ctrl+alt+del being pressed on some
	// systems but we don't detect the release of those keys.  so
	// if ctrl, alt, and del are down then mark them up.
	KeyModifierMask mask = getActiveModifiers();
	bool ctrlAlt = ((mask & (KeyModifierControl | KeyModifierAlt)) ==
							(KeyModifierControl | KeyModifierAlt));
	if (down && ctrlAlt &&
		isKeyDown(m_keyState->virtualKeyToButton(VK_DELETE))) {
		m_keyState->setKeyDown(
							m_keyState->virtualKeyToButton(VK_LCONTROL), false);
		m_keyState->setKeyDown(
							m_keyState->virtualKeyToButton(VK_RCONTROL), false);
		m_keyState->setKeyDown(
							m_keyState->virtualKeyToButton(VK_LMENU), false);
		m_keyState->setKeyDown(
							m_keyState->virtualKeyToButton(VK_RMENU), false);
		m_keyState->setKeyDown(
							m_keyState->virtualKeyToButton(VK_DELETE), false);
	}

	// ignore message if posted prior to last mark change
	if (!ignore()) {
		// check for ctrl+alt+del.  we do not want to pass that to the
		// client.  the user can use ctrl+alt+pause to emulate it.
		UINT virtKey = (wParam & 0xffu);
		if (virtKey == VK_DELETE && ctrlAlt) {
			LOG((CLOG_DEBUG "discard ctrl+alt+del"));
			return true;
		}

		// check for ctrl+alt+del emulation
		if ((virtKey == VK_PAUSE || virtKey == VK_CANCEL) && ctrlAlt) {
			LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
			// switch wParam and lParam to be as if VK_DELETE was
			// pressed or released
			wParam  = VK_DELETE;
			lParam &= 0xfe000000;
			lParam |= m_keyState->virtualKeyToButton(wParam) << 16;
			lParam |= 0x00000001;
		}

		// process key
		KeyModifierMask mask;
		KeyID key = m_keyState->mapKeyFromEvent(wParam, lParam, &mask);
		button    = static_cast<KeyButton>((lParam & 0x01ff0000u) >> 16);
		if (key != kKeyNone) {
			// fix up key.  if the key isn't down according to
			// our table then we never got the key press event
			// for it.  if it's not a modifier key then we'll
			// synthesize the press first.  only do this on
			// the windows 95 family, which eats certain special
			// keys like alt+tab, ctrl+esc, etc.
			if (m_is95Family && !wasDown && up) {
				switch (virtKey) {
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
					break;

				default:
					m_keyState->sendKeyEvent(getEventTarget(),
							true, false, key, mask, 1, button);
					break;
				}
			}

			// do it
			m_keyState->sendKeyEvent(getEventTarget(),
							((lParam & 0x80000000u) == 0),
							((lParam & 0x40000000u) == 1),
							key, mask, (SInt32)(lParam & 0xffff), button);
		}
		else {
			LOG((CLOG_DEBUG2 "event: cannot map key"));
		}
	}

	return true;
}

bool
CMSWindowsScreen::onMouseButton(WPARAM wParam, LPARAM lParam)
{
	// get which button
	bool pressed    = mapPressFromEvent(wParam, lParam);
	ButtonID button = mapButtonFromEvent(wParam, lParam);

	// keep our shadow key state up to date
	if (button >= kButtonLeft && button <= kButtonExtra0 + 1) {
		if (pressed) {
			m_buttons[button] = true;
		}
		else {
			m_buttons[button] = false;
		}
	}

	// ignore message if posted prior to last mark change
	if (!ignore()) {
		if (pressed) {
			LOG((CLOG_DEBUG1 "event: button press button=%d", button));
			if (button != kButtonNone) {
				sendEvent(getButtonDownEvent(), CButtonInfo::alloc(button));
			}
		}
		else {
			LOG((CLOG_DEBUG1 "event: button release button=%d", button));
			if (button != kButtonNone) {
				sendEvent(getButtonUpEvent(), CButtonInfo::alloc(button));
			}
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
		if (!m_screensaverActive &&
			m_screensaver->checkStarted(SYNERGY_MSG_SCREEN_SAVER, FALSE, 0)) {
			m_screensaverActive = true;
			sendEvent(getScreensaverActivatedEvent());

			// enable display power down
			CArchMiscWindows::removeBusyState(CArchMiscWindows::kDISPLAY);
		}
	}
	else {
		if (m_screensaverActive) {
			m_screensaverActive = false;
			sendEvent(getScreensaverDeactivatedEvent());

			// disable display power down
			CArchMiscWindows::addBusyState(CArchMiscWindows::kDISPLAY);
		}
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

		LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : ""));
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

	// tell the desks
	m_desks->setShape(m_x, m_y, m_w, m_h, m_xCenter, m_yCenter, m_multimon);
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
			if (GetSystemMetrics(SM_CMOUSEBUTTONS) >= 4) {
				return kButtonExtra0 + 0;
			}
			break;

		case XBUTTON2:
			if (GetSystemMetrics(SM_CMOUSEBUTTONS) >= 5) {
				return kButtonExtra0 + 1;
			}
			break;
		}
		return kButtonNone;

	default:
		return kButtonNone;
	}
}

bool
CMSWindowsScreen::mapPressFromEvent(WPARAM msg, LPARAM) const
{
	switch (msg) {
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
		return true;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_NCLBUTTONUP:
	case WM_NCMBUTTONUP:
	case WM_NCRBUTTONUP:
	case WM_NCXBUTTONUP:
		return false;

	default:
		return false;
	}
}

void
CMSWindowsScreen::fixKeys()
{
	// fake key releases for the windows keys if we think they're
	// down but they're really up.  we have to do this because if the
	// user presses and releases a windows key without pressing any
	// other key while it's down then the system will eat the key
	// release.  if we don't detect that and synthesize the release
	// then the client won't take the usual windows key release action
	// (which on windows is to show the start menu).
	//
	// only check on the windows 95 family since the NT family reports
	// the key releases as usual.
	if (m_is95Family) {
		m_keyState->fixKey(getEventTarget(), VK_LWIN);
		m_keyState->fixKey(getEventTarget(), VK_RWIN);

		// check if we need the fix timer anymore
		scheduleFixKeys();
	}
}

void
CMSWindowsScreen::scheduleFixKeys()
{
	if (m_is95Family) {
		// see if any keys that need fixing are down
		bool fix =
			(m_keyState->isKeyDown(m_keyState->virtualKeyToButton(VK_LWIN)) ||
			 m_keyState->isKeyDown(m_keyState->virtualKeyToButton(VK_RWIN)));

		// start or stop fix timer
		if (fix && m_fixTimer == NULL) {
			m_fixTimer = EVENTQUEUE->newTimer(0.1, NULL);
			EVENTQUEUE->adoptHandler(CEvent::kTimer, m_fixTimer,
								new TMethodEventJob<CMSWindowsScreen>(
									this, &CMSWindowsScreen::handleFixKeys));
		}
		else if (!fix && m_fixTimer != NULL) {
			EVENTQUEUE->removeHandler(CEvent::kTimer, m_fixTimer);
			EVENTQUEUE->deleteTimer(m_fixTimer);
			m_fixTimer = NULL;
		}
	}
}

void
CMSWindowsScreen::handleFixKeys(const CEvent&, void*)
{
	fixKeys();
}

void
CMSWindowsScreen::updateKeysCB(void*)
{
	m_keyState->updateKeys();
	updateButtons();
}

void
CMSWindowsScreen::forceShowCursor()
{
	// check for mouse
	m_hasMouse = (GetSystemMetrics(SM_MOUSEPRESENT) != 0);

	// decide if we should show the mouse
	bool showMouse = (!m_hasMouse && !m_isPrimary && m_isOnScreen);

	// show/hide the mouse
	if (showMouse != m_showingMouse) {
		if (showMouse) {
			m_oldMouseKeys.cbSize = sizeof(m_oldMouseKeys);
			m_gotOldMouseKeys =
				(SystemParametersInfo(SPI_GETMOUSEKEYS,
							m_oldMouseKeys.cbSize,	&m_oldMouseKeys, 0) != 0);
			if (m_gotOldMouseKeys) {
				m_mouseKeys    = m_oldMouseKeys;
				m_showingMouse = true;
				updateForceShowCursor();
			}
		}
		else {
			if (m_gotOldMouseKeys) {
				SystemParametersInfo(SPI_SETMOUSEKEYS,
							m_oldMouseKeys.cbSize,
							&m_oldMouseKeys, SPIF_SENDCHANGE);
				m_showingMouse = false;
			}
		}
	}
}

void
CMSWindowsScreen::updateForceShowCursor()
{
	DWORD oldFlags = m_mouseKeys.dwFlags;

	// turn on MouseKeys
	m_mouseKeys.dwFlags = MKF_AVAILABLE | MKF_MOUSEKEYSON;

	// make sure MouseKeys is active in whatever state the NumLock is
	// not currently in.
	if ((m_keyState->getActiveModifiers() & KeyModifierNumLock) != 0) {
		m_mouseKeys.dwFlags |= MKF_REPLACENUMBERS;
	}

	// update MouseKeys
	if (oldFlags != m_mouseKeys.dwFlags) {
		SystemParametersInfo(SPI_SETMOUSEKEYS,
							m_mouseKeys.cbSize, &m_mouseKeys, SPIF_SENDCHANGE);
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
