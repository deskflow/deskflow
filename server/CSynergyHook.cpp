#include "CSynergyHook.h"
#include "CConfig.h"
#include <assert.h>
#include <zmouse.h>

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


//
// globals
//

#pragma comment(linker, "-section:shared,rws")
#pragma data_seg("shared")
// all data in this shared section *must* be initialized

static HINSTANCE		g_hinstance = NULL;
static DWORD			g_process = NULL;
static EWheelSupport	g_wheelSupport = kWheelNone;
static UINT				g_wmMouseWheel = 0;
static HWND				g_hwnd = NULL;
static HHOOK			g_keyboard = NULL;
static HHOOK			g_mouse = NULL;
static HHOOK			g_cbt = NULL;
static HHOOK			g_getMessage = NULL;
static bool				g_relay = false;
static SInt32			g_zoneSize = 0;
static UInt32			g_zoneSides = 0;
static SInt32			g_wScreen = 0;
static SInt32			g_hScreen = 0;
static HCURSOR			g_cursor = NULL;
static DWORD			g_cursorThread = 0;

static int foo = 0;

#pragma data_seg()

//
// internal functions
//

static void				hideCursor(DWORD thread)
{
	// we should be running the context of the window who's cursor
	// we want to hide so we shouldn't have to attach thread input.
	g_cursor       = GetCursor();
	g_cursorThread = thread;
	SetCursor(NULL);
}

static void				restoreCursor()
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

static LRESULT CALLBACK	keyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		if (g_relay) {
			// forward message to our window
			PostMessage(g_hwnd, SYNERGY_MSG_KEY, wParam, lParam);

/* XXX -- this doesn't seem to work/help with lost key events
			// if the active window isn't our window then make it
			// active.
			const bool wrongFocus = (GetActiveWindow() != g_hwnd);
			if (wrongFocus) {
				SetForegroundWindow(g_hwnd);
			}
*/

			// let certain keys pass through
			switch (wParam) {
			case VK_CAPITAL:
			case VK_NUMLOCK:
			case VK_SCROLL:
				// pass event
				break;

			default:
				// discard event
				return 1;
			}
		}
	}

	return CallNextHookEx(g_keyboard, code, wParam, lParam);
}

static LRESULT CALLBACK	mouseHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		if (g_relay) {
			switch (wParam) {
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
				PostMessage(g_hwnd, SYNERGY_MSG_MOUSE_BUTTON, wParam, 0);
				return 1;

			case WM_MOUSEWHEEL: {
				// win2k and other systems supporting WM_MOUSEWHEEL in
				// the mouse hook are gratuitously different (and poorly
				// documented).
				switch (g_wheelSupport) {
				case kWheelModern: {
					const MOUSEHOOKSTRUCT* info =
								(const MOUSEHOOKSTRUCT*)lParam;
					PostMessage(g_hwnd, SYNERGY_MSG_MOUSE_WHEEL,
								static_cast<short>(LOWORD(info->dwExtraInfo)), 0);
					break;
				}

				case kWheelWin2000: {
					const MOUSEHOOKSTRUCTWin2000* info =
								(const MOUSEHOOKSTRUCTWin2000*)lParam;
					PostMessage(g_hwnd, SYNERGY_MSG_MOUSE_WHEEL,
								static_cast<short>(HIWORD(info->mouseData)), 0);
					break;
				}

				default:
					break;
				}
				return 1;
			}

			case WM_MOUSEMOVE: {
				const MOUSEHOOKSTRUCT* info = (const MOUSEHOOKSTRUCT*)lParam;
				SInt32 x = (SInt32)info->pt.x;
				SInt32 y = (SInt32)info->pt.y;

				// we want the cursor to be hidden at all times so we
				// hide the cursor on whatever window has it.  but then
				// we have to show the cursor whenever we leave that
				// window (or at some later time before we stop relaying).
				// so check the window with the cursor.  if it's not the
				// same window that had it before then show the cursor
				// in the last window and hide it in this window.
				DWORD thread = GetWindowThreadProcessId(info->hwnd, NULL);
				if (thread != g_cursorThread) {
					restoreCursor();
					hideCursor(thread);
				}

				// relay the motion
				PostMessage(g_hwnd, SYNERGY_MSG_MOUSE_MOVE, x, y);
				return 1;
			}
			}
		}
		else {
			// check for mouse inside jump zone
			bool inside = false;
			const MOUSEHOOKSTRUCT* info = (const MOUSEHOOKSTRUCT*)lParam;
			SInt32 x = (SInt32)info->pt.x;
			SInt32 y = (SInt32)info->pt.y;
			if (!inside && (g_zoneSides & CConfig::kLeftMask) != 0) {
				inside = (x < g_zoneSize);
			}
			if (!inside && (g_zoneSides & CConfig::kRightMask) != 0) {
				inside = (x >= g_wScreen - g_zoneSize);
			}
			if (!inside && (g_zoneSides & CConfig::kTopMask) != 0) {
				inside = (y < g_zoneSize);
			}
			if (!inside && (g_zoneSides & CConfig::kBottomMask) != 0) {
				inside = (y >= g_hScreen - g_zoneSize);
			}

			// if inside then eat event and notify our window
			if (inside) {
				restoreCursor();
				PostMessage(g_hwnd, SYNERGY_MSG_MOUSE_MOVE, x, y);
				return 1;
			}
		}
	}

	return CallNextHookEx(g_mouse, code, wParam, lParam);
}

