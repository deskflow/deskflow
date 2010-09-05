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
#include "CArch.h"
#include "CArchMiscWindows.h"
#include <cstring>

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

//
// map virtual key id to a name
//

static const char* g_buttonToName[] = {
	"button 0",
	"Left Button",
	"Middle Button",
	"Right Button",
	"X Button 1",
	"X Button 2"
};
static const char* g_vkToName[] = {
	"vk 0x00",
	"Left Button",
	"Right Button",
	"VK_CANCEL",
	"Middle Button",
	"vk 0x05",
	"vk 0x06",
	"vk 0x07",
	"VK_BACK",
	"VK_TAB",
	"vk 0x0a",
	"vk 0x0b",
	"VK_CLEAR",
	"VK_RETURN",
	"vk 0x0e",
	"vk 0x0f",
	"VK_SHIFT",
	"VK_CONTROL",
	"VK_MENU",
	"VK_PAUSE",
	"VK_CAPITAL",
	"VK_KANA",
	"vk 0x16",
	"VK_JUNJA",
	"VK_FINAL",
	"VK_KANJI",
	"vk 0x1a",
	"VK_ESCAPE",
	"VK_CONVERT",
	"VK_NONCONVERT",
	"VK_ACCEPT",
	"VK_MODECHANGE",
	"VK_SPACE",
	"VK_PRIOR",
	"VK_NEXT",
	"VK_END",
	"VK_HOME",
	"VK_LEFT",
	"VK_UP",
	"VK_RIGHT",
	"VK_DOWN",
	"VK_SELECT",
	"VK_PRINT",
	"VK_EXECUTE",
	"VK_SNAPSHOT",
	"VK_INSERT",
	"VK_DELETE",
	"VK_HELP",
	"VK_0",
	"VK_1",
	"VK_2",
	"VK_3",
	"VK_4",
	"VK_5",
	"VK_6",
	"VK_7",
	"VK_8",
	"VK_9",
	"vk 0x3a",
	"vk 0x3b",
	"vk 0x3c",
	"vk 0x3d",
	"vk 0x3e",
	"vk 0x3f",
	"vk 0x40",
	"VK_A",
	"VK_B",
	"VK_C",
	"VK_D",
	"VK_E",
	"VK_F",
	"VK_G",
	"VK_H",
	"VK_I",
	"VK_J",
	"VK_K",
	"VK_L",
	"VK_M",
	"VK_N",
	"VK_O",
	"VK_P",
	"VK_Q",
	"VK_R",
	"VK_S",
	"VK_T",
	"VK_U",
	"VK_V",
	"VK_W",
	"VK_X",
	"VK_Y",
	"VK_Z",
	"VK_LWIN",
	"VK_RWIN",
	"VK_APPS",
	"vk 0x5e",
	"vk 0x5f",
	"VK_NUMPAD0",
	"VK_NUMPAD1",
	"VK_NUMPAD2",
	"VK_NUMPAD3",
	"VK_NUMPAD4",
	"VK_NUMPAD5",
	"VK_NUMPAD6",
	"VK_NUMPAD7",
	"VK_NUMPAD8",
	"VK_NUMPAD9",
	"VK_MULTIPLY",
	"VK_ADD",
	"VK_SEPARATOR",
	"VK_SUBTRACT",
	"VK_DECIMAL",
	"VK_DIVIDE",
	"VK_F1",
	"VK_F2",
	"VK_F3",
	"VK_F4",
	"VK_F5",
	"VK_F6",
	"VK_F7",
	"VK_F8",
	"VK_F9",
	"VK_F10",
	"VK_F11",
	"VK_F12",
	"VK_F13",
	"VK_F14",
	"VK_F15",
	"VK_F16",
	"VK_F17",
	"VK_F18",
	"VK_F19",
	"VK_F20",
	"VK_F21",
	"VK_F22",
	"VK_F23",
	"VK_F24",
	"vk 0x88",
	"vk 0x89",
	"vk 0x8a",
	"vk 0x8b",
	"vk 0x8c",
	"vk 0x8d",
	"vk 0x8e",
	"vk 0x8f",
	"VK_NUMLOCK",
	"VK_SCROLL",
	"vk 0x92",
	"vk 0x93",
	"vk 0x94",
	"vk 0x95",
	"vk 0x96",
	"vk 0x97",
	"vk 0x98",
	"vk 0x99",
	"vk 0x9a",
	"vk 0x9b",
	"vk 0x9c",
	"vk 0x9d",
	"vk 0x9e",
	"vk 0x9f",
	"VK_LSHIFT",
	"VK_RSHIFT",
	"VK_LCONTROL",
	"VK_RCONTROL",
	"VK_LMENU",
	"VK_RMENU",
	"VK_BROWSER_BACK",
	"VK_BROWSER_FORWARD",
	"VK_BROWSER_REFRESH",
	"VK_BROWSER_STOP",
	"VK_BROWSER_SEARCH",
	"VK_BROWSER_FAVORITES",
	"VK_BROWSER_HOME",
	"VK_VOLUME_MUTE",
	"VK_VOLUME_DOWN",
	"VK_VOLUME_UP",
	"VK_MEDIA_NEXT_TRACK",
	"VK_MEDIA_PREV_TRACK",
	"VK_MEDIA_STOP",
	"VK_MEDIA_PLAY_PAUSE",
	"VK_LAUNCH_MAIL",
	"VK_LAUNCH_MEDIA_SELECT",
	"VK_LAUNCH_APP1",
	"VK_LAUNCH_APP2",
	"vk 0xb8",
	"vk 0xb9",
	"vk 0xba",
	"vk 0xbb",
	"vk 0xbc",
	"vk 0xbd",
	"vk 0xbe",
	"vk 0xbf",
	"vk 0xc0",
	"vk 0xc1",
	"vk 0xc2",
	"vk 0xc3",
	"vk 0xc4",
	"vk 0xc5",
	"vk 0xc6",
	"vk 0xc7",
	"vk 0xc8",
	"vk 0xc9",
	"vk 0xca",
	"vk 0xcb",
	"vk 0xcc",
	"vk 0xcd",
	"vk 0xce",
	"vk 0xcf",
	"vk 0xd0",
	"vk 0xd1",
	"vk 0xd2",
	"vk 0xd3",
	"vk 0xd4",
	"vk 0xd5",
	"vk 0xd6",
	"vk 0xd7",
	"vk 0xd8",
	"vk 0xd9",
	"vk 0xda",
	"vk 0xdb",
	"vk 0xdc",
	"vk 0xdd",
	"vk 0xde",
	"vk 0xdf",
	"vk 0xe0",
	"vk 0xe1",
	"vk 0xe2",
	"vk 0xe3",
	"vk 0xe4",
	"VK_PROCESSKEY",
	"vk 0xe6",
	"vk 0xe7",
	"vk 0xe8",
	"vk 0xe9",
	"vk 0xea",
	"vk 0xeb",
	"vk 0xec",
	"vk 0xed",
	"vk 0xee",
	"vk 0xef",
	"vk 0xf0",
	"vk 0xf1",
	"vk 0xf2",
	"vk 0xf3",
	"vk 0xf4",
	"vk 0xf5",
	"VK_ATTN",
	"VK_CRSEL",
	"VK_EXSEL",
	"VK_EREOF",
	"VK_PLAY",
	"VK_ZOOM",
	"VK_NONAME",
	"VK_PA1",
	"VK_OEM_CLEAR",
	"vk 0xff"
};

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
	m_mark(0),
	m_markReceived(0),
	m_lowLevel(false),
	m_cursorThread(0)
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

