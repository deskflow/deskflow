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

#include <windows.h>
#include <rfb_win32/LowLevelKeyEvents.h>
#include <rfb/Threading.h>
#include <rfb/LogWriter.h>
#include <list>

using namespace rfb;
using namespace win32;

static LogWriter vlog("LowLevelKeyEvents");


HHOOK hook = 0;
std::list<HWND> windows;
Mutex windowLock;


static bool filterKeyEvent(int vkCode) {
  switch (vkCode) {
  case VK_LWIN:
  case VK_RWIN:
  case VK_SNAPSHOT:
    return true;
  case VK_TAB:
    if (GetAsyncKeyState(VK_MENU) & 0x8000)
      return true;
  case VK_ESCAPE:
    if (GetAsyncKeyState(VK_MENU) & 0x8000)
      return true;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      return true;
  }
  return false;
}

LRESULT CALLBACK LowLevelKeyEventProc(int nCode,
                                      WPARAM wParam,
                                      LPARAM lParam) {
  if (nCode >= 0) {
    Lock l(windowLock);
    HWND foreground = GetForegroundWindow();
    std::list<HWND>::iterator i;
    for (i=windows.begin(); i!=windows.end(); i++) {
      if (*i == foreground) {
        UINT msgType = wParam;
        KBDLLHOOKSTRUCT* msgInfo = (KBDLLHOOKSTRUCT*)lParam;
        if (filterKeyEvent(msgInfo->vkCode)) {
          vlog.debug("filtered event %lx(%lu) %lu", msgInfo->vkCode, msgInfo->vkCode, wParam);
          PostMessage(*i, wParam, msgInfo->vkCode, (msgInfo->scanCode & 0xff) << 16);
          return 1;
        }
      }
    }
  }
  return CallNextHookEx(hook, nCode, wParam, lParam);
}


bool rfb::win32::enableLowLevelKeyEvents(HWND hwnd) {
// ***  return false; // *** THIS CODE IS EXPERIMENTAL, SO DISABLED BY DEFAULT!
  Lock l(windowLock);
  if (windows.empty() && !hook)
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyEventProc, GetModuleHandle(0), 0);
  if (hook)
    windows.push_back(hwnd);
  vlog.debug("enable %p -> %s", hwnd, hook ? "success" : "failure");
  return hook != 0;
}

void rfb::win32::disableLowLevelKeyEvents(HWND hwnd) {
  vlog.debug("disable %p", hwnd);
  Lock l(windowLock);
  windows.remove(hwnd);
  if (windows.empty() && hook) {
    UnhookWindowsHookEx(hook);
    hook = 0;
  }
}
