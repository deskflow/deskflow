/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- wm_hooks.cxx
//
// Window Message Hooks Dynamic Link library

#include <tchar.h>

#include <wm_hooks/wm_hooks.h>

UINT WM_HK_PingThread = RegisterWindowMessage(_T("RFB.WM_Hooks.PingThread"));

UINT WM_HK_WindowChanged = RegisterWindowMessage(_T("RFB.WM_Hooks.WindowChanged"));
UINT WM_Hooks_WindowChanged() {
  return WM_HK_WindowChanged;
}

UINT WM_HK_WindowClientAreaChanged = RegisterWindowMessage(_T("RFB.WM_Hooks.WindowClientAreaChanged"));
UINT WM_Hooks_WindowClientAreaChanged() {
  return WM_HK_WindowClientAreaChanged;
}

UINT WM_HK_WindowBorderChanged = RegisterWindowMessage(_T("RFB.WM_Hooks.WindowBorderChanged"));
UINT WM_Hooks_WindowBorderChanged() {
  return WM_HK_WindowBorderChanged;
}

UINT WM_HK_RectangleChanged = RegisterWindowMessage(_T("RFB.WM_Hooks.RectangleChanged"));
UINT WM_Hooks_RectangleChanged() {
  return WM_HK_RectangleChanged;
}

UINT WM_HK_CursorChanged = RegisterWindowMessage(_T("RFB.WM_Hooks.CursorChanged"));
UINT WM_Hooks_CursorChanged() {
  return WM_HK_CursorChanged;
}

#ifdef _DEBUG
UINT WM_HK_Diagnostic = RegisterWindowMessage(_T("RFB.WM_Hooks.Diagnostic"));
UINT WM_Hooks_Diagnostic() {
  return WM_HK_Diagnostic;
}
#endif

ATOM ATOM_Popup_Selection = GlobalAddAtom(_T("RFB.WM_Hooks.PopupSelectionAtom"));

//
// -=- DLL entry point
//

HINSTANCE dll_instance = 0;

BOOL WINAPI DllMain(HANDLE instance, ULONG reason, LPVOID reserved) {
  switch (reason) {
  case DLL_PROCESS_ATTACH:
    dll_instance = (HINSTANCE)instance;
    return TRUE;
  case DLL_PROCESS_DETACH:
    return TRUE;
  case DLL_THREAD_DETACH:
    WM_Hooks_Remove(GetCurrentThreadId());
    return TRUE;
  default:
    return TRUE;
  };
}

//
// -=- Display update hooks
//

#pragma data_seg(".WM_Hooks_Shared")
DWORD hook_owner = 0;
DWORD hook_target = 0;
HHOOK hook_CallWndProc = 0;
HHOOK hook_CallWndProcRet = 0;
HHOOK hook_GetMessage = 0;
HHOOK hook_DialogMessage = 0;
BOOL enable_cursor_shape = FALSE;
HCURSOR cursor = 0;
#ifdef _DEBUG
UINT diagnostic_min=1;
UINT diagnostic_max=0;
#endif
#pragma data_seg()

#ifdef _DEBUG
DLLEXPORT void WM_Hooks_SetDiagnosticRange(UINT min, UINT max) {
  diagnostic_min = min; diagnostic_max=max;
}
#endif

bool NotifyHookOwner(UINT event, WPARAM wParam, LPARAM lParam) {
  if (hook_owner) {
    return PostThreadMessage(hook_owner, event, wParam, lParam)!=0;
    /*
    if (last_event)
      return PostThreadMessage(hook_owner, last_event, last_wParam, last_lParam);
    last_event = event;
    last_wParam = wParam;
    last_lParam = lParam;
    return true;
    */
  }
  return false;
}

