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

#include "CMSWindowsSecondaryScreen.h"
#include "CMSWindowsScreen.h"
#include "XScreen.h"
#include "CLock.h"
#include "CLog.h"
#include "CArchMiscWindows.h"
#include <cctype>

// these are only defined when WINVER >= 0x0500
#if !defined(SPI_GETMOUSESPEED)
#define SPI_GETMOUSESPEED 112
#endif
#if !defined(SPI_SETMOUSESPEED)
#define SPI_SETMOUSESPEED 113
#endif

//
// CMSWindowsSecondaryScreen
//

CMSWindowsSecondaryScreen::CMSWindowsSecondaryScreen(
				IScreenReceiver* receiver) :
	m_is95Family(CArchMiscWindows::isWindows95Family()),
	m_window(NULL),
	m_mask(0)
{
	m_screen = new CMSWindowsScreen(receiver, this);
}

CMSWindowsSecondaryScreen::~CMSWindowsSecondaryScreen()
{
	assert(m_window == NULL);

	delete m_screen;
}

void
CMSWindowsSecondaryScreen::keyDown(KeyID key, KeyModifierMask mask)
{
	Keystrokes keys;
	UINT virtualKey;

	CLock lock(&m_mutex);
	m_screen->syncDesktop();

	// get the sequence of keys to simulate key press and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, kPress);
	if (keys.empty()) {
		return;
	}

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now down
	m_keys[virtualKey]         |= 0x80;
	m_fakeKeys[virtualKey]     |= 0x80;
	switch (virtualKey) {
	case VK_LSHIFT:
	case VK_RSHIFT:
		m_keys[VK_SHIFT]       |= 0x80;
		m_fakeKeys[VK_SHIFT]   |= 0x80;
		break;

	case VK_LCONTROL:
	case VK_RCONTROL:
		m_keys[VK_CONTROL]     |= 0x80;
		m_fakeKeys[VK_CONTROL] |= 0x80;
		break;

	case VK_LMENU:
	case VK_RMENU:
		m_keys[VK_MENU]        |= 0x80;
		m_fakeKeys[VK_MENU]    |= 0x80;
		break;
	}
}

void
CMSWindowsSecondaryScreen::keyRepeat(KeyID key,
				KeyModifierMask mask, SInt32 count)
{
	Keystrokes keys;
	UINT virtualKey;

	CLock lock(&m_mutex);
	m_screen->syncDesktop();

	// get the sequence of keys to simulate key repeat and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, kRepeat);
	if (keys.empty()) {
		return;
	}

	// generate key events
	doKeystrokes(keys, count);
}

void
CMSWindowsSecondaryScreen::keyUp(KeyID key, KeyModifierMask mask)
{
	Keystrokes keys;
	UINT virtualKey;

	CLock lock(&m_mutex);
	m_screen->syncDesktop();

	// get the sequence of keys to simulate key release and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, kRelease);
	if (keys.empty()) {
		return;
	}

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now up
	m_keys[virtualKey]             &= ~0x80;
	m_fakeKeys[virtualKey]         &= ~0x80;
	switch (virtualKey) {
	case VK_LSHIFT:
		if ((m_keys[VK_RSHIFT] & 0x80) == 0) {
			m_keys[VK_SHIFT]       &= ~0x80;
			m_fakeKeys[VK_SHIFT]   &= ~0x80;
		}
		break;

	case VK_RSHIFT:
		if ((m_keys[VK_LSHIFT] & 0x80) == 0) {
			m_keys[VK_SHIFT]       &= ~0x80;
			m_fakeKeys[VK_SHIFT]   &= ~0x80;
		}
		break;

	case VK_LCONTROL:
		if ((m_keys[VK_RCONTROL] & 0x80) == 0) {
			m_keys[VK_CONTROL]     &= ~0x80;
			m_fakeKeys[VK_CONTROL] &= ~0x80;
		}
		break;

	case VK_RCONTROL:
		if ((m_keys[VK_LCONTROL] & 0x80) == 0) {
			m_keys[VK_CONTROL]     &= ~0x80;
			m_fakeKeys[VK_CONTROL] &= ~0x80;
		}
		break;

	case VK_LMENU:
		if ((m_keys[VK_RMENU] & 0x80) == 0) {
			m_keys[VK_MENU]        &= ~0x80;
			m_fakeKeys[VK_MENU]    &= ~0x80;
		}
		break;

	case VK_RMENU:
		if ((m_keys[VK_LMENU] & 0x80) == 0) {
			m_keys[VK_MENU]        &= ~0x80;
			m_fakeKeys[VK_MENU]    &= ~0x80;
		}
		break;
	}
}

