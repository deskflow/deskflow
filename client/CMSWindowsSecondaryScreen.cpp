#include "CMSWindowsSecondaryScreen.h"
#include "CMSWindowsClipboard.h"
#include "CClient.h"
#include "CClipboard.h"
#include "CThread.h"
#include "CLog.h"
#include <assert.h>

//
// CMSWindowsSecondaryScreen
//

CMSWindowsSecondaryScreen::CMSWindowsSecondaryScreen() :
								m_client(NULL),
								m_window(NULL),
								m_nextClipboardWindow(NULL)
{
	// do nothing
}

CMSWindowsSecondaryScreen::~CMSWindowsSecondaryScreen()
{
	assert(m_window == NULL);
}

static CString s_log;
static CString s_logMore;
static HWND s_debug = NULL;
static HWND s_debugLog = NULL;
static DWORD s_thread = 0;
static BOOL CALLBACK WINAPI debugProc(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		return TRUE;

	case WM_CLOSE:
		PostQuitMessage(0);
		return TRUE;

	case WM_APP:
		if (!s_logMore.empty()) {
			if (s_log.size() > 20000)
				s_log = s_logMore;
			else
				s_log += s_logMore;
			s_logMore = "";
			SendMessage(s_debugLog, WM_SETTEXT, FALSE, (LPARAM)(LPCTSTR)s_log.c_str());
			SendMessage(s_debugLog, EM_SETSEL, s_log.size(), s_log.size());
			SendMessage(s_debugLog, EM_SCROLLCARET, 0, 0);
		}
		return TRUE;
	}
	return FALSE;
}
static void debugOutput(const char* msg)
{
	s_logMore += msg;
	PostMessage(s_debug, WM_APP, 0, 0);
}

void					CMSWindowsSecondaryScreen::run()
{
CLog::setOutputter(&debugOutput);
	log((CLOG_INFO "entering event loop"));
	doRun();
	log((CLOG_INFO "exiting event loop"));
CLog::setOutputter(NULL);
}

void					CMSWindowsSecondaryScreen::stop()
{
	log((CLOG_INFO "requesting event loop stop"));
	doStop();
}

void					CMSWindowsSecondaryScreen::open(CClient* client)
{
	assert(m_client == NULL);
	assert(client   != NULL);

	log((CLOG_INFO "opening screen"));

	// set the client
	m_client = client;

	// open the display
	openDisplay();

	// update key state
	updateKeys();
	updateModifiers();

	// assume primary has all clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id)
		grabClipboard(id);
}

void					CMSWindowsSecondaryScreen::close()
{
	assert(m_client != NULL);

	log((CLOG_INFO "closing screen"));

	// close the display
	closeDisplay();

	// done with client
	m_client = NULL;
}

void					CMSWindowsSecondaryScreen::enter(
								SInt32 x, SInt32 y, KeyModifierMask mask)
{
	assert(m_window != NULL);

	log((CLOG_INFO "entering screen at %d,%d mask=%04x", x, y, mask));

	// warp to requested location
	SInt32 w, h;
	getScreenSize(&w, &h);
	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
								(DWORD)((65535.99 * x) / (w - 1)),
								(DWORD)((65535.99 * y) / (h - 1)),
								0, 0);

	// show cursor
	log((CLOG_INFO "show cursor"));
	ShowWindow(m_window, SW_HIDE);

	// update our keyboard state to reflect the local state
	updateKeys();
	updateModifiers();

	// toggle modifiers that don't match the desired state
	if ((mask & KeyModifierCapsLock)   != (m_mask & KeyModifierCapsLock)) {
		toggleKey(VK_CAPITAL, KeyModifierCapsLock);
	}
	if ((mask & KeyModifierNumLock)    != (m_mask & KeyModifierNumLock)) {
		toggleKey(VK_NUMLOCK, KeyModifierNumLock);
	}
	if ((mask & KeyModifierScrollLock) != (m_mask & KeyModifierScrollLock)) {
		toggleKey(VK_SCROLL, KeyModifierScrollLock);
	}
}

