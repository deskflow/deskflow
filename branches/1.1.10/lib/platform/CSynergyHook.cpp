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

#include "CSynergyHook.h"
#include "ProtocolTypes.h"
#include <zmouse.h>

//
// debugging compile flag.  when not zero the server doesn't grab
// the keyboard when the mouse leaves the server screen.  this
// makes it possible to use the debugger (via the keyboard) when
// all user input would normally be caught by the hook procedures.
//
#define NO_GRAB_KEYBOARD 0

//
// extra mouse wheel stuff
//

enum EWheelSupport {
	kWheelNone,
	kWheelOld,
	kWheelWin2000,
	kWheelModern
};

// declare extended mouse hook struct.  useable on win2k
typedef struct tagMOUSEHOOKSTRUCTWin2000 {
	MOUSEHOOKSTRUCT mhs;
	DWORD mouseData;
} MOUSEHOOKSTRUCTWin2000;

#if !defined(SM_MOUSEWHEELPRESENT)
#define SM_MOUSEWHEELPRESENT 75
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


//
// globals
//

#pragma comment(linker, "-section:shared,rws")
#pragma data_seg("shared")
// all data in this shared section *must* be initialized

static HINSTANCE		g_hinstance       = NULL;
static DWORD			g_processID       = 0;
static EWheelSupport	g_wheelSupport    = kWheelNone;
static UINT				g_wmMouseWheel    = 0;
static DWORD			g_threadID        = 0;
static HHOOK			g_keyboard        = NULL;
static HHOOK			g_mouse           = NULL;
static HHOOK			g_getMessage      = NULL;
static HANDLE			g_hookThreadLL    = NULL;
static DWORD			g_hookThreadIDLL  = 0;
static HANDLE			g_hookEventLL     = NULL;
static HHOOK			g_keyboardLL      = NULL;
static HHOOK			g_mouseLL         = NULL;
static bool				g_screenSaver     = false;
static bool				g_relay           = false;
static UInt32			g_zoneSides       = 0;
static SInt32			g_zoneSize        = 0;
static SInt32			g_xScreen         = 0;
static SInt32			g_yScreen         = 0;
static SInt32			g_wScreen         = 0;
static SInt32			g_hScreen         = 0;
static HCURSOR			g_cursor          = NULL;
static DWORD			g_cursorThread    = 0;

#pragma data_seg()

// keep linker quiet about floating point stuff.  we don't use any
// floating point operations but our includes may define some
// (unused) floating point values.
#ifndef _DEBUG
extern "C" int _fltused=0;
#endif


//
// internal functions
//

static
void
hideCursor(DWORD thread)
{
	// we should be running the context of the window who's cursor
	// we want to hide so we shouldn't have to attach thread input
	// but we'll check to make sure.
	g_cursorThread = thread;
	if (g_cursorThread != 0) {
		DWORD myThread = GetCurrentThreadId();
		if (myThread != g_cursorThread)
			AttachThreadInput(myThread, g_cursorThread, TRUE);
		g_cursor = SetCursor(NULL);
		if (myThread != g_cursorThread)
			AttachThreadInput(myThread, g_cursorThread, FALSE);
	}
}

static
void
restoreCursor()
{
	// restore the show cursor in the window we hid it last
	if (g_cursor != NULL && g_cursorThread != 0) {
		DWORD myThread = GetCurrentThreadId();
		if (myThread != g_cursorThread)
			AttachThreadInput(myThread, g_cursorThread, TRUE);
		SetCursor(g_cursor);
		if (myThread != g_cursorThread)
			AttachThreadInput(myThread, g_cursorThread, FALSE);
	}
	g_cursor       = NULL;
	g_cursorThread = 0;
}

static
bool
keyboardHookHandler(WPARAM wParam, LPARAM lParam)
{
	// forward message to our window.  do this whether or not we're
	// forwarding events to clients because this'll keep our thread's
	// key state table up to date.  that's important for querying
	// the scroll lock toggle state.
	PostThreadMessage(g_threadID, SYNERGY_MSG_KEY, wParam, lParam);

	if (g_relay) {
		// let certain keys pass through
		switch (wParam) {
		case VK_CAPITAL:
		case VK_NUMLOCK:
		case VK_SCROLL:
			// pass event on.  we want to let these through to
			// the window proc because otherwise the keyboard
			// lights may not stay synchronized.
			break;

		default:
			// discard event
			return true;
		}
	}

	return false;
}

