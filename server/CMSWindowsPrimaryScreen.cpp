#include "CMSWindowsPrimaryScreen.h"
#include "CServer.h"
#include "CSynergyHook.h"
#include "CThread.h"
#include "CLog.h"
#include <assert.h>

//
// CMSWindowsPrimaryScreen
//

CMSWindowsPrimaryScreen::CMSWindowsPrimaryScreen() :
								m_server(NULL),
								m_active(false),
								m_window(NULL),
								m_hookLibrary(NULL),
								m_mark(0),
								m_markReceived(0)
{
	// do nothing
}

CMSWindowsPrimaryScreen::~CMSWindowsPrimaryScreen()
{
	assert(m_window      == NULL);
	assert(m_hookLibrary == NULL);
}

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
	}
	return FALSE;
}
static void debugOutput(const char* msg)
{
	if (s_thread != 0) {
		const DWORD threadID = ::GetCurrentThreadId();
		if (threadID != s_thread) {
			GetDesktopWindow();
			AttachThreadInput(threadID, s_thread, TRUE);
		}
	}
	DWORD len = SendMessage(s_debugLog, WM_GETTEXTLENGTH, 0, 0);
	if (len > 20000) {
		SendMessage(s_debugLog, EM_SETSEL, -1, 0);
		SendMessage(s_debugLog, WM_SETTEXT, FALSE, (LPARAM)(LPCTSTR)msg);
	}
	else {
		SendMessage(s_debugLog, EM_SETSEL, -1, len);
		SendMessage(s_debugLog, EM_REPLACESEL, FALSE, (LPARAM)(LPCTSTR)msg);
	}
	SendMessage(s_debugLog, EM_SCROLLCARET, 0, 0);
}

void					CMSWindowsPrimaryScreen::run()
{
CLog::setOutputter(&debugOutput);
	doRun();
CLog::setOutputter(NULL);
}

void					CMSWindowsPrimaryScreen::stop()
{
	doStop();
}

void					CMSWindowsPrimaryScreen::open(CServer* server)
{
	assert(m_server == NULL);
	assert(server   != NULL);

	// set the server
	m_server = server;

	// open the display
	openDisplay();

	// enter the screen
	doEnter();
}

void					CMSWindowsPrimaryScreen::close()
{
	assert(m_server != NULL);

	// close the display
	closeDisplay();

	// done with server
	m_server = NULL;
}

void					CMSWindowsPrimaryScreen::enter(SInt32 x, SInt32 y)
{
	log((CLOG_INFO "entering primary at %d,%d", x, y));
	assert(m_active == true);

	// do non-warp enter stuff
	doEnter();

	// warp to requested location
	warpCursor(x, y);
}

void					CMSWindowsPrimaryScreen::doEnter()
{
	// set the zones that should cause a jump
	SInt32 w, h;
	getScreenSize(&w, &h);
	SetZoneFunc setZone = (SetZoneFunc)GetProcAddress(
											m_hookLibrary, "setZone");
	setZone(m_server->getActivePrimarySides(), w, h, getJumpZoneSize());

	// all messages prior to now are invalid
	nextMark();

	// not active anymore
	m_active = false;
}

void					CMSWindowsPrimaryScreen::leave()
{
	log((CLOG_INFO "leaving primary"));
	assert(m_active == false);

	// all messages prior to now are invalid
	nextMark();

	// relay all mouse and keyboard events
	SetRelayFunc setRelay = (SetRelayFunc)GetProcAddress(
											m_hookLibrary, "setRelay");
	setRelay();

	// warp the mouse to the center of the screen
	SInt32 w, h;
	getScreenSize(&w, &h);
	warpCursor(w >> 1, h >> 1);

	// warp is also invalid
	nextMark();

	// local client now active
	m_active = true;
}

void					CMSWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	SInt32 w, h;
	getScreenSize(&w, &h);
	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
								(DWORD)((65535.99 * x) / (w - 1)),
								(DWORD)((65535.99 * y) / (h - 1)),
								0, 0);
}

void					CMSWindowsPrimaryScreen::setClipboard(
								const IClipboard* /*clipboard*/)
{
	assert(m_window != NULL);

	// FIXME -- should we retry until we get it?
	if (!OpenClipboard(m_window))
		return;
	if (EmptyClipboard()) {
		log((CLOG_DEBUG "grabbed clipboard"));
		// FIXME -- set the clipboard data
	}
	CloseClipboard();
}

void					CMSWindowsPrimaryScreen::getSize(
								SInt32* width, SInt32* height) const
{
	getScreenSize(width, height);
}