void					CMSWindowsSecondaryScreen::leave()
{
	assert(m_window != NULL);

	log((CLOG_INFO "leaving screen"));

	// move hider window under the mouse (rather than moving the mouse
	// somewhere else on the screen)
	POINT point;
	GetCursorPos(&point);
	MoveWindow(m_window, point.x, point.y, 1, 1, FALSE);

	// raise and show the hider window.  take activation.
	log((CLOG_INFO "hide cursor"));
	ShowWindow(m_window, SW_SHOWNORMAL);

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
		m_clipboardOwner = clipboardOwner;
		if (m_clipboardOwner != m_window) {
			m_client->onClipboardChanged(kClipboardClipboard);
			m_client->onClipboardChanged(kClipboardSelection);
		}
	}
}

void					CMSWindowsSecondaryScreen::keyDown(
								KeyID key, KeyModifierMask mask)
{
	Keystrokes keys;
	UINT virtualKey;

	// get the sequence of keys to simulate key press and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, true);
	if (keys.empty())
		return;

	// generate key events
	for (Keystrokes::const_iterator k = keys.begin(); k != keys.end(); ++k) {
		const UINT code = MapVirtualKey(k->first, 0);
		keybd_event(k->first, code, k->second ? 0 : KEYEVENTF_KEYUP, 0);
	}

	// note that key is now down
	m_keys[virtualKey] |= 0x80;
}

void					CMSWindowsSecondaryScreen::keyRepeat(
								KeyID key, KeyModifierMask mask, SInt32 count)
{
	Keystrokes keys;
	UINT virtualKey;

	// get the sequence of keys to simulate key release and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, true);
	if (keys.empty())
		return;

	// generate key events
// YYY -- need to know which code in Keystrokes should be repeated;
// then repeat only that key count times
	for (SInt32 i = 0; i < count; ++i) {
		for (Keystrokes::const_iterator k = keys.begin();
								k != keys.end(); ++k) {
			const UINT code = MapVirtualKey(k->first, 0);
			keybd_event(k->first, code, k->second ? 0 : KEYEVENTF_KEYUP, 0);
		}
	}
}

void					CMSWindowsSecondaryScreen::keyUp(
								KeyID key, KeyModifierMask mask)
{
	Keystrokes keys;
	UINT virtualKey;

	// get the sequence of keys to simulate key release and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, false);
	if (keys.empty())
		return;

	// generate key events
	for (Keystrokes::const_iterator k = keys.begin(); k != keys.end(); ++k) {
		const UINT code = MapVirtualKey(k->first, 0);
		keybd_event(k->first, code, k->second ? 0 : KEYEVENTF_KEYUP, 0);
	}

	// note that key is now up
	m_keys[virtualKey] &= ~0x80;
}

void					CMSWindowsSecondaryScreen::mouseDown(ButtonID button)
{
	// map button id to button flag
	DWORD flags = mapButton(button);

	// send event
	if (flags != 0)
		mouse_event(flags, 0, 0, 0, 0);
}

void					CMSWindowsSecondaryScreen::mouseUp(ButtonID button)
{
	// map button id to button flag
	DWORD flags = mapButton(button);

	// send event
	if (flags != 0)
		mouse_event(flags, 0, 0, 0, 0);
}

void					CMSWindowsSecondaryScreen::mouseMove(SInt32 x, SInt32 y)
{
	SInt32 w, h;
	getScreenSize(&w, &h);
	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
								(SInt32)(65535.99 * x / (w - 1)),
								(SInt32)(65535.99 * y / (h - 1)),
								0, 0);
}

void					CMSWindowsSecondaryScreen::mouseWheel(SInt32 delta)
{
	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0);
}

void					CMSWindowsSecondaryScreen::setClipboard(
								ClipboardID id, const IClipboard* src)
{
	assert(m_window != NULL);

	CMSWindowsClipboard dst(m_window);
	CClipboard::copy(&dst, src);
}

