#include "CMSWindowsSecondaryScreen.h"
#include "CMSWindowsClipboard.h"
#include "CClient.h"
#include "CPlatform.h"
#include "CClipboard.h"
#include "CLock.h"
#include "CLog.h"
#include "CThread.h"
#include "XScreen.h"
#include <assert.h>
#include <ctype.h>

//
// CMSWindowsSecondaryScreen
//

CMSWindowsSecondaryScreen::CMSWindowsSecondaryScreen() :
								m_client(NULL),
								m_threadID(0),
								m_desk(NULL),
								m_deskName(),
								m_window(NULL),
								m_active(false),
								m_nextClipboardWindow(NULL)
{
	m_is95Family = CPlatform::isWindows95Family();

	// make sure this thread has a message queue
	MSG dummy;
	PeekMessage(&dummy, NULL, WM_USER, WM_USER, PM_NOREMOVE);
}

CMSWindowsSecondaryScreen::~CMSWindowsSecondaryScreen()
{
	assert(m_window == NULL);
}

void					CMSWindowsSecondaryScreen::run()
{
	// must call run() from same thread as open()
	assert(m_threadID == GetCurrentThreadId());

	// change our priority
	CThread::getCurrentThread().setPriority(-7);

	// poll input desktop to see if it changes (onPreTranslate()
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

void					CMSWindowsSecondaryScreen::stop()
{
	doStop();
}

void					CMSWindowsSecondaryScreen::open(CClient* client)
{
	assert(m_client == NULL);
	assert(client   != NULL);

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

	// hide the cursor
	m_active = true;
	leave();
}

void					CMSWindowsSecondaryScreen::close()
{
	assert(m_client != NULL);

	// close the display
	closeDisplay();

	// done with client
	m_client = NULL;
}

void					CMSWindowsSecondaryScreen::enter(
								SInt32 x, SInt32 y, KeyModifierMask mask)
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);
	assert(m_active == false);

	log((CLOG_INFO "entering screen at %d,%d mask=%04x", x, y, mask));

	syncDesktop();

	// now active
	m_active = true;

	// update our keyboard state to reflect the local state
	updateKeys();
	updateModifiers();

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

	// hide mouse
	onEnter(x, y);
}

void					CMSWindowsSecondaryScreen::leave()
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);
	assert(m_active == true);

	log((CLOG_INFO "leaving screen"));

	syncDesktop();

	// hide mouse
	onLeave();

	// not active anymore
	m_active = false;

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

	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	// get the sequence of keys to simulate key press and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, kPress);
	if (keys.empty())
		return;

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now down
	m_keys[virtualKey] |= 0x80;
	switch (virtualKey) {
	case VK_LSHIFT:
	case VK_RSHIFT:
		m_keys[VK_SHIFT]   |= 0x80;
		break;

	case VK_LCONTROL:
	case VK_RCONTROL:
		m_keys[VK_CONTROL] |= 0x80;
		break;

	case VK_LMENU:
	case VK_RMENU:
		m_keys[VK_MENU]    |= 0x80;
		break;
	}
}

void					CMSWindowsSecondaryScreen::keyRepeat(
								KeyID key, KeyModifierMask mask, SInt32 count)
{
	Keystrokes keys;
	UINT virtualKey;

	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	// get the sequence of keys to simulate key repeat and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, kRepeat);
	if (keys.empty())
		return;

	// generate key events
	doKeystrokes(keys, count);
}

void					CMSWindowsSecondaryScreen::keyUp(
								KeyID key, KeyModifierMask mask)
{
	Keystrokes keys;
	UINT virtualKey;

	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	// get the sequence of keys to simulate key release and the final
	// modifier state.
	m_mask = mapKey(keys, virtualKey, key, mask, kRelease);
	if (keys.empty())
		return;

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now up
	m_keys[virtualKey] &= ~0x80;
	switch (virtualKey) {
	case VK_LSHIFT:
		if ((m_keys[VK_RSHIFT] & 0x80) == 0) {
			m_keys[VK_SHIFT]   &= ~0x80;
		}
		break;

	case VK_RSHIFT:
		if ((m_keys[VK_LSHIFT] & 0x80) == 0) {
			m_keys[VK_SHIFT]   &= ~0x80;
		}
		break;

	case VK_LCONTROL:
		if ((m_keys[VK_RCONTROL] & 0x80) == 0) {
			m_keys[VK_CONTROL] &= ~0x80;
		}
		break;

	case VK_RCONTROL:
		if ((m_keys[VK_LCONTROL] & 0x80) == 0) {
			m_keys[VK_CONTROL] &= ~0x80;
		}
		break;

	case VK_LMENU:
		if ((m_keys[VK_RMENU] & 0x80) == 0) {
			m_keys[VK_MENU]    &= ~0x80;
		}
		break;

	case VK_RMENU:
		if ((m_keys[VK_LMENU] & 0x80) == 0) {
			m_keys[VK_MENU]    &= ~0x80;
		}
		break;
	}
}

