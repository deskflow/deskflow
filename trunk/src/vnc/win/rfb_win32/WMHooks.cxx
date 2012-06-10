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

// -=- WMHooks.cxx

#include <rfb_win32/WMHooks.h>
#include <rfb_win32/DynamicFn.h>
#include <rfb_win32/Service.h>
#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/IntervalTimer.h>
#include <rfb/Threading.h>
#include <rfb/LogWriter.h>

#include <list>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMHooks");

#define HOOK_DLL "vnc_wm_hooks.dll"

typedef UINT (*WM_Hooks_WMVAL_proto)();
typedef BOOL (*WM_Hooks_Install_proto)(DWORD owner, DWORD thread);
typedef BOOL (*WM_Hooks_Remove_proto)(DWORD owner);
typedef BOOL (*WM_Hooks_EnableCursorShape_proto)(BOOL enable);
#ifdef _DEBUG
typedef void (*WM_Hooks_SetDiagnosticRange_proto)(UINT min, UINT max);
DynamicFn<WM_Hooks_SetDiagnosticRange_proto> WM_Hooks_SetDiagnosticRange(_T(HOOK_DLL), "WM_Hooks_SetDiagnosticRange");
#endif


class WMHooksThread : public Thread {
public:
  WMHooksThread() : Thread("WMHookThread"), active(true),
    WM_Hooks_Install(_T(HOOK_DLL), "WM_Hooks_Install"),
    WM_Hooks_Remove(_T(HOOK_DLL), "WM_Hooks_Remove"),
    WM_Hooks_EnableCursorShape(_T(HOOK_DLL), "WM_Hooks_EnableCursorShape") {
  }
  virtual void run();
  virtual Thread* join();
  DynamicFn<WM_Hooks_Install_proto> WM_Hooks_Install;;
  DynamicFn<WM_Hooks_Remove_proto> WM_Hooks_Remove;
  DynamicFn<WM_Hooks_EnableCursorShape_proto> WM_Hooks_EnableCursorShape;
protected:
  bool active;
};

WMHooksThread* hook_mgr = 0;
std::list<WMHooks*> hooks;
std::list<WMCursorHooks*> cursor_hooks;
Mutex hook_mgr_lock;
HCURSOR hook_cursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);


static bool StartHookThread() {
  if (hook_mgr)
    return true;
  vlog.debug("creating thread");
  hook_mgr = new WMHooksThread();
  if (!hook_mgr->WM_Hooks_Install.isValid() ||
      !hook_mgr->WM_Hooks_Remove.isValid()) {
    vlog.debug("hooks not available");
    return false;
  }
  vlog.debug("installing hooks");
  if (!(*hook_mgr->WM_Hooks_Install)(hook_mgr->getThreadId(), 0)) {
    vlog.error("failed to initialise hooks");
    delete hook_mgr->join();
    hook_mgr = 0;
    return false;
  }
  vlog.debug("starting thread");
  hook_mgr->start();
  return true;
}

static void StopHookThread() {
  if (!hook_mgr)
    return;
  if (!hooks.empty() || !cursor_hooks.empty())
    return;
  vlog.debug("closing thread");
  delete hook_mgr->join();
  hook_mgr = 0;
}


static bool AddHook(WMHooks* hook) {
  vlog.debug("adding hook");
  Lock l(hook_mgr_lock);
  if (!StartHookThread())
    return false;
  hooks.push_back(hook);
  return true;
}

static bool AddCursorHook(WMCursorHooks* hook) {
  vlog.debug("adding cursor hook");
  Lock l(hook_mgr_lock);
  if (!StartHookThread())
    return false;
  if (!hook_mgr->WM_Hooks_EnableCursorShape.isValid())
    return false;
  if (cursor_hooks.empty() && !(*hook_mgr->WM_Hooks_EnableCursorShape)(TRUE))
    return false;
  cursor_hooks.push_back(hook);
  return true;
}

static bool RemHook(WMHooks* hook) {
  {
    vlog.debug("removing hook");
    Lock l(hook_mgr_lock);
    hooks.remove(hook);
  }
  StopHookThread();
  return true;
}

static bool RemCursorHook(WMCursorHooks* hook) {
  {
    vlog.debug("removing cursor hook");
    Lock l(hook_mgr_lock);
    cursor_hooks.remove(hook);
    if (hook_mgr->WM_Hooks_EnableCursorShape.isValid() &&
        cursor_hooks.empty())
      (*hook_mgr->WM_Hooks_EnableCursorShape)(FALSE);
  }
  StopHookThread();
  return true;
}