void					CMSWindowsSecondaryScreen::grabClipboard(ClipboardID id)
{
	assert(m_window != NULL);

	CMSWindowsClipboard clipboard(m_window);
	if (clipboard.open(0)) {
		clipboard.close();
	}
}

void					CMSWindowsSecondaryScreen::getSize(
								SInt32* width, SInt32* height) const
{
	getScreenSize(width, height);
}

SInt32					CMSWindowsSecondaryScreen::getJumpZoneSize() const
{
	return 0;
}

void					CMSWindowsSecondaryScreen::getClipboard(
								ClipboardID id, IClipboard* dst) const
{
	assert(m_window != NULL);

	CMSWindowsClipboard src(m_window);
	CClipboard::copy(dst, &src);
}

#include "resource.h" // FIXME

void					CMSWindowsSecondaryScreen::onOpenDisplay()
{
	assert(m_window == NULL);

// create debug dialog
s_thread = GetCurrentThreadId();;
s_debug = CreateDialog(getInstance(), MAKEINTRESOURCE(IDD_SYNERGY), NULL, &debugProc);
s_debugLog = ::GetDlgItem(s_debug, IDC_LOG);
CLog::setOutputter(&debugOutput);
ShowWindow(s_debug, SW_SHOWNORMAL);

	// initialize clipboard owner to current owner.  we don't want
	// to take ownership of the clipboard just by starting up.
	m_clipboardOwner = GetClipboardOwner();

	// create the cursor hiding window.  this window is used to hide the
	// cursor when it's not on the screen.  the window is hidden as soon
	// as the cursor enters the screen or the display's real cursor is
	// moved.
	m_window = CreateWindowEx(WS_EX_TOPMOST |
								WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
								(LPCTSTR)getClass(), "Synergy",
								WS_POPUP,
								0, 0, 1, 1, NULL, NULL,
								getInstance(),
								NULL);

	// hide the cursor
	leave();

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);
}

void					CMSWindowsSecondaryScreen::onCloseDisplay()
{
	assert(m_window != NULL);

	// remove clipboard snooper
	ChangeClipboardChain(m_window, m_nextClipboardWindow);
	m_nextClipboardWindow = NULL;

	// destroy window
	DestroyWindow(m_window);
	m_window = NULL;

CLog::setOutputter(NULL);
DestroyWindow(s_debug);
s_debug = NULL;
s_thread = 0;
}

bool					CMSWindowsSecondaryScreen::onPreTranslate(MSG* msg)
{
if (IsDialogMessage(s_debug, msg)) {
	return true;
}
	return false;
}