void
CMSWindowsSecondaryScreen::mouseDown(ButtonID button)
{
	CLock lock(&m_mutex);
	m_screen->syncDesktop();

	// map button id to button flag
	DWORD flags = mapButton(button, true);

	// send event
	if (flags != 0) {
		mouse_event(flags, 0, 0, 0, 0);
	}
}

void
CMSWindowsSecondaryScreen::mouseUp(ButtonID button)
{
	CLock lock(&m_mutex);
	m_screen->syncDesktop();

	// map button id to button flag
	DWORD flags = mapButton(button, false);

	// send event
	if (flags != 0) {
		mouse_event(flags, 0, 0, 0, 0);
	}
}

void
CMSWindowsSecondaryScreen::mouseMove(SInt32 x, SInt32 y)
{
	CLock lock(&m_mutex);
	m_screen->syncDesktop();
	warpCursor(x, y);
}

void
CMSWindowsSecondaryScreen::mouseWheel(SInt32 delta)
{
	CLock lock(&m_mutex);
	m_screen->syncDesktop();
	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0);
}

void
CMSWindowsSecondaryScreen::resetOptions()
{
	// no options
}

void
CMSWindowsSecondaryScreen::setOptions(const COptionsList& /*options*/)
{
	// no options
}

IScreen*
CMSWindowsSecondaryScreen::getScreen() const
{
	return m_screen;
}

void
CMSWindowsSecondaryScreen::onScreensaver(bool)
{
	// ignore
}

bool
CMSWindowsSecondaryScreen::onPreDispatch(const CEvent*)
{
	return false;
}

bool
CMSWindowsSecondaryScreen::onEvent(CEvent* event)
{
	assert(event != NULL);

	const MSG& msg = event->m_msg;
	switch (msg.message) {
	case WM_ACTIVATEAPP:
		if (msg.wParam == FALSE) {
			// some other app activated.  hide the hider window.
			ShowWindow(m_window, SW_HIDE);
		}
		break;
	}

	return false;
}

void
CMSWindowsSecondaryScreen::onOneShotTimerExpired(UInt32)
{
	// ignore
}

SInt32
CMSWindowsSecondaryScreen::getJumpZoneSize() const
{
	return 0;
}

void
CMSWindowsSecondaryScreen::postCreateWindow(HWND window)
{
	m_window = window;

	// update key state
	updateKeys();

	// hide cursor if this screen isn't active
	if (!isActive()) {
		showWindow();
	}
}

void
CMSWindowsSecondaryScreen::preDestroyWindow(HWND)
{
	// do nothing
}

void
CMSWindowsSecondaryScreen::onPreMainLoop()
{
	assert(m_window != NULL);
}

void
CMSWindowsSecondaryScreen::onPreOpen()
{
	assert(m_window == NULL);
}

void
CMSWindowsSecondaryScreen::onPreEnter()
{
	assert(m_window != NULL);
}

void
CMSWindowsSecondaryScreen::onPreLeave()
{
	assert(m_window != NULL);
}

void
CMSWindowsSecondaryScreen::createWindow()
{
	// open the desktop and the window
	m_window = m_screen->openDesktop();
	if (m_window == NULL) {
		throw XScreenOpenFailure();
	}
}

void
CMSWindowsSecondaryScreen::destroyWindow()
{
	// release keys that are logically pressed
	releaseKeys();

	// close the desktop and the window
	m_screen->closeDesktop();
	m_window = NULL;
}

void
CMSWindowsSecondaryScreen::showWindow()
{
	// move hider window under the mouse (rather than moving the mouse
	// somewhere else on the screen)
	SInt32 x, y;
	getCursorPos(x, y);
	MoveWindow(m_window, x, y, 1, 1, FALSE);

	// raise and show the hider window.  take activation.
	ShowWindow(m_window, SW_SHOWNORMAL);
}