bool NotifyWindow(HWND hwnd, UINT msg) {
  return NotifyHookOwner(WM_HK_WindowChanged, msg, (LPARAM)hwnd);
}
bool NotifyWindowBorder(HWND hwnd, UINT msg) {
  return NotifyHookOwner(WM_HK_WindowBorderChanged, msg, (LPARAM)hwnd);
}
bool NotifyWindowClientArea(HWND hwnd, UINT msg) {
  return NotifyHookOwner(WM_HK_WindowClientAreaChanged, msg, (LPARAM)hwnd);
}
bool NotifyRectangle(RECT* rect) {
  WPARAM w = MAKELONG((SHORT)rect->left, (SHORT)rect->top);
  LPARAM l = MAKELONG((SHORT)rect->right, (SHORT)rect->bottom);
  return NotifyHookOwner(WM_HK_RectangleChanged, w, l);
}
bool NotifyCursor(HCURSOR cursor) {
  return NotifyHookOwner(WM_HK_CursorChanged, 0, (LPARAM)cursor);
}

void ProcessWindowMessage(UINT msg, HWND wnd, WPARAM wParam, LPARAM lParam) {
#ifdef _DEBUG
  if ((msg >= diagnostic_min) && (msg <= diagnostic_max))
    PostThreadMessage(hook_owner, WM_HK_Diagnostic, msg, (LPARAM)wnd);
#endif
  if (!IsWindowVisible(wnd)) return;
  switch (msg) {

    // -=- Border update events
	case WM_NCPAINT:
	case WM_NCACTIVATE:
    NotifyWindowBorder(wnd, msg);
		break;

    // -=- Client area update events
	case BM_SETCHECK:
	case BM_SETSTATE:
	case EM_SETSEL:
	case WM_CHAR:
	case WM_ENABLE:
	case WM_KEYUP:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_PALETTECHANGED:
	case WM_RBUTTONUP:
	case WM_SYSCOLORCHANGE:
	case WM_SETTEXT:
  case WM_SETFOCUS:
	//case WM_TIMER:
    NotifyWindowClientArea(wnd, msg);
    break;
	case WM_HSCROLL:
	case WM_VSCROLL:
		if (((int) LOWORD(wParam) == SB_THUMBTRACK) || ((int) LOWORD(wParam) == SB_ENDSCROLL))
			NotifyWindow(wnd, msg);
		break;

	case WM_WINDOWPOSCHANGING:
  case WM_DESTROY:
    {
      RECT wrect;
      if (GetWindowRect(wnd, &wrect)) {
        NotifyRectangle(&wrect);
      }
    }
    break;

  case WM_WINDOWPOSCHANGED:
    NotifyWindow(wnd, msg);
    break;

	case WM_PAINT:
    // *** could improve this
    NotifyWindowClientArea(wnd, msg);
    break;

    // Handle pop-up menus appearing
  case 482:
    NotifyWindow(wnd, 482);
    break;

    // Handle pop-up menus having items selected
	case 485:
		{
			HANDLE prop = GetProp(wnd, (LPCTSTR) MAKELONG(ATOM_Popup_Selection, 0));
      if (prop != (HANDLE) wParam) {
        NotifyWindow(wnd, 485);
				SetProp(wnd,
					(LPCTSTR) MAKELONG(ATOM_Popup_Selection, 0),
					(HANDLE) wParam);
			}
		}
		break;

  case WM_NCMOUSEMOVE:
  case WM_MOUSEMOVE:
    if (enable_cursor_shape) {
      HCURSOR new_cursor = GetCursor();
      if (new_cursor != cursor) {
        cursor = new_cursor;
        NotifyCursor(cursor);
      }
    }
    break;

    /* ***
		if (prf_use_GetUpdateRect)
		{
			HRGN region;
			region = CreateRectRgn(0, 0, 0, 0);

			// Get the affected region
			if (GetUpdateRgn(hWnd, region, FALSE) != ERROR)
			{
				int buffsize;
				UINT x;
				RGNDATA *buff;
				POINT TopLeft;

				// Get the top-left point of the client area
				TopLeft.x = 0;
				TopLeft.y = 0;
				if (!ClientToScreen(hWnd, &TopLeft))
					break;

				// Get the size of buffer required
				buffsize = GetRegionData(region, 0, 0);
				if (buffsize != 0)
				{
					buff = (RGNDATA *) new BYTE [buffsize];
					if (buff == NULL)
						break;

					// Now get the region data
					if(GetRegionData(region, buffsize, buff))
					{
						for (x=0; x<(buff->rdh.nCount); x++)
						{
							// Obtain the rectangles from the list
							RECT *urect = (RECT *) (((BYTE *) buff) + sizeof(RGNDATAHEADER) + (x * sizeof(RECT)));
							SendDeferredUpdateRect(
								hWnd,
								(SHORT) (TopLeft.x + urect->left),
								(SHORT) (TopLeft.y + urect->top),
								(SHORT) (TopLeft.x + urect->right),
								(SHORT) (TopLeft.y + urect->bottom)
								);
						}
					}

					delete [] buff;
				}
			}

			// Now free the region
			if (region != NULL)
				DeleteObject(region);
		}
    */
  };
}

