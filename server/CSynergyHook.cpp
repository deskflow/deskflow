#include "CSynergyHook.h"
#include "CScreenMap.h"
#include <assert.h>

//
// globals
//

#pragma comment(linker, "-section:shared,rws")
#pragma data_seg("shared")
// all data in this shared section *must* be initialized

static HINSTANCE		g_hinstance = NULL;
static DWORD			g_process = NULL;
static HWND				g_hwnd = NULL;
static HHOOK			g_keyboard = NULL;
static HHOOK			g_mouse = NULL;
static bool				g_relay = false;
static SInt32			g_zoneSize = 0;
static UInt32			g_zoneSides = 0;
static SInt32			g_wScreen = 0;
static SInt32			g_hScreen = 0;
static HCURSOR			g_cursor = NULL;
static DWORD			g_cursorThread = 0;

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
		// FIXME
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
			if (!inside && (g_zoneSides & CScreenMap::kLeftMask) != 0) {
				inside = (x < g_zoneSize);
			}
			if (!inside && (g_zoneSides & CScreenMap::kRightMask) != 0) {
				inside = (x >= g_wScreen - g_zoneSize);
			}
			if (!inside && (g_zoneSides & CScreenMap::kTopMask) != 0) {
				inside = (y < g_zoneSize);
			}
			if (!inside && (g_zoneSides & CScreenMap::kBottomMask) != 0) {
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
			if (g_keyboard != NULL || g_mouse != NULL) {
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
	assert(g_hinstance != NULL);
	assert(g_keyboard  == NULL);
	assert(g_mouse     == NULL);

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

	return 1;
}

int						uninstall(void)
{
	assert(g_keyboard != NULL);
	assert(g_mouse    != NULL);

	// uninstall hooks
	UnhookWindowsHookEx(g_keyboard);
	UnhookWindowsHookEx(g_mouse);
	g_keyboard = NULL;
	g_mouse    = NULL;
	g_hwnd     = NULL;

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