void
CMSWindowsSecondaryScreen::hideWindow()
{
	ShowWindow(m_window, SW_HIDE);
}

void
CMSWindowsSecondaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	// motion is simple (i.e. it's on the primary monitor) if there
	// is only one monitor.
	bool simple = !m_screen->isMultimon();
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
CMSWindowsSecondaryScreen::updateKeys()
{
	// clear key state
	memset(m_keys, 0, sizeof(m_keys));
	memset(m_fakeKeys, 0, sizeof(m_keys));

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

	// copy over lock states to m_fakeKeys
	m_fakeKeys[VK_CAPITAL] = static_cast<BYTE>(m_keys[VK_CAPITAL] & 0x01);
	m_fakeKeys[VK_NUMLOCK] = static_cast<BYTE>(m_keys[VK_NUMLOCK] & 0x01);
	m_fakeKeys[VK_SCROLL]  = static_cast<BYTE>(m_keys[VK_SCROLL]  & 0x01);

	// update active modifier mask
	m_mask = 0;
	if ((m_keys[VK_LSHIFT] & 0x80) != 0 || (m_keys[VK_RSHIFT] & 0x80) != 0) {
		m_mask |= KeyModifierShift;
	}
	if ((m_keys[VK_LCONTROL] & 0x80) != 0 ||
		(m_keys[VK_RCONTROL] & 0x80) != 0) {
		m_mask |= KeyModifierControl;
	}
	if ((m_keys[VK_LMENU] & 0x80) != 0 || (m_keys[VK_RMENU] & 0x80) != 0) {
		m_mask |= KeyModifierAlt;
	}
	// note -- no keys for KeyModifierMeta
	if ((m_keys[VK_LWIN] & 0x80) != 0 || (m_keys[VK_RWIN] & 0x80) != 0) {
		m_mask |= KeyModifierSuper;
	}
	if ((m_keys[VK_CAPITAL] & 0x01) != 0) {
		m_mask |= KeyModifierCapsLock;
	}
	if ((m_keys[VK_NUMLOCK] & 0x01) != 0) {
		m_mask |= KeyModifierNumLock;
	}
	if ((m_keys[VK_SCROLL] & 0x01) != 0) {
		m_mask |= KeyModifierScrollLock;
	}
	// note -- do not save KeyModifierModeSwitch in m_mask
	LOG((CLOG_DEBUG2 "modifiers on update: 0x%04x", m_mask));
}

void
CMSWindowsSecondaryScreen::setToggleState(KeyModifierMask mask)
{
	// toggle modifiers that don't match the desired state
	if ((mask & KeyModifierCapsLock)   != (m_mask & KeyModifierCapsLock)) {
		toggleKey(VK_CAPITAL, KeyModifierCapsLock);
	}
	if ((mask & KeyModifierNumLock)    != (m_mask & KeyModifierNumLock)) {
		toggleKey(VK_NUMLOCK | 0x100, KeyModifierNumLock);
	}
	if ((mask & KeyModifierScrollLock) != (m_mask & KeyModifierScrollLock)) {
		toggleKey(VK_SCROLL, KeyModifierScrollLock);
	}
}

KeyModifierMask
CMSWindowsSecondaryScreen::getToggleState() const
{
	return (m_mask & (KeyModifierCapsLock |
					  KeyModifierNumLock |
					  KeyModifierScrollLock));
}