static LRESULT CALLBACK	cbtHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		if (g_relay) {
			switch (code) {
			case HCBT_ACTIVATE:
			case HCBT_SETFOCUS:
				// discard unless activating our window
				if (reinterpret_cast<HWND>(wParam) != g_hwnd) {
					return 1;
				}
				break;
			}
		}
	}

	return CallNextHookEx(g_cbt, code, wParam, lParam);
}

static LRESULT CALLBACK	getMessageHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		if (g_relay) {
			MSG* msg = reinterpret_cast<MSG*>(lParam);
			if (msg->message == g_wmMouseWheel) {
				// post message to our window
				PostMessage(g_hwnd, SYNERGY_MSG_MOUSE_WHEEL, msg->wParam, 0);

				// zero out the delta in the message so it's (hopefully)
				// ignored
				msg->wParam = 0;
			}
		}
	}

	return CallNextHookEx(g_getMessage, code, wParam, lParam);
}

static EWheelSupport	GetWheelSupport()
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
	// though i'm not sure exactly when it does that (but WinME does
	// for my logitech usb trackball).
	return kWheelModern;
}


//
// external functions
//

BOOL WINAPI				DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) {
		if (g_hinstance == NULL) {
			g_hinstance = instance;
			g_process   = GetCurrentProcessId();
		}
	}
	else if (reason == DLL_PROCESS_DETACH) {
		if (g_process == GetCurrentProcessId()) {
			if (g_keyboard != NULL || g_mouse != NULL || g_cbt != NULL) {
				uninstall();
			}
			g_process = NULL;
		}
	}
	return TRUE;
}

extern "C" {

int						install(HWND hwnd)
{
	assert(g_hinstance    != NULL);
	assert(g_keyboard     == NULL);
	assert(g_mouse        == NULL);
	assert(g_cbt          == NULL);
	assert(g_wheelSupport != kWheelOld || g_getMessage == NULL);

	// save window
	g_hwnd = hwnd;

	// set defaults
	g_relay        = false;
	g_zoneSize     = 0;
	g_zoneSides    = 0;
	g_wScreen      = 0;
	g_hScreen      = 0;
	g_cursor       = NULL;
	g_cursorThread = 0;

	// check for mouse wheel support
	g_wheelSupport = GetWheelSupport();

	// install keyboard hook
	g_keyboard = SetWindowsHookEx(WH_KEYBOARD,
								&keyboardHook,
								g_hinstance,
								0);
	if (g_keyboard == NULL) {
		g_hwnd = NULL;
		return 0;
	}

	// install mouse hook
	g_mouse = SetWindowsHookEx(WH_MOUSE,
								&mouseHook,
								g_hinstance,
								0);
	if (g_mouse == NULL) {
		// uninstall keyboard hook before failing
		UnhookWindowsHookEx(g_keyboard);
		g_keyboard = NULL;
		g_hwnd     = NULL;
		return 0;
	}

	// install CBT hook
	g_cbt = SetWindowsHookEx(WH_CBT,
								&cbtHook,
								g_hinstance,
								0);
	if (g_cbt == NULL) {
		// uninstall keyboard and mouse hooks before failing
		UnhookWindowsHookEx(g_keyboard);
		UnhookWindowsHookEx(g_mouse);
		g_keyboard = NULL;
		g_mouse    = NULL;
		g_hwnd     = NULL;
		return 0;
	}

	// install GetMessage hook
	if (g_wheelSupport == kWheelOld) {
		g_getMessage = SetWindowsHookEx(WH_GETMESSAGE,
								&getMessageHook,
								g_hinstance,
								0);
		// ignore failure;  we just won't get mouse wheel messages
	}

	return 1;
}

int						uninstall(void)
{
	assert(g_keyboard != NULL);
	assert(g_mouse    != NULL);
	assert(g_cbt      != NULL);

	// uninstall hooks
	UnhookWindowsHookEx(g_keyboard);
	UnhookWindowsHookEx(g_mouse);
	UnhookWindowsHookEx(g_cbt);
	if (g_getMessage != NULL) {
		UnhookWindowsHookEx(g_getMessage);
	}
	g_keyboard   = NULL;
	g_mouse      = NULL;
	g_cbt        = NULL;
	g_getMessage = NULL;
	g_hwnd       = NULL;

	// show the cursor
	restoreCursor();

	return 1;
}

void					setZone(UInt32 sides,
								SInt32 w, SInt32 h, SInt32 jumpZoneSize)
{
	g_zoneSize  = jumpZoneSize;
	g_zoneSides = sides;
	g_wScreen   = w;
	g_hScreen   = h;
	g_relay     = false;
	restoreCursor();
}

void					setRelay(void)
{
	g_relay     = true;
	g_zoneSize  = 0;
	g_zoneSides = 0;
	g_wScreen   = 0;
	g_hScreen   = 0;
}

}