SInt32					CMSWindowsPrimaryScreen::getJumpZoneSize() const
{
	return 1;
}

void					CMSWindowsPrimaryScreen::getClipboard(
								IClipboard* clipboard) const
{
	// FIXME -- put this in superclass?
	// FIXME -- don't use CurrentTime
//	getDisplayClipboard(clipboard, m_window, CurrentTime);
}

#include "resource.h" // FIXME

void					CMSWindowsPrimaryScreen::onOpenDisplay()
{
	assert(m_window == NULL);
	assert(m_server != NULL);

// create debug dialog
s_thread = GetCurrentThreadId();;
s_debug = CreateDialog(getInstance(), MAKEINTRESOURCE(IDD_SYNERGY), NULL, &debugProc);
s_debugLog = ::GetDlgItem(s_debug, IDC_LOG);
CLog::setOutputter(&debugOutput);
ShowWindow(s_debug, SW_SHOWNORMAL);

	// create the window
	m_window = CreateWindowEx(WS_EX_TOPMOST |
								WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
								(LPCTSTR)getClass(), "Synergy",
								WS_POPUP,
								0, 0, 1, 1, NULL, NULL,
								getInstance(),
								NULL);

	// load the hook library
	bool hooked = false;
	m_hookLibrary = LoadLibrary("synrgyhk");
	if (m_hookLibrary != NULL) {
		// install input hooks
		InstallFunc install = (InstallFunc)GetProcAddress(
											m_hookLibrary, "install"); 
		if (install != NULL) {
			hooked = (install(m_window) != 0);
		}
	}
	if (!hooked) {
		DestroyWindow(m_window);
		m_window = NULL;
		// FIXME -- throw
	}

	// initialize marks
	m_mark         = 0;
	m_markReceived = 0;
	nextMark();
}

void					CMSWindowsPrimaryScreen::onCloseDisplay()
{
	assert(m_window != NULL);

	// uninstall input hooks
	UninstallFunc uninstall = (UninstallFunc)GetProcAddress(
											m_hookLibrary, "uninstall"); 
	if (uninstall != NULL) {
		uninstall();
	}

	// done with hook library
	FreeLibrary(m_hookLibrary);
	m_hookLibrary = NULL;

	// destroy window
	DestroyWindow(m_window);
	m_window = NULL;

CLog::setOutputter(NULL);
DestroyWindow(s_debug);
s_debug = NULL;
s_thread = 0;
}