// map special KeyID keys to virtual key codes. if the key is an
// extended key then the entry is the virtual key code | 0x100.
// unmapped keys have a 0 entry.
static const UINT		g_mapEE00[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ VK_TAB, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xb0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xb8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, 0
};
static const UINT		g_mapEF00[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,
	/* 0x10 */ 0, 0, 0, VK_PAUSE, VK_SCROLL, 0/*sys-req*/, 0, 0,
	/* 0x18 */ 0, 0, 0, VK_ESCAPE, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ VK_HOME|0x100, VK_LEFT|0x100, VK_UP|0x100, VK_RIGHT|0x100,
	/* 0x54 */ VK_DOWN|0x100, VK_PRIOR|0x100, VK_NEXT|0x100, VK_END|0x100,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ VK_SELECT|0x100, VK_SNAPSHOT|0x100, VK_EXECUTE|0x100, VK_INSERT|0x100,
	/* 0x64 */ 0, 0, 0, VK_APPS|0x100,
	/* 0x68 */ 0, 0, VK_HELP|0x100, VK_CANCEL|0x100, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, VK_MODECHANGE|0x100, VK_NUMLOCK|0x100,
	/* 0x80 */ VK_SPACE, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, VK_TAB, 0, 0, 0, VK_RETURN|0x100, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,
	/* 0x98 */ VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT,
	/* 0x9c */ VK_END, 0, VK_INSERT, VK_DELETE,
	/* 0xa0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa8 */ 0, 0, VK_MULTIPLY, VK_ADD,
	/* 0xac */ VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE|0x100,
	/* 0xb0 */ VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3,
	/* 0xb4 */ VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,
	/* 0xb8 */ VK_NUMPAD8, VK_NUMPAD9, 0, 0, 0, 0, VK_F1, VK_F2,
	/* 0xc0 */ VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
	/* 0xc8 */ VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18,
	/* 0xd0 */ VK_F19, VK_F20, VK_F21, VK_F22, VK_F23, VK_F24, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL,
	/* 0xe4 */ VK_RCONTROL|0x100, VK_CAPITAL, 0, 0,
	/* 0xe8 */ 0, VK_LMENU, VK_RMENU|0x100, VK_LWIN|0x100,
	/* 0xec */ VK_RWIN|0x100, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, VK_DELETE|0x100
};

DWORD
CMSWindowsSecondaryScreen::mapButton(ButtonID button, bool press) const
{
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

	// map button id to button flag
	switch (button) {
	case kButtonLeft:
		return press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;

	case kButtonMiddle:
		return press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;

	case kButtonRight:
		return press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;

	default:
		return 0;
	}
}