UInt32
CMSWindowsPrimaryScreen::addOneShotTimer(double timeout)
{
	return m_screen->addOneShotTimer(timeout);
}

KeyModifierMask
CMSWindowsPrimaryScreen::getToggleMask() const
{
	KeyModifierMask mask = 0;

	// get key state from our shadow state
	if ((m_keys[VK_CAPITAL] & 0x01) != 0)
		mask |= KeyModifierCapsLock;
	if ((m_keys[VK_NUMLOCK] & 0x01) != 0)
		mask |= KeyModifierNumLock;
	if ((m_keys[VK_SCROLL] & 0x01) != 0)
		mask |= KeyModifierScrollLock;

	return mask;
}

bool
CMSWindowsPrimaryScreen::isLockedToScreen() const
{
	// use shadow keyboard state in m_keys and m_buttons
	for (UInt32 i = 0; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
		if ((m_buttons[i] & 0x80) != 0) {
			LOG((CLOG_DEBUG "locked by \"%s\"", g_buttonToName[i]));
			return true;
		}
	}
	for (UInt32 i = 0; i < sizeof(m_keys) / sizeof(m_keys[0]); ++i) {
		if ((m_keys[i] & 0x80) != 0) {
			LOG((CLOG_DEBUG "locked by \"%s\"", g_vkToName[i]));
			return true;
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

	const MSG* msg = &event->m_msg;

	// check if windows key is up but we think it's down.  if so then
	// synthesize a key release for it.  we have to do this because
	// if the user presses and releases a windows key without pressing
	// any other key when its down then windows will eat the key
	// release.  if we don't detect that an synthesize the release
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
		if ((m_keys[VK_LWIN] & 0x80) != 0 &&
			(GetAsyncKeyState(VK_LWIN) & 0x8000) == 0 &&
			!(msg->message == SYNERGY_MSG_KEY && msg->wParam == VK_LWIN)) {
			// compute appropriate parameters for fake event
			WPARAM wParam = VK_LWIN;
			LPARAM lParam = 0xc1000000;
			lParam |= (0x00ff0000 & (MapVirtualKey(wParam, 0) << 24));

			// process as if it were a key up
			bool altgr;
			KeyModifierMask mask;
			KeyButton button = static_cast<KeyButton>(
								(lParam & 0x00ff0000u) >> 16);
			const KeyID key = mapKey(wParam, lParam, &mask, &altgr);
			LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
			m_receiver->onKeyUp(key, mask, button);
			updateKey(wParam, false);
		}
		if ((m_keys[VK_RWIN] & 0x80) != 0 &&
			(GetAsyncKeyState(VK_RWIN) & 0x8000) == 0 &&
			!(msg->message == SYNERGY_MSG_KEY && msg->wParam == VK_RWIN)) {
			// compute appropriate parameters for fake event
			WPARAM wParam = VK_RWIN;
			LPARAM lParam = 0xc1000000;
			lParam |= (0x00ff0000 & (MapVirtualKey(wParam, 0) << 24));

			// process as if it were a key up
			bool altgr;
			KeyModifierMask mask;
			KeyButton button = static_cast<KeyButton>(
								(lParam & 0x00ff0000u) >> 16);
			const KeyID key = mapKey(wParam, lParam, &mask, &altgr);
			LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
			m_receiver->onKeyUp(key, mask, button);
			updateKey(wParam, false);
		}
	}

	// handle event
	switch (msg->message) {
	case SYNERGY_MSG_MARK:
		m_markReceived = msg->wParam;
		return true;

	case SYNERGY_MSG_KEY:
		// ignore message if posted prior to last mark change
		if (!ignore()) {
			WPARAM wParam = msg->wParam;
			LPARAM lParam = msg->lParam;

			// check for ctrl+alt+del emulation
			if ((wParam == VK_PAUSE || wParam == VK_CANCEL) &&
				(m_keys[VK_CONTROL] & 0x80) != 0 &&
				(m_keys[VK_MENU]    & 0x80) != 0) {
				LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
				wParam  = VK_DELETE;
				lParam &= 0xffff0000;
				lParam |= 0x00000001;
			}

			// process key normally
			bool altgr;
			KeyModifierMask mask;
			const KeyID key = mapKey(wParam, lParam, &mask, &altgr);
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
					// alt back the way there were after we simulate
					// the key.
					bool ctrlL = ((m_keys[VK_LCONTROL] & 0x80) != 0);
					bool ctrlR = ((m_keys[VK_RCONTROL] & 0x80) != 0);
					bool altL  = ((m_keys[VK_LMENU]    & 0x80) != 0);
					bool altR  = ((m_keys[VK_RMENU]    & 0x80) != 0);
					if (altgr) {
						KeyID key;
						KeyButton button;
						KeyModifierMask mask2 = (mask &
											~(KeyModifierControl |
											KeyModifierAlt |
											KeyModifierModeSwitch));
						if (ctrlL) {
							key    = kKeyControl_L;
							button = mapKeyToScanCode(VK_LCONTROL, VK_CONTROL);
							LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyUp(key, mask2, button);
						}
						if (ctrlR) {
							key    = kKeyControl_R;
							button = mapKeyToScanCode(VK_RCONTROL, VK_CONTROL);
							LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyUp(key, mask2, button);
						}
						if (altL) {
							key    = kKeyAlt_L;
							button = mapKeyToScanCode(VK_LMENU, VK_MENU);
							LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyUp(key, mask2, button);
						}
						if (altR) {
							key    = kKeyAlt_R;
							button = mapKeyToScanCode(VK_RMENU, VK_MENU);
							LOG((CLOG_DEBUG1 "event: fake key release key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyUp(key, mask2, button);
						}
					}

					// send key
					const bool wasDown = ((lParam & 0x40000000) != 0);
					SInt32 repeat      = (SInt32)(lParam & 0xffff);
					if (!wasDown) {
						LOG((CLOG_DEBUG1 "event: key press key=%d mask=0x%04x button=0x%04x", key, mask, button));
						m_receiver->onKeyDown(key, mask, button);
						if (repeat > 0) {
							--repeat;
						}
					}
					if (repeat >= 1) {
						LOG((CLOG_DEBUG1 "event: key repeat key=%d mask=0x%04x count=%d button=0x%04x", key, mask, repeat, button));
						m_receiver->onKeyRepeat(key, mask, repeat, button);
					}

					// restore ctrl and alt state
					if (altgr) {
						KeyID key;
						KeyButton button;
						KeyModifierMask mask2 = (mask &
											~(KeyModifierControl |
											KeyModifierAlt |
											KeyModifierModeSwitch));
						if (ctrlL) {
							key    = kKeyControl_L;
							button = mapKeyToScanCode(VK_LCONTROL, VK_CONTROL);
							LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyDown(key, mask2, button);
							mask2 |= KeyModifierControl;
						}
						if (ctrlR) {
							key    = kKeyControl_R;
							button = mapKeyToScanCode(VK_RCONTROL, VK_CONTROL);
							LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyDown(key, mask2, button);
							mask2 |= KeyModifierControl;
						}
						if (altL) {
							key    = kKeyAlt_L;
							button = mapKeyToScanCode(VK_LMENU, VK_MENU);
							LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyDown(key, mask2, button);
							mask2 |= KeyModifierAlt;
						}
						if (altR) {
							key    = kKeyAlt_R;
							button = mapKeyToScanCode(VK_RMENU, VK_MENU);
							LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask2, button));
							m_receiver->onKeyDown(key, mask2, button);
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
					if (m_is95Family && !isModifier(msg->wParam) &&
						(m_keys[msg->wParam] & 0x80) == 0) {
						LOG((CLOG_DEBUG1 "event: fake key press key=%d mask=0x%04x button=0x%04x", key, mask, button));
						m_receiver->onKeyDown(key, mask, button);
						updateKey(msg->wParam, true);
					}

					// do key up
					LOG((CLOG_DEBUG1 "event: key release key=%d mask=0x%04x button=0x%04x", key, mask, button));
					m_receiver->onKeyUp(key, mask, button);
				}
			}
			else {
				LOG((CLOG_DEBUG2 "event: cannot map key wParam=%d lParam=0x%08x", msg->wParam, msg->lParam));
			}
		}

		// keep our shadow key state up to date
		updateKey(msg->wParam, ((msg->lParam & 0x80000000) == 0));

		return true;

	case SYNERGY_MSG_MOUSE_BUTTON: {
		// get which button
		bool pressed = false;
		const ButtonID button = mapButton(msg->wParam, msg->lParam);

		// ignore message if posted prior to last mark change
		if (!ignore()) {
			switch (msg->wParam) {
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
					m_receiver->onMouseDown(button);
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
					m_receiver->onMouseUp(button);
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
				if (x != 0 || y != 0) {
					m_receiver->onMouseMovePrimary(m_x, m_y);
				}
			}
			else {
				// motion on secondary screen.  warp mouse back to
				// center.
				if (x != 0 || y != 0) {
					// back to center
					warpCursorNoFlush(m_xCenter, m_yCenter);

					// examine the motion.  if it's about the distance
					// from the center of the screen to an edge then
					// it's probably a bogus motion that we want to
					// ignore (see warpCursorNoFlush() for a further
					// description).
					static SInt32 bogusZoneSize = 10;
					SInt32 x0, y0, w0, h0;
					m_screen->getShape(x0, y0, w0, h0);
					if (-x + bogusZoneSize > m_xCenter - x0 ||
						 x + bogusZoneSize > x0 + w0 - m_xCenter ||
						-y + bogusZoneSize > m_yCenter - y0 ||
						 y + bogusZoneSize > y0 + h0 - m_yCenter) {
						LOG((CLOG_DEBUG "dropped bogus motion %+d,%+d", x, y));
					}
					else {
						// send motion
						m_receiver->onMouseMoveSecondary(x, y);
					}
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

void
CMSWindowsPrimaryScreen::onOneShotTimerExpired(UInt32 id)
{
	m_receiver->onOneShotTimerExpired(id);
}

SInt32
CMSWindowsPrimaryScreen::getJumpZoneSize() const
{
	return 1;
}

void
CMSWindowsPrimaryScreen::postCreateWindow(HWND)
{
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

	if (!isActive()) {
		// watch jump zones
		m_setRelay(false);

		// all messages prior to now are invalid
		nextMark();
	}
}

void
CMSWindowsPrimaryScreen::preDestroyWindow(HWND)
{
	// uninstall hooks
	m_uninstall();
}

void
CMSWindowsPrimaryScreen::onAccessibleDesktop()
{
	// get the current keyboard state
	updateKeys();
}

void
CMSWindowsPrimaryScreen::onPreMainLoop()
{
	// must call mainLoop() from same thread as open()
	assert(m_threadID == GetCurrentThreadId());
}

void
CMSWindowsPrimaryScreen::onPreOpen()
{
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
	// show cursor if we hid it
	if (m_cursorThread != 0) {
		if (m_threadID != m_cursorThread) {
			AttachThreadInput(m_threadID, m_cursorThread, TRUE);
		}
		ShowCursor(TRUE);
		if (m_threadID != m_cursorThread) {
			AttachThreadInput(m_threadID, m_cursorThread, FALSE);
		}
		m_cursorThread = 0;
	}

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

		// hide the cursor if using low level hooks
		if (m_lowLevel) {
			HWND hwnd      = GetForegroundWindow();
			m_cursorThread = GetWindowThreadProcessId(hwnd, NULL);
			if (m_threadID != m_cursorThread) {
				AttachThreadInput(m_threadID, m_cursorThread, TRUE);
			}
			ShowCursor(FALSE);
			if (m_threadID != m_cursorThread) {
				AttachThreadInput(m_threadID, m_cursorThread, FALSE);
			}
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

	// we don't ever want our window to activate
	EnableWindow(m_window, FALSE);
}

void
CMSWindowsPrimaryScreen::destroyWindow()
{
	// close the desktop and the window
	m_screen->closeDesktop();
}

bool
CMSWindowsPrimaryScreen::showWindow()
{
	// we don't need a window to capture input but we need a window
	// to hide the cursor when using low-level hooks.  do not try to
	// take the activation;  we want the currently active window to
	// stay active.
	if (m_lowLevel) {
		SetWindowPos(m_window, HWND_TOPMOST, m_xCenter, m_yCenter, 1, 1,
							SWP_NOACTIVATE);
		ShowWindow(m_window, SW_SHOWNA);
	}
	return true;
}

void
CMSWindowsPrimaryScreen::hideWindow()
{
	// hide our window
	if (m_lowLevel) {
		ShowWindow(m_window, SW_HIDE);
	}
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
	/* 0x20 */ kKeyNone,		kKeyNone,		// VK_SPACE
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
	/* 0xa6 */ kKeyNone,		kKeyWWWBack,	// VK_BROWSER_BACK
	/* 0xa7 */ kKeyNone,		kKeyWWWForward,	// VK_BROWSER_FORWARD
	/* 0xa8 */ kKeyNone,		kKeyWWWRefresh,	// VK_BROWSER_REFRESH
	/* 0xa9 */ kKeyNone,		kKeyWWWStop,	// VK_BROWSER_STOP
	/* 0xaa */ kKeyNone,		kKeyWWWSearch,	// VK_BROWSER_SEARCH
	/* 0xab */ kKeyNone,		kKeyWWWFavorites,	// VK_BROWSER_FAVORITES
	/* 0xac */ kKeyNone,		kKeyWWWHome,	// VK_BROWSER_HOME
	/* 0xad */ kKeyNone,		kKeyAudioMute,	// VK_VOLUME_MUTE
	/* 0xae */ kKeyNone,		kKeyAudioDown,	// VK_VOLUME_DOWN
	/* 0xaf */ kKeyNone,		kKeyAudioUp,	// VK_VOLUME_UP
	/* 0xb0 */ kKeyNone,		kKeyAudioNext,	// VK_MEDIA_NEXT_TRACK
	/* 0xb1 */ kKeyNone,		kKeyAudioPrev,	// VK_MEDIA_PREV_TRACK
	/* 0xb2 */ kKeyNone,		kKeyAudioStop,	// VK_MEDIA_STOP
	/* 0xb3 */ kKeyNone,		kKeyAudioPlay,	// VK_MEDIA_PLAY_PAUSE
	/* 0xb4 */ kKeyNone,		kKeyAppMail,	// VK_LAUNCH_MAIL
	/* 0xb5 */ kKeyNone,		kKeyAppMedia,	// VK_LAUNCH_MEDIA_SELECT
	/* 0xb6 */ kKeyNone,		kKeyAppUser1,	// VK_LAUNCH_APP1
	/* 0xb7 */ kKeyNone,		kKeyAppUser2,	// VK_LAUNCH_APP2
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
	KeyModifierMask* maskOut,
	bool* altgr)
{
	// note:  known microsoft bugs
	//  Q72583 -- MapVirtualKey() maps keypad keys incorrectly
	//    95,98: num pad vk code -> invalid scan code
	//    95,98,NT4: num pad scan code -> bad vk code except
	//      SEPARATOR, MULTIPLY, SUBTRACT, ADD

	assert(maskOut != NULL);
	assert(altgr   != NULL);

	// get the scan code and the extended keyboard flag
	UINT scanCode = static_cast<UINT>((info & 0x00ff0000u) >> 16);
	int extended  = ((info & 0x01000000) == 0) ? 0 : 1;
	LOG((CLOG_DEBUG1 "key vk=%d info=0x%08x ext=%d scan=%d", vkCode, info, extended, scanCode));

	// handle some keys via table lookup
	char c   = 0;
	KeyID id = g_virtualKey[vkCode][extended];
	if (id == kKeyNone) {
		// not in table

		// save the control state then clear it.  ToAscii() maps ctrl+letter
		// to the corresponding control code and ctrl+backspace to delete.
		// we don't want that translation so we clear the control modifier
		// state.  however, if we want to simulate AltGr (which is ctrl+alt)
		// then we must not clear it.
		BYTE lControl = m_keys[VK_LCONTROL];
		BYTE rControl = m_keys[VK_RCONTROL];
		BYTE control  = m_keys[VK_CONTROL];
		BYTE lMenu    = m_keys[VK_LMENU];
		BYTE menu     = m_keys[VK_MENU];
		if ((control & 0x80) == 0 || (menu & 0x80) == 0) {
			m_keys[VK_LCONTROL] = 0;
			m_keys[VK_RCONTROL] = 0;
			m_keys[VK_CONTROL]  = 0;
		}
		else {
			m_keys[VK_LCONTROL] = 0x80;
			m_keys[VK_CONTROL]  = 0x80;
			m_keys[VK_LMENU]    = 0x80;
			m_keys[VK_MENU]     = 0x80;
		}

		// convert to ascii
		WORD ascii;
		int result = ToAscii(vkCode, scanCode, m_keys, &ascii,
								((menu & 0x80) == 0) ? 0 : 1);

		// restore control state
		m_keys[VK_LCONTROL] = lControl;
		m_keys[VK_RCONTROL] = rControl;
		m_keys[VK_CONTROL]  = control;
		m_keys[VK_LMENU]    = lMenu;
		m_keys[VK_MENU]     = menu;

		// if result is less than zero then it was a dead key.  leave it
		// there.
		if (result < 0) {
			id = kKeyMultiKey;
		}

		// if result is 1 then the key was succesfully converted
		else if (result == 1) {
			c = static_cast<char>(ascii & 0xff);
			if (ascii >= 0x80) {
				// character is not really ASCII.  instead it's some
				// character in the current ANSI code page.  try to
				// convert that to a Unicode character.  if we fail
				// then use the single byte character as is.
				char src = c;
				wchar_t unicode;
				if (MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED,
											&src, 1, &unicode, 1) > 0) {
					id = static_cast<KeyID>(unicode);
				}
				else {
					id = static_cast<KeyID>(ascii & 0x00ff);
				}
			}
			else {
				id = static_cast<KeyID>(ascii & 0x00ff);
			}
		}

		// if result is 2 then a previous dead key could not be composed.
		else if (result == 2) {
			// if the two characters are the same and this is a key release
			// then this event is the dead key being released.  we put the
			// dead key back in that case, otherwise we discard both key
			// events because we can't compose the character.  alternatively
			// we could generate key events for both keys.
			if (((ascii & 0xff00) >> 8) != (ascii & 0x00ff) ||
				(info & 0x80000000) == 0) {
				// cannot compose key
				return kKeyNone;
			}

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
			id = kKeyMultiKey;
		}
	}

	// set mask
	*altgr = false;
	if (id != kKeyNone && id != kKeyMultiKey && c != 0) {
		// note if key requires AltGr.  VkKeyScan() can have a problem
		// with some characters.  there are two problems in particular.
		// first, typing a dead key then pressing space will cause
		// VkKeyScan() to return 0xffff.  second, certain characters
		// may map to multiple virtual keys and we might get the wrong
		// one.  if that happens then we might not get the right
		// modifier mask.  AltGr+9 on the french keyboard layout (^)
		// has this problem.  in the first case, we'll assume AltGr is
		// required (only because it solves the problems we've seen
		// so far).  in the second, we'll use whatever the keyboard
		// state says.
		WORD virtualKeyAndModifierState = VkKeyScan(c);
		if (virtualKeyAndModifierState == 0xffff) {
			// there is no mapping.  assume AltGr.
			LOG((CLOG_DEBUG1 "no VkKeyScan() mapping"));
			*altgr = true;
		}
		else if (LOBYTE(virtualKeyAndModifierState) != vkCode) {
			// we didn't get the key that was actually pressed
			LOG((CLOG_DEBUG1 "VkKeyScan() mismatch"));
			if ((m_keys[VK_CONTROL] & 0x80) != 0 &&
				(m_keys[VK_MENU] & 0x80) != 0) {
				*altgr = true;
			}
		}
		else {
			BYTE modifierState = HIBYTE(virtualKeyAndModifierState);
			if ((modifierState & 6) == 6) {
				// key requires ctrl and alt == AltGr
				*altgr = true;
			}
		}
	}

	// map modifier key
	KeyModifierMask mask = 0;
	if (((m_keys[VK_LSHIFT] |
		  m_keys[VK_RSHIFT] |
		  m_keys[VK_SHIFT]) & 0x80) != 0) {
		mask |= KeyModifierShift;
	}
	if (*altgr) {
		mask |= KeyModifierModeSwitch;
	}
	else {
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
	}
	if (((m_keys[VK_LWIN] |
		  m_keys[VK_RWIN]) & 0x80) != 0) {
		mask |= KeyModifierSuper;
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

	return id;
}

ButtonID
CMSWindowsPrimaryScreen::mapButton(WPARAM msg, LPARAM button) const
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

void
CMSWindowsPrimaryScreen::updateKeys()
{
	// not using GetKeyboardState() because that doesn't seem to give
	// up-to-date results.  i don't know why that is or why GetKeyState()
	// should give different results.

	// clear key and button state
	memset(m_keys, 0, sizeof(m_keys));
	memset(m_buttons, 0, sizeof(m_buttons));

	// we only care about the modifier key states.  other keys and the
	// mouse buttons should be up.
	// sometimes these seem to be out of date so, to avoid getting
	// locked to a screen unexpectedly, just assume the non-toggle
	// modifier keys are up.
/*
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
*/
	m_keys[VK_CAPITAL]  = static_cast<BYTE>(GetKeyState(VK_CAPITAL));
	m_keys[VK_NUMLOCK]  = static_cast<BYTE>(GetKeyState(VK_NUMLOCK));
	m_keys[VK_SCROLL]   = static_cast<BYTE>(GetKeyState(VK_SCROLL));
}

void
CMSWindowsPrimaryScreen::updateKey(UINT vkCode, bool press)
{
	if (press) {
		switch (vkCode) {
		case 0:
		case VK_LBUTTON:
		case VK_MBUTTON:
		case VK_RBUTTON:
			// ignore bogus key
			break;

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

		// special case:  we detect ctrl+alt+del being pressed on some
		// systems but we don't detect the release of those keys.  so
		// if ctrl, alt, and del are down then mark them up.
		if ((m_keys[VK_CONTROL] & 0x80) != 0 &&
			(m_keys[VK_MENU]    & 0x80) != 0 &&
			(m_keys[VK_DELETE]  & 0x80) != 0) {
			m_keys[VK_LCONTROL] &= ~0x80;
			m_keys[VK_RCONTROL] &= ~0x80;
			m_keys[VK_CONTROL]  &= ~0x80;
			m_keys[VK_LMENU]    &= ~0x80;
			m_keys[VK_RMENU]    &= ~0x80;
			m_keys[VK_MENU]     &= ~0x80;
			m_keys[VK_DELETE]   &= ~0x80;
		}
	}
	else {
		switch (vkCode) {
		case 0:
		case VK_LBUTTON:
		case VK_MBUTTON:
		case VK_RBUTTON:
			// ignore bogus key
			break;

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

bool
CMSWindowsPrimaryScreen::isModifier(UINT vkCode) const

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

KeyButton
CMSWindowsPrimaryScreen::mapKeyToScanCode(UINT vk1, UINT vk2) const
{
	KeyButton button = static_cast<KeyButton>(MapVirtualKey(vk1, 0));
	if (button == 0) {
		button = static_cast<KeyButton>(MapVirtualKey(vk2, 0));
	}
	return button;
}