void					CMSWindowsSecondaryScreen::mouseDown(ButtonID button)
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	// map button id to button flag
	DWORD flags = mapButton(button, true);

	// send event
	if (flags != 0)
		mouse_event(flags, 0, 0, 0, 0);
}

void					CMSWindowsSecondaryScreen::mouseUp(ButtonID button)
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	// map button id to button flag
	DWORD flags = mapButton(button, false);

	// send event
	if (flags != 0)
		mouse_event(flags, 0, 0, 0, 0);
}

void					CMSWindowsSecondaryScreen::mouseMove(
								SInt32 x, SInt32 y)
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	SInt32 w, h;
	getScreenSize(&w, &h);
	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
								(SInt32)(65535.99 * x / (w - 1)),
								(SInt32)(65535.99 * y / (h - 1)),
								0, 0);
}

void					CMSWindowsSecondaryScreen::mouseWheel(SInt32 delta)
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0);
}

void					CMSWindowsSecondaryScreen::setClipboard(
								ClipboardID /*id*/, const IClipboard* src)
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);

	CMSWindowsClipboard dst(m_window);
	CClipboard::copy(&dst, src);
}

void					CMSWindowsSecondaryScreen::grabClipboard(
								ClipboardID /*id*/)
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);

	CMSWindowsClipboard clipboard(m_window);
	if (clipboard.open(0)) {
		clipboard.close();
	}
}