KeyModifierMask
CMSWindowsSecondaryScreen::mapKey(Keystrokes& keys, UINT& virtualKey,
				KeyID id, KeyModifierMask mask, EKeyAction action) const
{
	virtualKey = 0;

	// check for special keys
	if ((id & 0xfffff000) == 0xe000) {
		if ((id & 0xff00) == 0xee00) {
			virtualKey = g_mapEE00[id & 0xff];
		}
		else if ((id & 0xff00) == 0xef00) {
			virtualKey = g_mapEF00[id & 0xff];
		}
		if (virtualKey == 0) {
			LOG((CLOG_DEBUG2 "unknown special key"));
			return m_mask;
		}
	}

	// special handling of VK_SNAPSHOT
	if ((virtualKey & 0xff) == VK_SNAPSHOT) {
		// ignore key repeats on print screen
		if (action != kRepeat) {
			// get event flags
			DWORD flags = 0;
			if (isExtendedKey(virtualKey)) {
				flags |= KEYEVENTF_EXTENDEDKEY;
			}
			if (action != kPress) {
				flags |= KEYEVENTF_KEYUP;
			}

			// active window or fullscreen?
			BYTE scan = 0;
			if ((mask & KeyModifierAlt) == 0) {
				scan = 1;
			}

			// send event
			keybd_event(static_cast<BYTE>(virtualKey & 0xff), scan, flags, 0);
		}
		return m_mask;
	}

	// get output mask.  default output mask carries over the current
	// toggle modifier states and includes desired shift, control, alt,
	// meta, and super states.
	KeyModifierMask outMask = (m_mask &
								(KeyModifierCapsLock |
								KeyModifierNumLock |
								KeyModifierScrollLock));
	outMask                |= (mask &
								(KeyModifierShift |
								KeyModifierControl |
								KeyModifierAlt |
								KeyModifierMeta |
								KeyModifierSuper));

	// set control and alt if mode shift (AltGr) is requested
	if ((mask & KeyModifierModeSwitch) != 0) {
		outMask |= KeyModifierControl | KeyModifierAlt;
	}

	// extract extended key flag
	const bool isExtended = ((virtualKey & 0x100) != 0);
	virtualKey           &= ~0x100;

	// if not in map then ask system to convert character
	if (virtualKey == 0) {
		// translate.  return no keys if unknown key.
		// FIXME -- handle unicode
		TCHAR ascii = static_cast<TCHAR>(id & 0x000000ff);
		SHORT vk = VkKeyScan(ascii);
		if (vk == 0xffff) {
			LOG((CLOG_DEBUG2 "no virtual key for character %d", id));
			return m_mask;
		}

		// use whatever shift state VkKeyScan says
		// FIXME -- also for control and alt, but it's more difficult
		// to determine if control and alt must be off or if it just
		// doesn't matter.
		outMask &= ~KeyModifierShift;

		// convert system modifier mask to our mask
		if (HIBYTE(vk) & 1) {
			outMask |= KeyModifierShift;
		}
		if (HIBYTE(vk) & 2) {
			outMask |= KeyModifierControl;
		}
		if (HIBYTE(vk) & 4) {
			outMask |= KeyModifierAlt;
		}

		// handle combination of caps-lock and shift.  if caps-lock is
		// off locally then use shift as necessary.  if caps-lock is on
		// locally then it reverses the meaning of shift for keys that
		// are subject to case conversion.
		if ((outMask & KeyModifierCapsLock) != 0) {
			if (tolower(ascii) != toupper(ascii)) {
				LOG((CLOG_DEBUG2 "flip shift"));
				outMask ^= KeyModifierShift;
			}
		}

		// get virtual key
		virtualKey = LOBYTE(vk);
	}

	// if in map then figure out correct modifier state
	else {
		// check numeric keypad.  note that while KeyID distinguishes
		// between the keypad movement keys (e.g. Home, left arrow),
		// the virtual keys do not.  however, the virtual keys do
		// distinguish between keypad numbers and operators (e.g.
		// add, multiply) and their main keyboard counterparts.
		// therefore, we can ignore the num-lock state for movement
		// virtual keys but not for numeric keys.
		if (virtualKey >= VK_NUMPAD0 && virtualKey <= VK_DIVIDE) {
			// set required shift state based on current numlock state
			if ((outMask & KeyModifierNumLock) == 0) {
				if ((m_mask & KeyModifierNumLock) == 0) {
					LOG((CLOG_DEBUG2 "turn on num lock for keypad key"));
					outMask |= KeyModifierNumLock;
				}
				else {
					LOG((CLOG_DEBUG2 "turn on shift for keypad key"));
					outMask |= KeyModifierShift;
				}
			}
		}

		// check for left tab
		else if (id == kKeyLeftTab) {
			outMask |= KeyModifierShift;
		}
	}
	LOG((CLOG_DEBUG2 "KeyID %d to virtual key %d mask 0x%04x", id, virtualKey, outMask));

	// a list of modifier key info
	class CModifierInfo {
	public:
		KeyModifierMask	m_mask;
		UINT			m_virtualKey;
		UINT			m_virtualKey2;
		bool			m_isToggle;
	};
	static const CModifierInfo s_modifier[] = {
		{ KeyModifierShift,		VK_LSHIFT,			VK_RSHIFT,			false },
		{ KeyModifierControl,	VK_LCONTROL,		VK_RCONTROL | 0x100,false },
		{ KeyModifierAlt,		VK_LMENU,			VK_RMENU | 0x100,	false },
		// note -- no keys for KeyModifierMeta
		{ KeyModifierSuper,		VK_LWIN | 0x100,	VK_RWIN | 0x100,	false },
		{ KeyModifierCapsLock,	VK_CAPITAL,			0,					true },
		{ KeyModifierNumLock,	VK_NUMLOCK | 0x100,	0,					true },
		{ KeyModifierScrollLock,VK_SCROLL,			0,					true }
	};
	static const unsigned int s_numModifiers =
								sizeof(s_modifier) / sizeof(s_modifier[0]);

	// note if the key is a modifier
	unsigned int modifierIndex;
	switch (virtualKey) {
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
		modifierIndex = 0;
		break;

	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
		modifierIndex = 1;
		break;

	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
		modifierIndex = 2;
		break;

	case VK_LWIN:
	case VK_RWIN:
		modifierIndex = 3;
		break;

	case VK_CAPITAL:
		modifierIndex = 4;
		break;

	case VK_NUMLOCK:
		modifierIndex = 5;
		break;

	case VK_SCROLL:
		modifierIndex = 6;
		break;

	default:
		modifierIndex = s_numModifiers;
		break;
	}
	const bool isModifier = (modifierIndex != s_numModifiers);

	// add the key events required to get to the modifier state
	// necessary to generate an event yielding id.  also save the
	// key events required to restore the state.  if the key is
	// a modifier key then skip this because modifiers should not
	// modify modifiers.
	Keystrokes undo;
	Keystroke keystroke;
	if (outMask != m_mask && !isModifier) {
		for (unsigned int i = 0; i < s_numModifiers; ++i) {
			KeyModifierMask bit = s_modifier[i].m_mask;
			if ((outMask & bit) != (m_mask & bit)) {
				if ((outMask & bit) != 0) {
					// modifier is not active but should be.  if the
					// modifier is a toggle then toggle it on with a
					// press/release, otherwise activate it with a
					// press.
					keystroke.m_virtualKey = s_modifier[i].m_virtualKey;
					keystroke.m_press      = true;
					keystroke.m_repeat     = false;
					keys.push_back(keystroke);
					if (s_modifier[i].m_isToggle) {
						keystroke.m_press = false;
						keys.push_back(keystroke);
						undo.push_back(keystroke);
						keystroke.m_press = true;
						undo.push_back(keystroke);
					}
					else {
						keystroke.m_press = false;
						undo.push_back(keystroke);
					}
				}

				else {
					// modifier is active but should not be.  if the
					// modifier is a toggle then toggle it off with a
					// press/release, otherwise deactivate it with a
					// release.  we must check each keycode for the
					// modifier if not a toggle.
					if (s_modifier[i].m_isToggle) {
						keystroke.m_virtualKey = s_modifier[i].m_virtualKey;
						keystroke.m_press      = true;
						keystroke.m_repeat     = false;
						keys.push_back(keystroke);
						keystroke.m_press      = false;
						keys.push_back(keystroke);
						undo.push_back(keystroke);
						keystroke.m_press      = true;
						undo.push_back(keystroke);
					}
					else {
						UINT key = s_modifier[i].m_virtualKey;
						if ((m_keys[key & 0xff] & 0x80) != 0) {
							keystroke.m_virtualKey = key;
							keystroke.m_press      = false;
							keystroke.m_repeat     = false;
							keys.push_back(keystroke);
							keystroke.m_press      = true;
							undo.push_back(keystroke);
						}
						key = s_modifier[i].m_virtualKey2;
						if (key != 0 && (m_keys[key & 0xff] & 0x80) != 0) {
							keystroke.m_virtualKey = key;
							keystroke.m_press      = false;
							keystroke.m_repeat     = false;
							keys.push_back(keystroke);
							keystroke.m_press      = true;
							undo.push_back(keystroke);
						}
					}
				}
			}
		}
	}

	// add the key event
	keystroke.m_virtualKey = virtualKey;
	if (isExtended) {
		keystroke.m_virtualKey |= 0x100;
	}
	switch (action) {
	case kPress:
		keystroke.m_press      = true;
		keystroke.m_repeat     = false;
		keys.push_back(keystroke);
		break;

	case kRelease:
		keystroke.m_press      = false;
		keystroke.m_repeat     = false;
		keys.push_back(keystroke);
		break;

	case kRepeat:
		keystroke.m_press      = true;
		keystroke.m_repeat     = true;
		keys.push_back(keystroke);
		break;
	}

	// add key events to restore the modifier state.  apply events in
	// the reverse order that they're stored in undo.
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	// if the key is a modifier key then compute the modifier mask after
	// this key is pressed.
	mask = m_mask;
	if (isModifier && action != kRepeat) {
		// toggle keys modify the state on release.  other keys set
		// the bit on press and clear the bit on release.
		const CModifierInfo& modifier = s_modifier[modifierIndex];
		if (modifier.m_isToggle) {
			if (action == kRelease) {
				mask ^= modifier.m_mask;
			}
		}
		else if (action == kPress) {
			mask |= modifier.m_mask;
		}
		else {
			// can't reset bit until all keys that set it are released.
			// scan those keys to see if any are pressed.
			bool down = false;
			if (virtualKey != (modifier.m_virtualKey & 0xff) &&
				(m_keys[modifier.m_virtualKey & 0xff] & 0x80) != 0) {
				down = true;
			}
			if (modifier.m_virtualKey2 != 0 &&
				virtualKey != (modifier.m_virtualKey2 & 0xff) &&
				(m_keys[modifier.m_virtualKey2 & 0xff] & 0x80) != 0) {
				down = true;
			}
			if (!down)
				mask &= ~modifier.m_mask;
		}
	}

	LOG((CLOG_DEBUG2 "previous modifiers 0x%04x, final modifiers 0x%04x", m_mask, mask));
	return mask;
}