bool					CMSWindowsPrimaryScreen::onEvent(MSG* msg)
{
if (IsDialogMessage(s_debug, msg)) {
	return true;
}

	// handle event
	switch (msg->message) {
	  // FIXME -- handle display changes
	  case WM_PAINT:
		ValidateRect(m_window, NULL);
		return true;

	  case SYNERGY_MSG_MARK:
		m_markReceived = msg->wParam;
		return true;

	  case SYNERGY_MSG_KEY:
		// ignore if not at current mark
		if (m_mark == m_markReceived) {
			// FIXME -- vk code; key data
		}
		return true;

	  case SYNERGY_MSG_MOUSE_BUTTON:
		// ignore if not at current mark
		if (m_mark == m_markReceived) {
			const ButtonID button = mapButton(msg->wParam);
			switch (msg->wParam) {
			  case WM_LBUTTONDOWN:
			  case WM_MBUTTONDOWN:
			  case WM_RBUTTONDOWN:
				log((CLOG_DEBUG "event: button press button=%d", button));
				if (button != kButtonNone) {
					m_server->onMouseDown(button);
				}
				break;

			  case WM_LBUTTONUP:
			  case WM_MBUTTONUP:
			  case WM_RBUTTONUP:
				log((CLOG_DEBUG "event: button release button=%d", button));
				if (button != kButtonNone) {
					m_server->onMouseUp(button);
				}
				break;
			}
		}
		return true;

	  case SYNERGY_MSG_MOUSE_MOVE:
		// ignore if not at current mark
		if (m_mark == m_markReceived) {
			SInt32 x = (SInt32)msg->wParam;
			SInt32 y = (SInt32)msg->lParam;
			if (!m_active) {
				log((CLOG_DEBUG "event: inactive move %d,%d", x, y));
				m_server->onMouseMovePrimary(x, y);
			}
			else {
				log((CLOG_DEBUG "event: active move %d,%d", x, y));

				// get screen size
				SInt32 w, h;
				getScreenSize(&w, &h);

				// get center pixel
				w >>= 1;
				h >>= 1;

				// ignore and discard message if motion is to center of
				// screen.  those are caused by us warping the mouse.
				if (x != w || y != h) {
					// get mouse deltas
					x -= w;
					y -= h;

					// warp mouse back to center
					warpCursor(w, h);

					// send motion
					m_server->onMouseMoveSecondary(x, y);
				}
			}
		}
		return true;
	}

	return false;
/*
	  case WM_MOUSEMOVE: {
		if (!m_active) {
			// mouse entered a jump zone window
			POINT p;
			p.x = (short)LOWORD(msg.lParam);
			p.y = (short)HIWORD(msg.lParam);
			ClientToScreen(msg.hwnd, &p);
			log((CLOG_DEBUG "event: WM_MOUSEMOVE %d,%d", p.x, p.y));
			m_server->onMouseMovePrimary((SInt32)p.x, (SInt32)p.y);
		}
		break;
	  }

	  case KeyPress: {
		log((CLOG_DEBUG "event: KeyPress code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
		const KeyModifierMask mask = mapModifier(xevent.xkey.state);
		const KeyID key = mapKey(xevent.xkey.keycode, mask);
		if (key != kKeyNULL) {
			m_server->onKeyDown(key, mask);
		}
		break;
	  }

	  // FIXME -- simulate key repeat.  X sends press/release for
	  // repeat.  must detect auto repeat and use kKeyRepeat.
	  case KeyRelease: {
		log((CLOG_DEBUG "event: KeyRelease code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
		const KeyModifierMask mask = mapModifier(xevent.xkey.state);
		const KeyID key = mapKey(xevent.xkey.keycode, mask);
		if (key != kKeyNULL) {
			m_server->onKeyUp(key, mask);
		}
		break;
	  }

	  case ButtonPress: {
		log((CLOG_DEBUG "event: ButtonPress button=%d", xevent.xbutton.button));
		const ButtonID button = mapButton(xevent.xbutton.button);
		if (button != kButtonNULL) {
			m_server->onMouseDown(button);
		}
		break;
	  }

	  case ButtonRelease: {
		log((CLOG_DEBUG "event: ButtonRelease button=%d", xevent.xbutton.button));
		const ButtonID button = mapButton(xevent.xbutton.button);
		if (button != kButtonNULL) {
			m_server->onMouseUp(button);
		}
		break;
	  }

	  case SelectionClear:
		target->XXX(xevent.xselectionclear.);
		break;

	  case SelectionNotify:
		target->XXX(xevent.xselection.);
		break;

	  case SelectionRequest:
		target->XXX(xevent.xselectionrequest.);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
*/
}

void					CMSWindowsPrimaryScreen::nextMark()
{
	assert(m_window != NULL);

	PostMessage(m_window, SYNERGY_MSG_MARK, ++m_mark, 0);
}

#if 0
bool					CMSWindowsPrimaryScreen::keyboardHook(
								int /*code*/, WPARAM wParam, LPARAM lParam)
{
	if (m_active) {
		// handle keyboard events
		const KeyID key = mapKey(wParam, lParam);
		if (key != kKeyNone) {
			const KeyModifierMask modifiers = mapModifier(wParam, lParam);
			if ((lParam & KF_UP) == 0) {
				log((CLOG_DEBUG "event: key press key=%d", key));
				m_server->onKeyDown(key, modifiers);
			}
			else {
				log((CLOG_DEBUG "event: key release key=%d", key));
				m_server->onKeyUp(key, modifiers);
			}
			return true;
		}
	}
	return false;
}
#endif

KeyModifierMask			CMSWindowsPrimaryScreen::mapModifier(
								WPARAM keycode, LPARAM info) const
{
	// FIXME -- should be configurable
	KeyModifierMask mask = 0;
	if (GetKeyState(VK_SHIFT) < 0)
		mask |= KeyModifierShift;
	if ((GetKeyState(VK_CAPITAL) & 1) != 0)
		mask |= KeyModifierCapsLock;
	if (GetKeyState(VK_CONTROL) < 0)
		mask |= KeyModifierControl;
	if (GetKeyState(VK_MENU) < 0)
		mask |= KeyModifierAlt;
	if ((GetKeyState(VK_NUMLOCK) & 1) != 0)
		mask |= KeyModifierNumLock;
	if (GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0)
		mask |= KeyModifierMeta;
	if ((GetKeyState(VK_SCROLL) & 1) != 0)
		mask |= KeyModifierScrollLock;
	return mask;
}

KeyID					CMSWindowsPrimaryScreen::mapKey(
								WPARAM keycode, LPARAM info) const
{
	// FIXME -- must convert to X keysyms
	return keycode;
}

ButtonID				CMSWindowsPrimaryScreen::mapButton(
								WPARAM button) const
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