void					CMSWindowsSecondaryScreen::getMousePos(
								SInt32* x, SInt32* y) const
{
	assert(x != NULL);
	assert(y != NULL);

	CLock lock(&m_mutex);
	assert(m_window != NULL);
	syncDesktop();

	POINT pos;
	if (GetCursorPos(&pos)) {
		*x = pos.x;
		*y = pos.y;
	}
	else {
		*x = 0;
		*y = 0;
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
								ClipboardID /*id*/, IClipboard* dst) const
{
	CLock lock(&m_mutex);
	assert(m_window != NULL);

	CMSWindowsClipboard src(m_window);
	CClipboard::copy(dst, &src);
}

void					CMSWindowsSecondaryScreen::onOpenDisplay()
{
	assert(m_window == NULL);

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

void					CMSWindowsSecondaryScreen::onCloseDisplay()
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

bool					CMSWindowsSecondaryScreen::onPreTranslate(MSG* msg)
{
	// handle event
	switch (msg->message) {
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

LRESULT					CMSWindowsSecondaryScreen::onEvent(
								HWND hwnd, UINT msg,
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

	case WM_ACTIVATEAPP:
		if (wParam == FALSE) {
			// some other app activated.  hide the hider window.
			ShowWindow(m_window, SW_HIDE);
		}
		break;

	case WM_DRAWCLIPBOARD:
		log((CLOG_DEBUG "clipboard was taken"));

		// first pass it on
		SendMessage(m_nextClipboardWindow, msg, wParam, lParam);

		// now notify client that somebody changed the clipboard (unless
		// we're now the owner, in which case it's because we took
		// ownership, or now it's owned by nobody, which will happen if
		// we owned it and switched desktops because we destroy our
		// window to do that).
		m_clipboardOwner = GetClipboardOwner();
		if (m_clipboardOwner != m_window && m_clipboardOwner != NULL) {
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

	case WM_DISPLAYCHANGE:
		// screen resolution has changed
		updateScreenSize();
		m_client->onResolutionChanged();
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void					CMSWindowsSecondaryScreen::onEnter(SInt32 x, SInt32 y)
{
	// warp to requested location
	SInt32 w, h;
	getScreenSize(&w, &h);
	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
								(DWORD)((65535.99 * x) / (w - 1)),
								(DWORD)((65535.99 * y) / (h - 1)),
								0, 0);

	// show cursor
	ShowWindow(m_window, SW_HIDE);
}

void					CMSWindowsSecondaryScreen::onLeave()
{
	// move hider window under the mouse (rather than moving the mouse
	// somewhere else on the screen)
	POINT point;
	GetCursorPos(&point);
	MoveWindow(m_window, point.x, point.y, 1, 1, FALSE);

	// raise and show the hider window.  take activation.
	ShowWindow(m_window, SW_SHOWNORMAL);
}

bool					CMSWindowsSecondaryScreen::openDesktop()
{
	CLock lock(&m_mutex);

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

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	return true;
}

void					CMSWindowsSecondaryScreen::closeDesktop()
{
	CLock lock(&m_mutex);

	if (m_window != NULL) {
		// remove clipboard snooper
		ChangeClipboardChain(m_window, m_nextClipboardWindow);
		m_nextClipboardWindow = NULL;

		// destroy window
		DestroyWindow(m_window);
		m_window = NULL;
	}
}

bool					CMSWindowsSecondaryScreen::switchDesktop(HDESK desk)
{
	CLock lock(&m_mutex);

	bool ownClipboard = false;
	if (m_window != NULL) {
		// note if we own the clipboard
		ownClipboard = (m_clipboardOwner == m_window);

		// remove clipboard snooper
		ChangeClipboardChain(m_window, m_nextClipboardWindow);
		m_nextClipboardWindow = NULL;

		// destroy window
		DestroyWindow(m_window);
		m_window = NULL;
	}

	// done with desktop
	if (m_desk != NULL) {
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
	if (m_window == NULL) {
		log((CLOG_ERR "failed to create window: %d", GetLastError()));
		CloseDesktop(desk);
		return false;
	}

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	// if we owned the desktop then set the clipboard owner
	if (ownClipboard) {
		m_clipboardOwner = GetClipboardOwner();
	}

	// save new desktop
	m_desk     = desk;
	m_deskName = getDesktopName(m_desk);
	log((CLOG_INFO "switched to desktop %s", m_deskName.c_str()));

	// get desktop up to date
	if (!m_active) {
		onLeave();
	}

	return true;
}

void					CMSWindowsSecondaryScreen::syncDesktop() const
{
	// note -- mutex must be locked on entry

	DWORD threadID = GetCurrentThreadId();
	if (!m_is95Family) {
		if (GetThreadDesktop(threadID) != m_desk) {
			// FIXME -- this doesn't work.  if we set a desktop then
			// sending events doesn't work.
			if (SetThreadDesktop(m_desk) == 0) {
				log((CLOG_ERR "failed to set desktop: %d", GetLastError()));
			}
		}
	}
	AttachThreadInput(threadID, m_threadID, TRUE);
}

CString					CMSWindowsSecondaryScreen::getCurrentDesktopName() const
{
	return m_deskName;
}

// these tables map KeyID (a X windows KeySym) to virtual key codes.
// if the key is an extended key then the entry is the virtual key
// code | 0x100.  keys that map to normal characters have a 0 entry
// and the conversion is done elsewhere.
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
/* XK_KP_Space to XK_KP_Equal */
static const UINT		g_miscellany[] =
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
	/* 0xe4 */ VK_RCONTROL|0x100, VK_CAPITAL, 0, VK_LWIN|0x100,
	/* 0xe8 */ VK_RWIN|0x100, VK_LMENU, VK_RMENU|0x100, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, VK_DELETE|0x100
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
								ButtonID button, bool press) const
{
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

KeyModifierMask			CMSWindowsSecondaryScreen::mapKey(
								Keystrokes& keys,
								UINT& virtualKey,
								KeyID id, KeyModifierMask mask,
								EKeyAction action) const
{
	// lookup the key table
	const UInt32 mapID = ((id >> 8) & 0xff);
	const UINT* map    = g_mapTable[mapID];
	if (map == NULL) {
		// unknown key
		return m_mask;
	}

	// look up virtual key for id.  default output mask carries over
	// the current toggle modifier states and includes desired shift,
	// control, alt, and meta states.
	const UInt32 code       = (id & 0xff);
	virtualKey              = map[code];
	KeyModifierMask outMask = (m_mask &
								(KeyModifierCapsLock |
								KeyModifierNumLock |
								KeyModifierScrollLock));
	outMask                |= (mask &
								(KeyModifierShift |
								KeyModifierControl |
								KeyModifierAlt |
								KeyModifierMeta));
	log((CLOG_DEBUG2 "key id %d -> virtual key %d", id, virtualKey));

	// extract extended key flag
	const bool isExtended = ((virtualKey & 0x100) != 0);
	virtualKey           &= ~0x100;

	// if not in map then ask system to convert ascii character
	if (virtualKey == 0) {
		if (mapID != 0) {
			// not ascii
			log((CLOG_DEBUG2 "not ascii"));
			return m_mask;
		}

		// translate.  return no keys if unknown key.
		SHORT vk = VkKeyScan(static_cast<TCHAR>(code));
		if (vk == 0xffff) {
			log((CLOG_DEBUG2 "no virtual key for character %d", code));
			return m_mask;
		}

		// use whatever shift state VkKeyScan says
		// FIXME -- also for control and alt, but it's more difficult
		// to determine if control and alt must be off or if it just
		// doesn't matter.
		outMask &= ~KeyModifierShift;

		// convert system modifier mask to our mask
		if (HIBYTE(vk) & 1)
			outMask |= KeyModifierShift;
		if (HIBYTE(vk) & 2)
			outMask |= KeyModifierControl;
		if (HIBYTE(vk) & 4)
			outMask |= KeyModifierAlt;
		log((CLOG_DEBUG2 "character %d to virtual key %d mask 0x%04x", code, LOBYTE(vk), outMask));

		// handle combination of caps-lock and shift.  if caps-lock is
		// off locally then use shift as necessary.  if caps-lock is on
		// locally then it reverses the meaning of shift for keys that
		// are subject to case conversion.
		if ((outMask & KeyModifierCapsLock) != 0) {
			if (tolower(code) != toupper(code)) {
				log((CLOG_DEBUG2 "flip shift"));
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
					log((CLOG_DEBUG2 "turn on num lock for keypad key"));
					outMask |= KeyModifierNumLock;
				}
				else {
					log((CLOG_DEBUG2 "turn on shift for keypad key"));
					outMask |= KeyModifierShift;
				}
			}
		}

		// check for ISO_Left_Tab
		else if (id == 0xfe20) {
			outMask |= KeyModifierShift;
		}
	}

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
		{ KeyModifierMeta,		VK_LWIN | 0x100,	VK_RWIN | 0x100,	false },
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
	if (isExtended)
		keystroke.m_virtualKey |= 0x100;
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

	log((CLOG_DEBUG2 "previous modifiers 0x%04x, final modifiers 0x%04x", m_mask, mask));
	return mask;
}

void					CMSWindowsSecondaryScreen::doKeystrokes(
								const Keystrokes& keys, SInt32 count)
{
	// do nothing if no keys or no repeats
	if (count < 1 || keys.empty())
		return;

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

void					CMSWindowsSecondaryScreen::updateKeys()
{
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
	log((CLOG_DEBUG2 "modifiers on update: 0x%04x", m_mask));
}

void					CMSWindowsSecondaryScreen::toggleKey(
								UINT virtualKey, KeyModifierMask mask)
{
	// send key events to simulate a press and release
	sendKeyEvent(virtualKey, true);
	sendKeyEvent(virtualKey, false);

	// toggle shadow state
	m_mask                    ^= mask;
	m_keys[virtualKey & 0xff] ^= 0x01;
}

UINT					CMSWindowsSecondaryScreen::virtualKeyToScanCode(
								UINT& virtualKey)
{
	// try mapping given virtual key
	UINT code = MapVirtualKey(virtualKey & 0xff, 0);
	if (code != 0)
		return code;

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

bool					CMSWindowsSecondaryScreen::isExtendedKey(
								UINT virtualKey)
{
	// see if we've already encoded the extended flag
	if ((virtualKey & 0x100) != 0)
		return true;

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

void					CMSWindowsSecondaryScreen::sendKeyEvent(
								UINT virtualKey, bool press)
{
	DWORD flags = 0;
	if (isExtendedKey(virtualKey))
		flags |= KEYEVENTF_EXTENDEDKEY;
	if (!press)
		flags |= KEYEVENTF_KEYUP;
	const UINT code = virtualKeyToScanCode(virtualKey);
	keybd_event(static_cast<BYTE>(virtualKey & 0xff),
								static_cast<BYTE>(code), flags, 0);
	log((CLOG_DEBUG2 "send key %d, 0x%04x, %s%s", virtualKey & 0xff, code, ((flags & KEYEVENTF_KEYUP) ? "release" : "press"), ((flags & KEYEVENTF_EXTENDEDKEY) ? " extended" : "")));
}