void
CMSWindowsSecondaryScreen::doKeystrokes(const Keystrokes& keys, SInt32 count)
{
	// do nothing if no keys or no repeats
	if (count < 1 || keys.empty()) {
		return;
	}

	// generate key events
	for (Keystrokes::const_iterator k = keys.begin(); k != keys.end(); ) {
		if (k->m_repeat) {
			// repeat from here up to but not including the next key
			// with m_repeat == false count times.
			Keystrokes::const_iterator start = k;
			for (; count > 0; --count) {
				// send repeating events
				for (k = start; k != keys.end() && k->m_repeat; ++k) {
					sendKeyEvent(k->m_virtualKey, k->m_press);
				}
			}

			// note -- k is now on the first non-repeat key after the
			// repeat keys, exactly where we'd like to continue from.
		}
		else {
			// send event
			sendKeyEvent(k->m_virtualKey, k->m_press);

			// next key
			++k;
		}
	}
}

void
CMSWindowsSecondaryScreen::releaseKeys()
{
	// release keys that we've synthesized a press for and only those
	// keys.  we don't want to synthesize a release on a key the user
	// is still physically pressing.

	CLock lock(&m_mutex);

	m_screen->syncDesktop();

	// release left/right modifier keys first.  if the platform doesn't
	// support them then they won't be set and the non-side-distinuishing
	// key will retain its state.  if the platform does support them then
	// the non-side-distinguishing will be reset.
	if ((m_fakeKeys[VK_LSHIFT] & 0x80) != 0) {
		sendKeyEvent(VK_LSHIFT, false);
		m_fakeKeys[VK_SHIFT]    = 0;
		m_fakeKeys[VK_LSHIFT]   = 0;
	}
	if ((m_fakeKeys[VK_RSHIFT] & 0x80) != 0) {
		sendKeyEvent(VK_RSHIFT, false);
		m_fakeKeys[VK_SHIFT]    = 0;
		m_fakeKeys[VK_RSHIFT]   = 0;
	}
	if ((m_fakeKeys[VK_LCONTROL] & 0x80) != 0) {
		sendKeyEvent(VK_LCONTROL, false);
		m_fakeKeys[VK_CONTROL]  = 0;
		m_fakeKeys[VK_LCONTROL] = 0;
	}
	if ((m_fakeKeys[VK_RCONTROL] & 0x80) != 0) {
		sendKeyEvent(VK_RCONTROL, false);
		m_fakeKeys[VK_CONTROL]  = 0;
		m_fakeKeys[VK_RCONTROL] = 0;
	}
	if ((m_fakeKeys[VK_LMENU] & 0x80) != 0) {
		sendKeyEvent(VK_LMENU, false);
		m_fakeKeys[VK_MENU]     = 0;
		m_fakeKeys[VK_LMENU]    = 0;
	}
	if ((m_fakeKeys[VK_RMENU] & 0x80) != 0) {
		sendKeyEvent(VK_RMENU, false);
		m_fakeKeys[VK_MENU]     = 0;
		m_fakeKeys[VK_RMENU]    = 0;
	}

	// now check all the other keys
	for (UInt32 i = 0; i < sizeof(m_fakeKeys) / sizeof(m_fakeKeys[0]); ++i) {
		if ((m_fakeKeys[i] & 0x80) != 0) {
			sendKeyEvent(i, false);
			m_fakeKeys[i] = 0;
		}
	}
}