LRESULT CALLBACK HookCallWndProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    CWPSTRUCT* info = (CWPSTRUCT*) lParam;
    ProcessWindowMessage(info->message, info->hwnd, info->wParam, info->lParam);
  }
  return CallNextHookEx(hook_CallWndProc, nCode, wParam, lParam);
}

LRESULT CALLBACK HookCallWndProcRet(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    CWPRETSTRUCT* info = (CWPRETSTRUCT*) lParam;
    ProcessWindowMessage(info->message, info->hwnd, info->wParam, info->lParam);
  }
  return CallNextHookEx(hook_CallWndProcRet, nCode, wParam, lParam);
}

LRESULT CALLBACK HookGetMessage(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    if (wParam & PM_REMOVE) {
      MSG* msg = (MSG*) lParam;
      ProcessWindowMessage(msg->message, msg->hwnd, msg->wParam, msg->lParam);
    }
  }
  return CallNextHookEx(hook_GetMessage, nCode, wParam, lParam);
}

LRESULT CALLBACK HookDialogMessage(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    MSG* msg = (MSG*) lParam;
    ProcessWindowMessage(msg->message, msg->hwnd, msg->wParam, msg->lParam);
  }
  return CallNextHookEx(hook_DialogMessage, nCode, wParam, lParam);
}

// - WM_Hooks_Install

BOOL WM_Hooks_Install(DWORD owner, DWORD thread) {
  // - Are there already hooks set?
  if (hook_owner) {
    if (!PostThreadMessage(hook_owner, WM_HK_PingThread, 0, 0)) {
      WM_Hooks_Remove(hook_owner);
    } else {
      return FALSE;
    }
  }

  // - Initialise the hooks
  hook_owner = owner;
  hook_target = thread;

  hook_CallWndProc = SetWindowsHookEx(WH_CALLWNDPROC, HookCallWndProc, dll_instance, thread);
  hook_CallWndProcRet = SetWindowsHookEx(WH_CALLWNDPROCRET, HookCallWndProcRet, dll_instance, thread);
  hook_GetMessage = SetWindowsHookEx(WH_GETMESSAGE, HookGetMessage, dll_instance, thread);
  hook_DialogMessage = SetWindowsHookEx(WH_SYSMSGFILTER, HookDialogMessage, dll_instance, thread);

  if (!hook_CallWndProc /*|| !hook_CallWndProcRet*/ || !hook_GetMessage || !hook_DialogMessage) {
    WM_Hooks_Remove(owner);
    return FALSE;
  }

  return TRUE;
}

// - WM_Hooks_Remove