static
bool
mouseHookHandler(WPARAM wParam, SInt32 x, SInt32 y, SInt32 data)
{
	switch (wParam) {
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_NCLBUTTONDOWN:
	case WM_NCMBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
	case WM_NCXBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCXBUTTONDBLCLK:
	case WM_NCLBUTTONUP:
	case WM_NCMBUTTONUP:
	case WM_NCRBUTTONUP:
	case WM_NCXBUTTONUP:
		// always relay the event.  eat it if relaying.
		PostThreadMessage(g_threadID, SYNERGY_MSG_MOUSE_BUTTON, wParam, data);
		return g_relay;

	case WM_MOUSEWHEEL:
		if (g_relay) {
			// relay event
			PostThreadMessage(g_threadID, SYNERGY_MSG_MOUSE_WHEEL, data, 0);
		}
		return g_relay;

	case WM_NCMOUSEMOVE:
	case WM_MOUSEMOVE:
		if (g_relay) {
			// we want the cursor to be hidden at all times so we
			// hide the cursor on whatever window has it.  but then
			// we have to show the cursor whenever we leave that
			// window (or at some later time before we stop relaying).
			// so check the window with the cursor.  if it's not the
			// same window that had it before then show the cursor
			// in the last window and hide it in this window.
			DWORD thread = GetCurrentThreadId();
			if (thread != g_cursorThread) {
				restoreCursor();
				hideCursor(thread);
			}

			// relay and eat event
			PostThreadMessage(g_threadID, SYNERGY_MSG_MOUSE_MOVE, x, y);
			return true;
		}
		else {
			// check for mouse inside jump zone
			bool inside = false;
			if (!inside && (g_zoneSides & kLeftMask) != 0) {
				inside = (x < g_xScreen + g_zoneSize);
			}
			if (!inside && (g_zoneSides & kRightMask) != 0) {
				inside = (x >= g_xScreen + g_wScreen - g_zoneSize);
			}
			if (!inside && (g_zoneSides & kTopMask) != 0) {
				inside = (y < g_yScreen + g_zoneSize);
			}
			if (!inside && (g_zoneSides & kBottomMask) != 0) {
				inside = (y >= g_yScreen + g_hScreen - g_zoneSize);
			}

			// relay the event
			PostThreadMessage(g_threadID, SYNERGY_MSG_MOUSE_MOVE, x, y);

			// if inside then eat the event
			return inside;
		}
	}

	// pass the event
	return false;
}

static
LRESULT CALLBACK
keyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		// handle the message
		if (keyboardHookHandler(wParam, lParam)) {
			return 1;
		}
	}

	return CallNextHookEx(g_keyboard, code, wParam, lParam);
}

static
LRESULT CALLBACK
mouseHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		// decode message
		const MOUSEHOOKSTRUCT* info = (const MOUSEHOOKSTRUCT*)lParam;
		SInt32 x = (SInt32)info->pt.x;
		SInt32 y = (SInt32)info->pt.y;
		SInt32 w = 0;
		if (wParam == WM_MOUSEWHEEL) {
			// win2k and other systems supporting WM_MOUSEWHEEL in
			// the mouse hook are gratuitously different (and poorly
			// documented).  if a low-level mouse hook is in place
			// it should capture these events so we'll never see
			// them.
			switch (g_wheelSupport) {
			case kWheelModern:
				w = static_cast<SInt32>(LOWORD(info->dwExtraInfo));
				break;

			case kWheelWin2000: {
				const MOUSEHOOKSTRUCTWin2000* info2k =
						(const MOUSEHOOKSTRUCTWin2000*)lParam;
				w = static_cast<SInt32>(HIWORD(info2k->mouseData));
				break;
			}
			}
		}

		// handle the message.  note that we don't handle X buttons
		// here.  that's okay because they're only supported on
		// win2k and winxp and up and on those platforms we'll get
		// get the mouse events through the low level hook.
		if (mouseHookHandler(wParam, x, y, w)) {
			return 1;
		}
	}

	return CallNextHookEx(g_mouse, code, wParam, lParam);
}

static
LRESULT CALLBACK
getMessageHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		if (g_screenSaver) {
			MSG* msg = reinterpret_cast<MSG*>(lParam);
			if (msg->message == WM_SYSCOMMAND &&
				msg->wParam  == SC_SCREENSAVE) {
				// broadcast screen saver started message
				PostThreadMessage(g_threadID,
								SYNERGY_MSG_SCREEN_SAVER, TRUE, 0);
			}
		}
		if (g_relay) {
			MSG* msg = reinterpret_cast<MSG*>(lParam);
			if (msg->message == g_wmMouseWheel) {
				// post message to our window
				PostThreadMessage(g_threadID,
								SYNERGY_MSG_MOUSE_WHEEL, msg->wParam, 0);

				// zero out the delta in the message so it's (hopefully)
				// ignored
				msg->wParam = 0;
			}
		}
	}

	return CallNextHookEx(g_getMessage, code, wParam, lParam);
}