static void NotifyHooksRegion(const Region& r) {
  Lock l(hook_mgr_lock);
  std::list<WMHooks*>::iterator i;
  for (i=hooks.begin(); i!=hooks.end(); i++)
    (*i)->NotifyHooksRegion(r);
}

static void NotifyHooksCursor(HCURSOR c) {
  Lock l(hook_mgr_lock);
  hook_cursor = c;
}


static UINT GetMsgVal(DynamicFn<WM_Hooks_WMVAL_proto>& fn) {
  if (fn.isValid())
    return (*fn)();
  return WM_NULL;
}

void
WMHooksThread::run() {
  // Obtain message ids for all supported hook messages
  DynamicFn<WM_Hooks_WMVAL_proto> WM_Hooks_WindowChanged(_T(HOOK_DLL), "WM_Hooks_WindowChanged");
  DynamicFn<WM_Hooks_WMVAL_proto> WM_Hooks_WindowBorderChanged(_T(HOOK_DLL), "WM_Hooks_WindowBorderChanged");
  DynamicFn<WM_Hooks_WMVAL_proto> WM_Hooks_WindowClientAreaChanged(_T(HOOK_DLL), "WM_Hooks_WindowClientAreaChanged");
  DynamicFn<WM_Hooks_WMVAL_proto> WM_Hooks_RectangleChanged(_T(HOOK_DLL), "WM_Hooks_RectangleChanged");
  DynamicFn<WM_Hooks_WMVAL_proto> WM_Hooks_CursorChanged(_T(HOOK_DLL), "WM_Hooks_CursorChanged");
  UINT windowMsg = GetMsgVal(WM_Hooks_WindowChanged);
  UINT clientAreaMsg = GetMsgVal(WM_Hooks_WindowClientAreaChanged);
  UINT borderMsg = GetMsgVal(WM_Hooks_WindowBorderChanged);
  UINT rectangleMsg = GetMsgVal(WM_Hooks_RectangleChanged);
  UINT cursorMsg = GetMsgVal(WM_Hooks_CursorChanged);
#ifdef _DEBUG
  DynamicFn<WM_Hooks_WMVAL_proto> WM_Hooks_Diagnostic(_T(HOOK_DLL), "WM_Hooks_Diagnostic");
  UINT diagnosticMsg = GetMsgVal(WM_Hooks_Diagnostic);
#endif
  MSG msg;
  RECT wrect;
  HWND hwnd;
  int count = 0;

  // Update delay handling
  //   We delay updates by 40-80ms, so that the triggering application has time to
  //   actually complete them before we notify the hook callbacks & they go off
  //   capturing screen state.
  const int updateDelayMs = 40;
  MsgWindow updateDelayWnd(_T("WMHooks::updateDelay"));
  IntervalTimer updateDelayTimer(updateDelayWnd.getHandle(), 1);
  Region updates[2];
  int activeRgn = 0;

  vlog.debug("starting hook thread");

  while (active && GetMessage(&msg, NULL, 0, 0)) {
    count++;

    if (msg.message == WM_TIMER) {
      // Actually notify callbacks of graphical updates
      NotifyHooksRegion(updates[1-activeRgn]);
      if (updates[activeRgn].is_empty())
        updateDelayTimer.stop();
      activeRgn = 1-activeRgn;
      updates[activeRgn].clear();

    } else if (msg.message == windowMsg) {
      // An entire window has (potentially) changed
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
        GetWindowRect(hwnd, &wrect) && !IsRectEmpty(&wrect)) {
          updates[activeRgn].assign_union(Rect(wrect.left, wrect.top,
                                               wrect.right, wrect.bottom));
          updateDelayTimer.start(updateDelayMs);
      }

    } else if (msg.message == clientAreaMsg) {
      // The client area of a window has (potentially) changed
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
          GetClientRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
      {
        POINT pt = {0,0};
        if (ClientToScreen(hwnd, &pt)) {
          updates[activeRgn].assign_union(Rect(wrect.left+pt.x, wrect.top+pt.y,
                                               wrect.right+pt.x, wrect.bottom+pt.y));
          updateDelayTimer.start(updateDelayMs);
        }
      }

    } else if (msg.message == borderMsg) {
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
          GetWindowRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
      {
        Region changed(Rect(wrect.left, wrect.top, wrect.right, wrect.bottom));
        RECT crect;
        POINT pt = {0,0};
        if (GetClientRect(hwnd, &crect) && ClientToScreen(hwnd, &pt) &&
            !IsRectEmpty(&crect))
        {
          changed.assign_subtract(Rect(crect.left+pt.x, crect.top+pt.y,
                                       crect.right+pt.x, crect.bottom+pt.y));
        }
        if (!changed.is_empty()) {
          updates[activeRgn].assign_union(changed);
          updateDelayTimer.start(updateDelayMs);
        }
      }
    } else if (msg.message == rectangleMsg) {
      Rect r = Rect(LOWORD(msg.wParam), HIWORD(msg.wParam),
                    LOWORD(msg.lParam), HIWORD(msg.lParam));
      if (!r.is_empty()) {
        updates[activeRgn].assign_union(r);
        updateDelayTimer.start(updateDelayMs);
      }

    } else if (msg.message == cursorMsg) {
      NotifyHooksCursor((HCURSOR)msg.lParam);
#ifdef _DEBUG
    } else if (msg.message == diagnosticMsg) {
      vlog.info("DIAG msg=%x(%d) wnd=%lx", msg.wParam, msg.wParam, msg.lParam);
#endif
    }
  }

  vlog.debug("stopping hook thread - processed %d events", count);
  (*WM_Hooks_Remove)(getThreadId());
}