void
CMSWindowsSecondaryScreen::toggleKey(UINT virtualKey, KeyModifierMask mask)
{
	// send key events to simulate a press and release
	sendKeyEvent(virtualKey, true);
	sendKeyEvent(virtualKey, false);

	// toggle shadow state
	m_mask                        ^= mask;
	m_keys[virtualKey & 0xff]     ^= 0x01;
	m_fakeKeys[virtualKey & 0xff] ^= 0x01;
}

UINT
CMSWindowsSecondaryScreen::virtualKeyToScanCode(UINT& virtualKey) const
{
	// try mapping given virtual key
	UINT code = MapVirtualKey(virtualKey & 0xff, 0);
	if (code != 0) {
		return code;
	}

	// no dice.  if the virtual key distinguishes between left/right
	// then try the one that doesn't distinguish sides.  windows (or
	// keyboard drivers) are inconsistent in their treatment of these
	// virtual keys.  the following behaviors have been observed:
	//
	//  win2k (gateway desktop):
	//      MapVirtualKey(vk, 0):
	//        VK_SHIFT == VK_LSHIFT != VK_RSHIFT
	//        VK_CONTROL == VK_LCONTROL == VK_RCONTROL
	//        VK_MENU == VK_LMENU == VK_RMENU
	//      MapVirtualKey(sc, 3):
	//        VK_LSHIFT and VK_RSHIFT mapped independently
	//        VK_LCONTROL is mapped but not VK_RCONTROL
	//        VK_LMENU is mapped but not VK_RMENU
	//
	//  win me (sony vaio laptop):
	//      MapVirtualKey(vk, 0):
	//        VK_SHIFT mapped;  VK_LSHIFT, VK_RSHIFT not mapped
	//        VK_CONTROL mapped;  VK_LCONTROL, VK_RCONTROL not mapped
	//        VK_MENU mapped;  VK_LMENU, VK_RMENU not mapped
	//      MapVirtualKey(sc, 3):
	//        all scan codes unmapped (function apparently unimplemented)
	switch (virtualKey & 0xff) {
	case VK_LSHIFT:
	case VK_RSHIFT:
		virtualKey = VK_SHIFT;
		return MapVirtualKey(VK_SHIFT, 0);

	case VK_LCONTROL:
	case VK_RCONTROL:
		virtualKey = VK_CONTROL;
		return MapVirtualKey(VK_CONTROL, 0);

	case VK_LMENU:
	case VK_RMENU:
		virtualKey = VK_MENU;
		return MapVirtualKey(VK_MENU, 0);

	default:
		return 0;
	}
}