#if (_WIN32_WINNT >= 0x0400)

//
// low-level keyboard hook -- this allows us to capture and handle
// alt+tab, alt+esc, ctrl+esc, and windows key hot keys.  on the down
// side, key repeats are not reported to us.
//

static
LRESULT CALLBACK
keyboardLLHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		// decode the message
		KBDLLHOOKSTRUCT* info = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		WPARAM wParam = info->vkCode;
		LPARAM lParam = 1;							// repeat code
		lParam      |= (info->scanCode << 16);		// scan code
		if (info->flags & LLKHF_EXTENDED) {
			lParam  |= (1lu << 24);					// extended key
		}
		if (info->flags & LLKHF_ALTDOWN) {
			lParam  |= (1lu << 29);					// context code
		}
		if (info->flags & LLKHF_UP) {
			lParam  |= (1lu << 31);					// transition
		}
		// FIXME -- bit 30 should be set if key was already down but
		// we don't know that info.  as a result we'll never generate
		// key repeat events.

		// handle the message
		if (keyboardHookHandler(wParam, lParam)) {
			return 1;
		}
	}

	return CallNextHookEx(g_keyboardLL, code, wParam, lParam);
}

//
// low-level mouse hook -- this allows us to capture and handle mouse
// events very early.  the earlier the better.
//

static
LRESULT CALLBACK
mouseLLHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		// decode the message
		MSLLHOOKSTRUCT* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
		SInt32 x = (SInt32)info->pt.x;
		SInt32 y = (SInt32)info->pt.y;
		SInt32 w = (SInt32)HIWORD(info->mouseData);

		// handle the message
		if (mouseHookHandler(wParam, x, y, w)) {
			return 1;
		}
	}

	return CallNextHookEx(g_mouseLL, code, wParam, lParam);
}

static
DWORD WINAPI
getLowLevelProc(void*)
{
	// thread proc for low-level keyboard/mouse hooks.  this does
	// nothing but install the hook, process events, and uninstall
	// the hook.

	// force this thread to have a message queue
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

#if !NO_GRAB_KEYBOARD
	// install low-level keyboard hook
	g_keyboardLL = SetWindowsHookEx(WH_KEYBOARD_LL,
								&keyboardLLHook,
								g_hinstance,
								0);
	if (g_keyboardLL == NULL) {
		// indicate failure and exit
		g_hookThreadIDLL = 0;
		SetEvent(g_hookEventLL);
		return 1;
	}
#else
	// keep compiler quiet
	&keyboardLLHook;
#endif

	// install low-level mouse hook
	g_mouseLL = SetWindowsHookEx(WH_MOUSE_LL,
								&mouseLLHook,
								g_hinstance,
								0);
	if (g_mouseLL == NULL) {
		// indicate failure and exit
		if (g_keyboardLL != NULL) {
			UnhookWindowsHookEx(g_keyboardLL);
			g_keyboardLL     = NULL;
		}
		g_hookThreadIDLL = 0;
		SetEvent(g_hookEventLL);
		return 1;
	}

	// ready
	SetEvent(g_hookEventLL);

	// message loop
	bool done = false;
	while (!done) {
		switch (GetMessage(&msg, NULL, 0, 0)) {
		case -1:
			break;

		case 0:
			done = true;
			break;

		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			break;
		}
	}

	// uninstall hook
	UnhookWindowsHookEx(g_mouseLL);
	UnhookWindowsHookEx(g_keyboardLL);
	g_mouseLL    = NULL;
	g_keyboardLL = NULL;

	return 0;
}

#else // (_WIN32_WINNT < 0x0400)

static
DWORD WINAPI
getLowLevelProc(void*)
{
	g_hookThreadIDLL = 0;
	SetEvent(g_hookEventLL);
	return 1;
}

#endif