LRESULT					CMSWindowsSecondaryScreen::onEvent(
								HWND hwnd, UINT msg,
								WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	// FIXME -- handle display changes
	case WM_PAINT:
		ValidateRect(hwnd, NULL);
		return 0;

	case WM_ACTIVATEAPP:
		if (wParam == FALSE) {
			// some other app activated.  hide the hider window.
			log((CLOG_INFO "show cursor"));
			ShowWindow(m_window, SW_HIDE);
		}
		break;

	case WM_DRAWCLIPBOARD:
		log((CLOG_DEBUG "clipboard was taken"));

		// first pass it on
		SendMessage(m_nextClipboardWindow, msg, wParam, lParam);

		// now notify client that somebody changed the clipboard (unless
		// we're now the owner, in which case it's because we took
		// ownership).
		m_clipboardOwner = GetClipboardOwner();
		if (m_clipboardOwner != m_window) {
			m_client->onClipboardChanged(kClipboardClipboard);
			m_client->onClipboardChanged(kClipboardSelection);
		}
		return 0;

	case WM_CHANGECBCHAIN:
		if (m_nextClipboardWindow == (HWND)wParam)
			m_nextClipboardWindow = (HWND)lParam;
		else
			SendMessage(m_nextClipboardWindow, msg, wParam, lParam);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static const UINT		g_latin1[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin2[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin3[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin4[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin5[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin6[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin7[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin8[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_latin9[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_terminal[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
static const UINT		g_function[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
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
// FIXME -- will this work?
// 0x100 + = shift
// 0x200 + = ctrl
// 0x400 + = alt
/* XK_KP_Space to XK_KP_Equal */
static const UINT		g_miscellany[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ VK_BACK, VK_TAB, /*0x100 +*/ VK_RETURN, VK_CLEAR, 0, VK_RETURN, 0, 0,
	/* 0x10 */ 0, 0, 0, VK_PAUSE, VK_SCROLL, 0/*sys-req*/, 0, 0,
	/* 0x18 */ 0, 0, 0, VK_ESCAPE, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ VK_HOME, VK_LEFT, VK_UP, VK_RIGHT,
	/* 0x54 */ VK_DOWN, VK_PRIOR, VK_NEXT, VK_END,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT, 0, 0, 0, VK_APPS,
	/* 0x68 */ 0, 0, VK_HELP, VK_CANCEL, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, VK_MODECHANGE, VK_NUMLOCK,
	/* 0x80 */ VK_SPACE, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, VK_TAB, 0, 0, 0, VK_RETURN, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,
	/* 0x98 */ VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT,
	/* 0x9c */ VK_END, 0, VK_INSERT, VK_DELETE,
	/* 0xa0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa8 */ 0, 0, VK_MULTIPLY, VK_ADD,
	/* 0xac */ VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE,
	/* 0xb0 */ VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3,
	/* 0xb4 */ VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,
	/* 0xb8 */ VK_NUMPAD8, VK_NUMPAD9, 0, 0, 0, 0, VK_F1, VK_F2,
	/* 0xc0 */ VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
	/* 0xc8 */ VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18,
	/* 0xd0 */ VK_F19, VK_F20, VK_F21, VK_F22, VK_F23, VK_F24, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* FIXME -- want to use LSHIFT, LCONTROL, and LMENU but those don't seem
 * to affect the shift state for VkKeyScan. */
	/* 0xe0 */ 0, VK_SHIFT, VK_RSHIFT, VK_CONTROL,
	/* 0xe4 */ VK_RCONTROL, VK_CAPITAL, 0, VK_LWIN,
	/* 0xe8 */ VK_RWIN, VK_MENU, VK_RMENU, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, VK_DELETE
};
static const UINT*		g_katakana = NULL;
static const UINT*		g_arabic = NULL;
static const UINT*		g_cyrillic = NULL;
static const UINT*		g_greek = NULL;
static const UINT*		g_technical = NULL;
static const UINT*		g_special = NULL;
static const UINT*		g_publishing = NULL;
static const UINT*		g_apl = NULL;
static const UINT*		g_hebrew = NULL;
static const UINT*		g_thai = NULL;
static const UINT*		g_korean = NULL;
static const UINT*		g_armenian = NULL;
static const UINT*		g_georgian = NULL;
static const UINT*		g_azeri = NULL;
static const UINT*		g_vietnamese = NULL;
static const UINT*		g_currency = NULL;
static const UINT*		g_mapTable[] =
{
	/* 0x00 */ g_latin1, g_latin2, g_latin3, g_latin4,
	/* 0x04 */ g_katakana, g_arabic, g_cyrillic, g_greek,
	/* 0x08 */ g_technical, g_special, g_publishing, g_apl,
	/* 0x0c */ g_hebrew, g_thai, g_korean, NULL,
	/* 0x10 */ NULL, NULL, g_latin8, g_latin9,
	/* 0x14 */ g_armenian, g_georgian, g_azeri, NULL,
	/* 0x18 */ NULL, NULL, NULL, NULL, NULL, NULL, g_vietnamese, NULL,
	/* 0x20 */ g_currency, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x28 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x30 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x38 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x40 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x48 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x50 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x58 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x60 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x68 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x70 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x78 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x80 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x88 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x90 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0x98 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xa0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xa8 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xb0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xb8 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xc0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xc8 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xd0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xd8 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xe0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xe8 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xf0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* 0xf8 */ NULL, NULL, NULL, NULL,
	/* 0xfc */ NULL, g_terminal, g_function, g_miscellany
};

DWORD					CMSWindowsSecondaryScreen::mapButton(
								ButtonID button) const
{
	// map button id to button flag
	switch (button) {
	case kButtonLeft:
		return MOUSEEVENTF_LEFTDOWN;

	case kButtonMiddle:
		return MOUSEEVENTF_MIDDLEDOWN;

	case kButtonRight:
		return MOUSEEVENTF_RIGHTDOWN;

	default:
		return 0;
	}
}

KeyModifierMask			CMSWindowsSecondaryScreen::mapKey(
								Keystrokes& keys,
								UINT& virtualKey,
								KeyID id, KeyModifierMask mask,
								bool press) const
{
	// lookup the key table
	const UInt32 mapID = ((id >> 8) & 0xff);
	const UINT* map    = g_mapTable[mapID];
	if (map == NULL) {
		// unknown key
		return m_mask;
	}

	// look up virtual key for id.  default output mask carries over
	// the current toggle modifier states.
	const UInt32 code       = (id & 0xff);
	virtualKey              = map[code];
	KeyModifierMask outMask = (m_mask &
								(KeyModifierCapsLock |
								KeyModifierNumLock |
								KeyModifierScrollLock));

	// if not in map then ask system to convert ascii character
	if (virtualKey == 0) {
		if (mapID != 0) {
			// not ascii
			return m_mask;
		}

		// translate.  return no keys if unknown key.
		SHORT vk = VkKeyScan(code);
		if (vk == 0xffff) {
			return m_mask;
		}

		// convert system modifier mask to our mask
		if (HIBYTE(vk) & 1)
			outMask |= KeyModifierShift;
		if (HIBYTE(vk) & 2)
			outMask |= KeyModifierControl;
		if (HIBYTE(vk) & 4)
			outMask |= KeyModifierAlt;

		// if caps-lock is on and so is shift then turn off caps-lock
		if (outMask & (KeyModifierShift | KeyModifierCapsLock) ==
								(KeyModifierShift | KeyModifierCapsLock))
			outMask &= ~KeyModifierCapsLock;

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
			if ((outMask & KeyModifierNumLock) == 0)
				outMask |= KeyModifierShift;
		}

		// FIXME -- should check for LeftTab KeySym
	}

	// a list of modifier key info
	class CModifierInfo {
	public:
		KeyModifierMask	mask;
		UINT			virtualKey;
		UINT			virtualKey2;
		bool			isToggle;
	};
	static const CModifierInfo s_modifier[] = {
		{ KeyModifierShift,			VK_LSHIFT,		VK_RSHIFT,		false },
		{ KeyModifierControl,		VK_LCONTROL,	VK_RCONTROL,	false },
		{ KeyModifierAlt,			VK_LMENU,		VK_RMENU,		false },
		{ KeyModifierMeta,			VK_LWIN,		VK_RWIN,		false },
		{ KeyModifierCapsLock,		VK_CAPITAL,		0,				true },
		{ KeyModifierNumLock,		VK_NUMLOCK,		0,				true },
		{ KeyModifierScrollLock,	VK_SCROLL,		0,				true }
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
	if (outMask != m_mask && !isModifier) {
		for (unsigned int i = 0; i < s_numModifiers; ++i) {
			KeyModifierMask bit = s_modifier[i].mask;
			if ((outMask & bit) != (m_mask & bit)) {
				if ((outMask & bit) != 0) {
					// modifier is not active but should be.  if the
					// modifier is a toggle then toggle it on with a
					// press/release, otherwise activate it with a
					// press.
					const UINT modifierKey = s_modifier[i].virtualKey;
					keys.push_back(std::make_pair(modifierKey, true));
					if (s_modifier[i].isToggle) {
						keys.push_back(std::make_pair(modifierKey, false));
						undo.push_back(std::make_pair(modifierKey, false));
						undo.push_back(std::make_pair(modifierKey, true));
					}
					else {
						undo.push_back(std::make_pair(modifierKey, false));
					}
				}

				else {
					// modifier is active but should not be.  if the
					// modifier is a toggle then toggle it off with a
					// press/release, otherwise deactivate it with a
					// release.  we must check each keycode for the
					// modifier if not a toggle.
					if (s_modifier[i].isToggle) {
						const UINT modifierKey = s_modifier[i].virtualKey;
						keys.push_back(std::make_pair(modifierKey, true));
						keys.push_back(std::make_pair(modifierKey, false));
						undo.push_back(std::make_pair(modifierKey, false));
						undo.push_back(std::make_pair(modifierKey, true));
					}
					else {
						UINT key = s_modifier[i].virtualKey;
						if ((m_keys[key] & 0x80) != 0) {
							keys.push_back(std::make_pair(key, false));
							undo.push_back(std::make_pair(key, true));
						}
						key = s_modifier[i].virtualKey2;
						if ((m_keys[key] & 0x80) != 0) {
							keys.push_back(std::make_pair(key, false));
							undo.push_back(std::make_pair(key, true));
						}
					}
				}
			}
		}
	}

	// add the key event
	keys.push_back(std::make_pair(virtualKey, press));

	// add key events to restore the modifier state.  apply events in
	// the reverse order that they're stored in undo.
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	// if the key is a modifier key then compute the modifier mask after
	// this key is pressed.
	mask = m_mask;
	if (isModifier) {
		// toggle keys modify the state on press if toggling on and on
		// release if toggling off.  other keys set the bit on press
		// and clear the bit on release.
		// FIXME -- verify if that's true on win32
		if (s_modifier[modifierIndex].isToggle) {
			if (((mask & s_modifier[modifierIndex].mask) == 0) == press)
				mask ^= s_modifier[modifierIndex].mask;
		}
		else if (press) {
			mask |= s_modifier[modifierIndex].mask;
		}
		else {
			// can't reset bit until all keys that set it are released.
			// scan those keys to see if any are pressed.
			bool down = false;
			if ((m_keys[s_modifier[modifierIndex].virtualKey] & 0x80) != 0) {
				down = true;
			}
			if ((m_keys[s_modifier[modifierIndex].virtualKey2] & 0x80) != 0) {
				down = true;
			}
			if (!down)
				mask &= ~s_modifier[modifierIndex].mask;
		}
	}

	return mask;
}

void					CMSWindowsSecondaryScreen::updateKeys()
{
	GetKeyboardState(m_keys);
}

void					CMSWindowsSecondaryScreen::updateModifiers()
{
	// update active modifier mask
	m_mask = 0;
	if ((m_keys[VK_LSHIFT] & 0x80) != 0 || (m_keys[VK_RSHIFT] & 0x80) != 0)
		m_mask |= KeyModifierShift;
	if ((m_keys[VK_LCONTROL] & 0x80) != 0 || (m_keys[VK_RCONTROL] & 0x80) != 0)
		m_mask |= KeyModifierControl;
	if ((m_keys[VK_LMENU] & 0x80) != 0 || (m_keys[VK_RMENU] & 0x80) != 0)
		m_mask |= KeyModifierAlt;
	if ((m_keys[VK_LWIN] & 0x80) != 0 || (m_keys[VK_RWIN] & 0x80) != 0)
		m_mask |= KeyModifierMeta;
	if ((m_keys[VK_CAPITAL] & 0x01) != 0)
		m_mask |= KeyModifierCapsLock;
	if ((m_keys[VK_NUMLOCK] & 0x01) != 0)
		m_mask |= KeyModifierNumLock;
	if ((m_keys[VK_SCROLL] & 0x01) != 0)
		m_mask |= KeyModifierScrollLock;
}

void					CMSWindowsSecondaryScreen::toggleKey(
								UINT virtualKey, KeyModifierMask mask)
{
	// send key events to simulate a press and release
	const UINT code = MapVirtualKey(virtualKey, 0);
	keybd_event(virtualKey, code, 0, 0);
	keybd_event(virtualKey, code, KEYEVENTF_KEYUP, 0);

	// toggle shadow state
	m_mask             ^= mask;
	m_keys[virtualKey] ^= 0x01;
}