bool
CMSWindowsSecondaryScreen::isExtendedKey(UINT virtualKey) const
{
	// see if we've already encoded the extended flag
	if ((virtualKey & 0x100) != 0) {
		return true;
	}

	// check known virtual keys
	switch (virtualKey & 0xff) {
	case VK_NUMLOCK:
	case VK_RCONTROL:
	case VK_RMENU:
	case VK_LWIN:
	case VK_RWIN:
	case VK_APPS:
		return true;

	default:
		return false;
	}
}

void
CMSWindowsSecondaryScreen::sendKeyEvent(UINT virtualKey, bool press)
{
	DWORD flags = 0;
	if (isExtendedKey(virtualKey)) {
		flags |= KEYEVENTF_EXTENDEDKEY;
	}
	if (!press) {
		flags |= KEYEVENTF_KEYUP;
	}
	const UINT code = virtualKeyToScanCode(virtualKey);
	keybd_event(static_cast<BYTE>(virtualKey & 0xff),
								static_cast<BYTE>(code), flags, 0);
	LOG((CLOG_DEBUG1 "send key %d, 0x%04x, %s%s", virtualKey & 0xff, code, ((flags & KEYEVENTF_KEYUP) ? "release" : "press"), ((flags & KEYEVENTF_EXTENDEDKEY) ? " extended" : "")));
}