static
EWheelSupport
getWheelSupport()
{
	// get operating system
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof(info);
	if (!GetVersionEx(&info)) {
		return kWheelNone;
	}

	// see if modern wheel is present
	if (GetSystemMetrics(SM_MOUSEWHEELPRESENT)) {
		// note if running on win2k
		if (info.dwPlatformId   == VER_PLATFORM_WIN32_NT &&
			info.dwMajorVersion == 5 &&
			info.dwMinorVersion == 0) {
			return kWheelWin2000;
		}
		return kWheelModern;
	}

	// not modern.  see if we've got old-style support.
	UINT wheelSupportMsg    = RegisterWindowMessage(MSH_WHEELSUPPORT);
	HWND wheelSupportWindow = FindWindow(MSH_WHEELMODULE_CLASS,
										MSH_WHEELMODULE_TITLE);
	if (wheelSupportWindow != NULL && wheelSupportMsg != 0) {
		if (SendMessage(wheelSupportWindow, wheelSupportMsg, 0, 0) != 0) {
			g_wmMouseWheel = RegisterWindowMessage(MSH_MOUSEWHEEL);
			if (g_wmMouseWheel != 0) {
				return kWheelOld;
			}
		}
	}

	// assume modern.  we don't do anything special in this case
	// except respond to WM_MOUSEWHEEL messages.  GetSystemMetrics()
	// can apparently return FALSE even if a mouse wheel is present
	// though i'm not sure exactly when it does that (WinME returns
	// FALSE for my logitech USB trackball).
	return kWheelModern;
}


//
// external functions
//

BOOL WINAPI
DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);
		if (g_processID == 0) {
			g_hinstance = instance;
			g_processID = GetCurrentProcessId();
		}
	}
	else if (reason == DLL_PROCESS_DETACH) {
		if (g_processID == GetCurrentProcessId()) {
			if (g_keyboard   != NULL ||
				g_mouse      != NULL ||
				g_getMessage != NULL) {
				uninstall();
				uninstallScreenSaver();
			}
			g_processID = 0;
			g_hinstance = NULL;
		}
	}
	return TRUE;
}

extern "C" {

int
init(DWORD threadID)
{
	assert(g_hinstance != NULL);

	// try to open process that last called init() to see if it's
	// still running or if it died without cleaning up.
	if (g_processID != 0 && g_processID != GetCurrentProcessId()) {
		HANDLE process = OpenProcess(STANDARD_RIGHTS_REQUIRED,
								FALSE, g_processID);
		if (process != NULL) {
			// old process (probably) still exists so refuse to
			// reinitialize this DLL (and thus steal it from the
			// old process).
			CloseHandle(process);
			return 0;
		}

		// clean up after old process.  the system should've already
		// removed the hooks so we just need to reset our state.
		g_hinstance       = GetModuleHandle("synrgyhk");
		g_processID       = GetCurrentProcessId();
		g_wheelSupport    = kWheelNone;
		g_threadID        = 0;
		g_keyboard        = NULL;
		g_mouse           = NULL;
		g_getMessage      = NULL;
		g_hookThreadLL    = NULL;
		g_hookThreadIDLL  = 0;
		g_hookEventLL     = NULL;
		g_keyboardLL      = NULL;
		g_mouseLL         = NULL;
		g_screenSaver     = false;
	}

	// save thread id.  we'll post messages to this thread's
	// message queue.
	g_threadID     = threadID;

	// set defaults
	g_relay        = false;
	g_zoneSides    = 0;
	g_zoneSize     = 0;
	g_xScreen      = 0;
	g_yScreen      = 0;
	g_wScreen      = 0;
	g_hScreen      = 0;
	g_cursor       = NULL;
	g_cursorThread = 0;

	return 1;
}

int
cleanup(void)
{
	assert(g_hinstance != NULL);

	if (g_processID == GetCurrentProcessId()) {
		g_threadID = 0;
	}

	return 1;
}

EHookResult
install()
{
	assert(g_hinstance  != NULL);
	assert(g_keyboard   == NULL);
	assert(g_mouse      == NULL);
	assert(g_getMessage == NULL || g_screenSaver);

	// must be initialized
	if (g_threadID == 0) {
		return kHOOK_FAILED;
	}

	// check for mouse wheel support
	g_wheelSupport = getWheelSupport();

	// install GetMessage hook (unless already installed)
	if (g_wheelSupport == kWheelOld && g_getMessage == NULL) {
		g_getMessage = SetWindowsHookEx(WH_GETMESSAGE,
								&getMessageHook,
								g_hinstance,
								0);
	}

	// install low-level keyboard/mouse hooks, if possible.  since these
	// hooks are called in the context of the installing thread and that
	// thread must have a message loop but we don't want the caller's
	// message loop to do the work, we'll fire up a separate thread
	// just for the hooks.  note that low-level hooks are only available
	// on windows NT SP3 and above.
	g_hookEventLL = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_hookEventLL != NULL) {
		g_hookThreadLL = CreateThread(NULL, 0, &getLowLevelProc, 0,
								CREATE_SUSPENDED, &g_hookThreadIDLL);
		if (g_hookThreadLL != NULL) {
			// start the thread and wait for it to initialize
			ResumeThread(g_hookThreadLL);
			WaitForSingleObject(g_hookEventLL, INFINITE);
			ResetEvent(g_hookEventLL);

			// the thread clears g_hookThreadIDLL if it failed
			if (g_hookThreadIDLL == 0) {
				CloseHandle(g_hookThreadLL);
				g_hookThreadLL = NULL;
			}
		}
		if (g_hookThreadLL == NULL) {
			CloseHandle(g_hookEventLL);
			g_hookEventLL = NULL;
		}
	}

	// install non-low-level hooks if the low-level hooks are not installed
	if (g_hookThreadLL == NULL) {
#if !NO_GRAB_KEYBOARD
		g_keyboard = SetWindowsHookEx(WH_KEYBOARD,
								&keyboardHook,
								g_hinstance,
								0);
#else
		// keep compiler quiet
		&keyboardHook;
#endif
		g_mouse = SetWindowsHookEx(WH_MOUSE,
								&mouseHook,
								g_hinstance,
								0);
	}

	// check for any failures.  uninstall all hooks on failure.
	if (g_hookThreadLL == NULL &&
#if !NO_GRAB_KEYBOARD
		(g_keyboard == NULL || g_mouse == NULL)) {
#else
		(g_mouse == NULL)) {
#endif
		if (g_keyboard != NULL) {
			UnhookWindowsHookEx(g_keyboard);
			g_keyboard = NULL;
		}
		if (g_mouse != NULL) {
			UnhookWindowsHookEx(g_mouse);
			g_mouse = NULL;
		}
		if (g_getMessage != NULL && !g_screenSaver) {
			UnhookWindowsHookEx(g_getMessage);
			g_getMessage = NULL;
		}
		g_threadID = NULL;
		return kHOOK_FAILED;
	}

	return (g_hookThreadLL == NULL) ? kHOOK_OKAY : kHOOK_OKAY_LL;
}