Thread*
WMHooksThread::join() {
  vlog.debug("stopping WMHooks thread");
  active = false;
  PostThreadMessage(thread_id, WM_QUIT, 0, 0);
  vlog.debug("joining WMHooks thread");
  return Thread::join();
}

// -=- WMHooks class

rfb::win32::WMHooks::WMHooks() : updateEvent(0) {
}

rfb::win32::WMHooks::~WMHooks() {
  RemHook(this);
}

bool rfb::win32::WMHooks::setEvent(HANDLE ue) {
  if (updateEvent)
    RemHook(this);
  updateEvent = ue;
  return AddHook(this);
}

bool rfb::win32::WMHooks::getUpdates(UpdateTracker* ut) {
  if (!updatesReady) return false;
  Lock l(hook_mgr_lock);
  updates.copyTo(ut);
  updates.clear();
  updatesReady = false;
  return true;
}

bool rfb::win32::WMHooks::areAvailable() {
  WMHooksThread wmht;
  return wmht.WM_Hooks_Install.isValid();
}

#ifdef _DEBUG
void
rfb::win32::WMHooks::setDiagnosticRange(UINT min, UINT max) {
  if (WM_Hooks_SetDiagnosticRange.isValid())
    (*WM_Hooks_SetDiagnosticRange)(min, max);
}
#endif

void rfb::win32::WMHooks::NotifyHooksRegion(const Region& r) {
  // hook_mgr_lock is already held at this point
  updates.add_changed(r);
  updatesReady = true;
  SetEvent(updateEvent);
}


// -=- WMBlockInput class

rfb::win32::WMBlockInput::WMBlockInput() : active(false) {
}

rfb::win32::WMBlockInput::~WMBlockInput() {
  blockInputs(false);
}

typedef BOOL (*WM_Hooks_EnableRealInputs_proto)(BOOL pointer, BOOL keyboard);
DynamicFn<WM_Hooks_EnableRealInputs_proto>* WM_Hooks_EnableRealInputs = 0;
static bool blockRealInputs(bool block_) {
  // NB: Requires blockMutex to be held!
  if (block_) {
    if (WM_Hooks_EnableRealInputs)
      return true;
    // Enable blocking
    WM_Hooks_EnableRealInputs = new DynamicFn<WM_Hooks_EnableRealInputs_proto>(_T(HOOK_DLL), "WM_Hooks_EnableRealInputs");
    if (WM_Hooks_EnableRealInputs->isValid() && (**WM_Hooks_EnableRealInputs)(false, false))
      return true;
  }
  if (WM_Hooks_EnableRealInputs) {
    // Clean up the DynamicFn, either if init failed, or block_ is false
    if (WM_Hooks_EnableRealInputs->isValid())
      (**WM_Hooks_EnableRealInputs)(true, true);
    delete WM_Hooks_EnableRealInputs;
    WM_Hooks_EnableRealInputs = 0;
  }
  return block_ == (WM_Hooks_EnableRealInputs != 0);
}

Mutex blockMutex;
int blockCount = 0;

bool rfb::win32::WMBlockInput::blockInputs(bool on) {
  if (active == on) return true;
  Lock l(blockMutex);
  int newCount = on ? blockCount+1 : blockCount-1;
  if (!blockRealInputs(newCount > 0))
    return false;
  blockCount = newCount;
  active = on;
  return true;
}


// -=- WMCursorHooks class

rfb::win32::WMCursorHooks::WMCursorHooks() {
}

rfb::win32::WMCursorHooks::~WMCursorHooks() {
  RemCursorHook(this);
}

bool
rfb::win32::WMCursorHooks::start() {
  return AddCursorHook(this);
}

HCURSOR
rfb::win32::WMCursorHooks::getCursor() const {
  return hook_cursor;
}