BOOL WM_Hooks_Remove(DWORD owner) {
  if (owner != hook_owner) return FALSE;
  if (hook_CallWndProc) {
    UnhookWindowsHookEx(hook_CallWndProc);
    hook_CallWndProc = 0;
  }
  if (hook_CallWndProcRet) {
    UnhookWindowsHookEx(hook_CallWndProcRet);
    hook_CallWndProcRet = 0;
  }
  if (hook_GetMessage) {
    UnhookWindowsHookEx(hook_GetMessage);
    hook_GetMessage = 0;
  }
  if (hook_DialogMessage) {
    UnhookWindowsHookEx(hook_DialogMessage);
    hook_DialogMessage = 0;
  }
  hook_owner = 0;
  hook_target = 0;
  return TRUE;
}

//
// -=- User input hooks
//

#pragma data_seg(".WM_Hooks_Shared")
HHOOK hook_keyboard = 0;
HHOOK hook_pointer = 0;
bool enable_real_ptr = true;
bool enable_synth_ptr = true;
bool enable_real_kbd = true;
bool enable_synth_kbd = true;
#pragma data_seg()

#ifdef WH_KEYBOARD_LL
LRESULT CALLBACK HookKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) {
    KBDLLHOOKSTRUCT* info = (KBDLLHOOKSTRUCT*) lParam;
    bool real_event = (info->flags & LLKHF_INJECTED) == 0;
    if ((real_event && !enable_real_kbd) ||
      (!real_event && !enable_synth_kbd)) {
      return 1;
    }
  }
  return CallNextHookEx(hook_keyboard, nCode, wParam, lParam);
}

LRESULT CALLBACK HookPointerHook(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) {
    MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*) lParam;
    bool real_event = (info->flags & LLMHF_INJECTED) == 0;
    if ((real_event && !enable_real_ptr) ||
      (!real_event && !enable_synth_ptr)) {
      return 1;
    }
  }
  return CallNextHookEx(hook_keyboard, nCode, wParam, lParam);
}

bool RefreshInputHooks() {
  bool success = true;
  bool set_ptr_hook = !enable_real_ptr || !enable_synth_ptr;
  bool set_kbd_hook = !enable_real_kbd || !enable_synth_kbd;
  if (hook_keyboard && !set_kbd_hook) {
    UnhookWindowsHookEx(hook_keyboard);
    hook_keyboard = 0;
  }
  if (hook_pointer && !set_ptr_hook) {
    UnhookWindowsHookEx(hook_pointer);
    hook_pointer = 0;
  }
  if (!hook_keyboard && set_kbd_hook) {
    hook_keyboard = SetWindowsHookEx(WH_KEYBOARD_LL, HookKeyboardHook, dll_instance, 0);
    if (!hook_keyboard) success = false;
  }
  if (!hook_pointer && set_ptr_hook) {
    hook_pointer = SetWindowsHookEx(WH_MOUSE_LL, HookPointerHook, dll_instance, 0);
    if (!hook_pointer) success = false;
  }
  return success;
}
#else
#pragma message("  NOTE: low-level mouse and keyboard hooks not supported")
#endif

// - WM_Hooks_EnableRealInputs

BOOL WM_Hooks_EnableRealInputs(BOOL pointer, BOOL keyboard) {
#ifdef WH_KEYBOARD_LL
  enable_real_ptr = pointer!=0;
  enable_real_kbd = keyboard!=0;
  return RefreshInputHooks();
#else
  return FALSE;
#endif
}

// - WM_Hooks_EnableSynthInputs

BOOL WM_Hooks_EnableSynthInputs(BOOL pointer, BOOL keyboard) {
#ifdef WH_KEYBOARD_LL
  enable_synth_ptr = pointer!=0;
  enable_synth_kbd = keyboard!=0;
  return RefreshInputHooks();
#else
  return FALSE;
#endif
}

// - WM_Hooks_EnableCursorShape

BOOL WM_Hooks_EnableCursorShape(BOOL enable) {
  enable_cursor_shape = enable;
  return TRUE;
}