int
uninstall(void)
{
	assert(g_hinstance != NULL);

	// uninstall hooks
	if (g_hookThreadLL != NULL) {
		PostThreadMessage(g_hookThreadIDLL, WM_QUIT, 0, 0);
		WaitForSingleObject(g_hookThreadLL, INFINITE);
		CloseHandle(g_hookEventLL);
		CloseHandle(g_hookThreadLL);
		g_hookEventLL    = NULL;
		g_hookThreadLL   = NULL;
		g_hookThreadIDLL = 0;
	}
	if (g_keyboard != NULL) {
		UnhookWindowsHookEx(g_keyboard);
		g_keyboard = NULL;
	}
	if (g_mouse != NULL) {
		UnhookWindowsHookEx(g_mouse);
		g_mouse = NULL;
	}
	if (g_getMessage != NULL && !g_screenSaver) {
		UnhookWindowsHookEx(g_getMessage);
		g_getMessage = NULL;
	}
	g_wheelSupport = kWheelNone;

	// show the cursor
	restoreCursor();

	return 1;
}

int
installScreenSaver(void)
{
	assert(g_hinstance != NULL);

	// must be initialized
	if (g_threadID == 0) {
		return 0;
	}

	// generate screen saver messages
	g_screenSaver = true;

	// install hook unless it's already installed
	if (g_getMessage == NULL) {
		g_getMessage = SetWindowsHookEx(WH_GETMESSAGE,
								&getMessageHook,
								g_hinstance,
								0);
	}

	return (g_getMessage != NULL) ? 1 : 0;
}

int
uninstallScreenSaver(void)
{
	assert(g_hinstance != NULL);

	// uninstall hook unless the mouse wheel hook is installed
	if (g_getMessage != NULL && g_wheelSupport != kWheelOld) {
		UnhookWindowsHookEx(g_getMessage);
		g_getMessage = NULL;
	}

	// screen saver hook is no longer installed
	g_screenSaver = false;

	return 1;
}

void
setSides(UInt32 sides)
{
	g_zoneSides = sides;
}

void
setZone(SInt32 x, SInt32 y, SInt32 w, SInt32 h, SInt32 jumpZoneSize)
{
	g_zoneSize = jumpZoneSize;
	g_xScreen  = x;
	g_yScreen  = y;
	g_wScreen  = w;
	g_hScreen  = h;
}

void
setRelay(int enable)
{
	if ((enable != 0) == g_relay) {
		// no change
		return;
	}
	g_relay = (enable != 0);
	if (!g_relay) {
		restoreCursor();
	}
}

}
