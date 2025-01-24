/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsHook.h"
#include "base/Log.h"
#include "deskflow/XScreen.h"
#include "deskflow/protocol_types.h"

static const char *g_name = "dfwhook";

static DWORD g_processID = 0;
static DWORD g_threadID = 0;
static HHOOK g_getMessage = NULL;
static HHOOK g_keyboardLL = NULL;
static HHOOK g_mouseLL = NULL;
static bool g_screenSaver = false;
static EHookMode g_mode = kHOOK_DISABLE;
static uint32_t g_zoneSides = 0;
static int32_t g_zoneSize = 0;
static int32_t g_xScreen = 0;
static int32_t g_yScreen = 0;
static int32_t g_wScreen = 0;
static int32_t g_hScreen = 0;
static WPARAM g_deadVirtKey = 0;
static WPARAM g_deadRelease = 0;
static LPARAM g_deadLParam = 0;
static BYTE g_deadKeyState[256] = {0};
static BYTE g_keyState[256] = {0};
static DWORD g_hookThread = 0;
static bool g_fakeServerInput = false;
static BOOL g_isPrimary = TRUE;

MSWindowsHook::MSWindowsHook()
{
}

MSWindowsHook::~MSWindowsHook()
{
  cleanup();

  if (g_processID == GetCurrentProcessId()) {
    uninstall();
    uninstallScreenSaver();
    g_processID = 0;
  }
}

void MSWindowsHook::loadLibrary()
{
  if (g_processID == 0) {
    g_processID = GetCurrentProcessId();
  }

  // initialize library
  if (init(GetCurrentThreadId()) == 0) {
    LOG((CLOG_ERR "failed to init %s.dll, another program may be using it", g_name));
    LOG((CLOG_INFO "restarting your computer may solve this error"));
    throw XScreenOpenFailure();
  }
}

int MSWindowsHook::init(DWORD threadID)
{
  // try to open process that last called init() to see if it's
  // still running or if it died without cleaning up.
  if (g_processID != 0 && g_processID != GetCurrentProcessId()) {
    HANDLE process = OpenProcess(STANDARD_RIGHTS_REQUIRED, FALSE, g_processID);
    if (process != NULL) {
      // old process (probably) still exists so refuse to
      // reinitialize this DLL (and thus steal it from the
      // old process).
      int result = CloseHandle(process);
      if (result == false) {
        return 0;
      }
    }

    // clean up after old process.  the system should've already
    // removed the hooks so we just need to reset our state.
    g_processID = GetCurrentProcessId();
    g_threadID = 0;
    g_getMessage = NULL;
    g_keyboardLL = NULL;
    g_mouseLL = NULL;
    g_screenSaver = false;
  }

  // save thread id.  we'll post messages to this thread's
  // message queue.
  g_threadID = threadID;

  // set defaults
  g_mode = kHOOK_DISABLE;
  g_zoneSides = 0;
  g_zoneSize = 0;
  g_xScreen = 0;
  g_yScreen = 0;
  g_wScreen = 0;
  g_hScreen = 0;

  return 1;
}

int MSWindowsHook::cleanup()
{
  if (g_processID == GetCurrentProcessId()) {
    g_threadID = 0;
  }

  return 1;
}

void MSWindowsHook::setSides(uint32_t sides)
{
  g_zoneSides = sides;
}

void MSWindowsHook::setZone(int32_t x, int32_t y, int32_t w, int32_t h, int32_t jumpZoneSize)
{
  g_zoneSize = jumpZoneSize;
  g_xScreen = x;
  g_yScreen = y;
  g_wScreen = w;
  g_hScreen = h;
}

void MSWindowsHook::setMode(EHookMode mode)
{
  if (mode == g_mode) {
    // no change
    return;
  }
  g_mode = mode;
}

static void keyboardGetState(BYTE keys[256], DWORD vkCode, bool kf_up)
{
  // we have to use GetAsyncKeyState() rather than GetKeyState() because
  // we don't pass through most keys so the event synchronous state
  // doesn't get updated.  we do that because certain modifier keys have
  // side effects, like alt and the windows key.
  if (vkCode < 0 || vkCode >= 256) {
    return;
  }

  // Keep track of key state on our own in case GetAsyncKeyState() fails
  g_keyState[vkCode] = kf_up ? 0 : 0x80;
  g_keyState[VK_SHIFT] = g_keyState[VK_LSHIFT] | g_keyState[VK_RSHIFT];

  SHORT key;
  // Test whether GetAsyncKeyState() is being honest with us
  key = GetAsyncKeyState(vkCode);

  if (key & 0x80) {
    // The only time we know for sure that GetAsyncKeyState() is working
    // is when it tells us that the current key is down.
    // In this case, update g_keyState to reflect what GetAsyncKeyState()
    // is telling us, just in case we have gotten out of sync

    for (int i = 0; i < 256; ++i) {
      key = GetAsyncKeyState(i);
      g_keyState[i] = (BYTE)((key < 0) ? 0x80u : 0);
    }
  }

  // copy g_keyState to keys
  for (int i = 0; i < 256; ++i) {
    keys[i] = g_keyState[i];
  }

  key = GetKeyState(VK_CAPITAL);
  keys[VK_CAPITAL] = (BYTE)(((key < 0) ? 0x80 : 0) | (key & 1));
}

static WPARAM makeKeyMsg(UINT virtKey, WCHAR wc, bool noAltGr)
{
  return MAKEWPARAM((WORD)wc, MAKEWORD(virtKey & 0xff, noAltGr ? 1 : 0));
}

static void setDeadKey(WCHAR wc[], int size, UINT flags)
{
  if (g_deadVirtKey != 0) {
    auto virtualKey = static_cast<UINT>(g_deadVirtKey);
    auto scanCode = static_cast<UINT>((g_deadLParam & 0x10ff0000u) >> 16);
    if (ToUnicode(virtualKey, scanCode, g_deadKeyState, wc, size, flags) >= 2) {
      // If ToUnicode returned >=2, it means that we accidentally removed
      // a double dead key instead of restoring it. Thus, we call
      // ToUnicode again with the same parameters to restore the
      // internal dead key state.
      ToUnicode(virtualKey, scanCode, g_deadKeyState, wc, size, flags);

      // We need to keep track of this because g_deadVirtKey will be
      // cleared later on; this would cause the dead key release to
      // incorrectly restore the dead key state.
      g_deadRelease = g_deadVirtKey;
    }
  }
}

static bool keyboardHookHandler(WPARAM wParam, LPARAM lParam)
{
  DWORD vkCode = static_cast<DWORD>(wParam);
  bool kf_up = (lParam & (KF_UP << 16)) != 0;

  // check for special events indicating if we should start or stop
  // passing events through and not report them to the server.  this
  // is used to allow the server to synthesize events locally but
  // not pick them up as user events.
  if (wParam == DESKFLOW_HOOK_FAKE_INPUT_VIRTUAL_KEY && ((lParam >> 16) & 0xffu) == DESKFLOW_HOOK_FAKE_INPUT_SCANCODE) {
    // update flag
    g_fakeServerInput = ((lParam & 0x80000000u) == 0);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, 0xff000000u | wParam, lParam);

    // discard event
    return true;
  }

  // if we're expecting fake input then just pass the event through
  // and do not forward to the server
  if (g_fakeServerInput) {
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, 0xfe000000u | wParam, lParam);
    return false;
  }

  // VK_RSHIFT may be sent with an extended scan code but right shift
  // is not an extended key so we reset that bit.
  if (wParam == VK_RSHIFT) {
    lParam &= ~0x01000000u;
  }

  // tell server about event
  PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, wParam, lParam);

  // ignore dead key release
  if ((g_deadVirtKey == wParam || g_deadRelease == wParam) && (lParam & 0x80000000u) != 0) {
    g_deadRelease = 0;
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, wParam | 0x04000000, lParam);
    return false;
  }

  // we need the keyboard state for ToAscii()
  BYTE keys[256];
  keyboardGetState(keys, vkCode, kf_up);

  // ToAscii() maps ctrl+letter to the corresponding control code
  // and ctrl+backspace to delete.  we don't want those translations
  // so clear the control modifier state.  however, if we want to
  // simulate AltGr (which is ctrl+alt) then we must not clear it.
  UINT control = keys[VK_CONTROL] | keys[VK_LCONTROL] | keys[VK_RCONTROL];
  UINT menu = keys[VK_MENU] | keys[VK_LMENU] | keys[VK_RMENU];
  if ((control & 0x80) == 0 || (menu & 0x80) == 0) {
    keys[VK_LCONTROL] = 0;
    keys[VK_RCONTROL] = 0;
    keys[VK_CONTROL] = 0;
  } else {
    keys[VK_LCONTROL] = 0x80;
    keys[VK_RCONTROL] = 0x80;
    keys[VK_CONTROL] = 0x80;
    keys[VK_LMENU] = 0x80;
    keys[VK_RMENU] = 0x80;
    keys[VK_MENU] = 0x80;
  }

  // ToAscii() needs to know if a menu is active for some reason.
  // we don't know and there doesn't appear to be any way to find
  // out.  so we'll just assume a menu is active if the menu key
  // is down.
  // FIXME -- figure out some way to check if a menu is active
  UINT flags = 0;
  if ((menu & 0x80) != 0)
    flags |= 1;

  // if we're on the server screen then just pass numpad keys with alt
  // key down as-is.  we won't pick up the resulting character but the
  // local app will.  if on a client screen then grab keys as usual;
  // if the client is a windows system it'll synthesize the expected
  // character.  if not then it'll probably just do nothing.
  if (g_mode != kHOOK_RELAY_EVENTS) {
    // we don't use virtual keys because we don't know what the
    // state of the numlock key is.  we'll hard code the scan codes
    // instead.  hopefully this works across all keyboards.
    UINT sc = (lParam & 0x01ff0000u) >> 16;
    if (menu && (sc >= 0x47u && sc <= 0x52u && sc != 0x4au && sc != 0x4eu)) {
      return false;
    }
  }

  // map the key event to a character.  we have to put the dead
  // key back first and this has the side effect of removing it.
  WCHAR wc[] = {0, 0};
  setDeadKey(wc, 2, flags);

  UINT scanCode = ((lParam & 0x10ff0000u) >> 16);
  int n = ToUnicode((UINT)wParam, scanCode, keys, wc, 2, flags);

  // if mapping failed and ctrl and alt are pressed then try again
  // with both not pressed.  this handles the case where ctrl and
  // alt are being used as individual modifiers rather than AltGr.
  // we note that's the case in the message sent back to deskflow
  // because there's no simple way to deduce it after the fact.
  // we have to put the dead key back first, if there was one.
  bool noAltGr = false;
  if (n == 0 && (control & 0x80) != 0 && (menu & 0x80) != 0) {
    noAltGr = true;
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, wParam | 0x05000000, lParam);
    setDeadKey(wc, 2, flags);

    BYTE keys2[256];
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
      keys2[i] = keys[i];
    }
    keys2[VK_LCONTROL] = 0;
    keys2[VK_RCONTROL] = 0;
    keys2[VK_CONTROL] = 0;
    keys2[VK_LMENU] = 0;
    keys2[VK_RMENU] = 0;
    keys2[VK_MENU] = 0;
    n = ToUnicode((UINT)wParam, scanCode, keys2, wc, 2, flags);
  }

  PostThreadMessage(
      g_threadID, DESKFLOW_MSG_DEBUG, (wc[0] & 0xffff) | ((wParam & 0xff) << 16) | ((n & 0xf) << 24) | 0x60000000,
      lParam
  );
  WPARAM charAndVirtKey = 0;
  bool clearDeadKey = false;
  switch (n) {
  default:
    // key is a dead key

    if (lParam & 0x80000000u)
      // This handles the obscure situation where a key has been
      // pressed which is both a dead key and a normal character
      // depending on which modifiers have been pressed. We
      // break here to prevent it from being considered a dead
      // key.
      break;

    g_deadVirtKey = wParam;
    g_deadLParam = lParam;
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
      g_deadKeyState[i] = keys[i];
    }
    break;

  case 0:
    // key doesn't map to a character.  this can happen if
    // non-character keys are pressed after a dead key.
    charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    break;

  case 1:
    // key maps to a character composed with dead key
    charAndVirtKey = makeKeyMsg((UINT)wParam, wc[0], noAltGr);
    clearDeadKey = true;
    break;

  case 2: {
    // previous dead key not composed.  send a fake key press
    // and release for the dead key to our window.
    WPARAM deadCharAndVirtKey = makeKeyMsg((UINT)g_deadVirtKey, wc[0], noAltGr);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_deadLParam & 0x7fffffffu);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_deadLParam | 0x80000000u);

    // use uncomposed character
    charAndVirtKey = makeKeyMsg((UINT)wParam, wc[1], noAltGr);
    clearDeadKey = true;
    break;
  }
  }

  // put back the dead key, if any, for the application to use
  if (g_deadVirtKey != 0) {
    ToUnicode((UINT)g_deadVirtKey, (g_deadLParam & 0x10ff0000u) >> 16, g_deadKeyState, wc, 2, flags);
  }

  // clear out old dead key state
  if (clearDeadKey) {
    g_deadVirtKey = 0;
    g_deadLParam = 0;
  }

  // forward message to our window.  do this whether or not we're
  // forwarding events to clients because this'll keep our thread's
  // key state table up to date.  that's important for querying
  // the scroll lock toggle state.
  // XXX -- with hot keys for actions we may only need to do this when
  // forwarding.
  if (charAndVirtKey != 0) {
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, charAndVirtKey | 0x07000000, lParam);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, charAndVirtKey, lParam);
  }

  if (g_mode == kHOOK_RELAY_EVENTS) {
    // let certain keys pass through
    switch (wParam) {
    case VK_CAPITAL:
    case VK_NUMLOCK:
    case VK_SCROLL:
      // pass event on.  we want to let these through to
      // the window proc because otherwise the keyboard
      // lights may not stay synchronized.
      break;

    case VK_HANGUL:
      // pass these modifiers if using a low level hook, discard
      // them if not.
      if (g_hookThread == 0) {
        return true;
      }
      break;

    default:
      // discard
      return true;
    }
  }

  return false;
}

#if !NO_GRAB_KEYBOARD
static LRESULT CALLBACK keyboardLLHook(int code, WPARAM wParam, LPARAM lParam)
{
  if (code >= 0) {
    // decode the message
    KBDLLHOOKSTRUCT *info = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);

    bool const injected = info->flags & LLKHF_INJECTED;
    if (!g_isPrimary && injected) {
      return CallNextHookEx(g_keyboardLL, code, wParam, lParam);
    }

    WPARAM wParam = info->vkCode;
    LPARAM lParam = 1;                // repeat code
    lParam |= (info->scanCode << 16); // scan code
    if (info->flags & LLKHF_EXTENDED) {
      lParam |= (1lu << 24); // extended key
    }
    if (info->flags & LLKHF_ALTDOWN) {
      lParam |= (1lu << 29); // context code
    }
    if (info->flags & LLKHF_UP) {
      lParam |= (1lu << 31); // transition
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
#endif

//
// low-level mouse hook -- this allows us to capture and handle mouse
// events very early.  the earlier the better.
//

static bool mouseHookHandler(WPARAM wParam, int32_t x, int32_t y, int32_t data)
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
    PostThreadMessage(g_threadID, DESKFLOW_MSG_MOUSE_BUTTON, wParam, data);
    return (g_mode == kHOOK_RELAY_EVENTS);

  case WM_MOUSEWHEEL:
    if (g_mode == kHOOK_RELAY_EVENTS) {
      // relay event
      PostThreadMessage(g_threadID, DESKFLOW_MSG_MOUSE_WHEEL, data, 0);
    }
    return (g_mode == kHOOK_RELAY_EVENTS);

  case WM_NCMOUSEMOVE:
  case WM_MOUSEMOVE:
    if (g_mode == kHOOK_RELAY_EVENTS) {
      // relay and eat event
      PostThreadMessage(g_threadID, DESKFLOW_MSG_MOUSE_MOVE, x, y);
      return true;
    } else if (g_mode == kHOOK_WATCH_JUMP_ZONE) {
      // low level hooks can report bogus mouse positions that are
      // outside of the screen.  jeez.  naturally we end up getting
      // fake motion in the other direction to get the position back
      // on the screen, which plays havoc with switch on double tap.
      // Server deals with that.  we'll clamp positions onto the
      // screen.  also, if we discard events for positions outside
      // of the screen then the mouse appears to get a bit jerky
      // near the edge.  we can either accept that or pass the bogus
      // events.  we'll try passing the events.
      bool bogus = false;
      if (x < g_xScreen) {
        x = g_xScreen;
        bogus = true;
      } else if (x >= g_xScreen + g_wScreen) {
        x = g_xScreen + g_wScreen - 1;
        bogus = true;
      }
      if (y < g_yScreen) {
        y = g_yScreen;
        bogus = true;
      } else if (y >= g_yScreen + g_hScreen) {
        y = g_yScreen + g_hScreen - 1;
        bogus = true;
      }

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
      PostThreadMessage(g_threadID, DESKFLOW_MSG_MOUSE_MOVE, x, y);

      // if inside and not bogus then eat the event
      return inside && !bogus;
    }
  }

  // pass the event
  return false;
}

static LRESULT CALLBACK mouseLLHook(int code, WPARAM wParam, LPARAM lParam)
{
  if (code >= 0) {
    // decode the message
    MSLLHOOKSTRUCT *info = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);

    bool const injected = info->flags & LLMHF_INJECTED;
    if (!g_isPrimary && injected) {
      return CallNextHookEx(g_mouseLL, code, wParam, lParam);
    }

    int32_t x = static_cast<int32_t>(info->pt.x);
    int32_t y = static_cast<int32_t>(info->pt.y);
    int32_t w = static_cast<int16_t>(HIWORD(info->mouseData));

    // handle the message
    if (mouseHookHandler(wParam, x, y, w)) {
      return 1;
    }
  }

  return CallNextHookEx(g_mouseLL, code, wParam, lParam);
}

EHookResult MSWindowsHook::install()
{
  assert(g_getMessage == NULL || g_screenSaver);

  // must be initialized
  if (g_threadID == 0) {
    return kHOOK_FAILED;
  }

  // discard old dead keys
  g_deadVirtKey = 0;
  g_deadLParam = 0;

  // reset fake input flag
  g_fakeServerInput = false;

  // install low-level hooks.  we require that they both get installed.
  g_mouseLL = SetWindowsHookEx(WH_MOUSE_LL, &mouseLLHook, NULL, 0);
#if !NO_GRAB_KEYBOARD
  g_keyboardLL = SetWindowsHookEx(WH_KEYBOARD_LL, &keyboardLLHook, NULL, 0);
  if (g_mouseLL == NULL || g_keyboardLL == NULL) {
    if (g_keyboardLL != NULL) {
      UnhookWindowsHookEx(g_keyboardLL);
      g_keyboardLL = NULL;
    }
    if (g_mouseLL != NULL) {
      UnhookWindowsHookEx(g_mouseLL);
      g_mouseLL = NULL;
    }
  }
#endif

  // check that we got all the hooks we wanted
  if ((g_mouseLL == NULL) ||
#if !NO_GRAB_KEYBOARD
      (g_keyboardLL == NULL)
#endif
  ) {
    uninstall();
    return kHOOK_FAILED;
  }

  if (g_keyboardLL != NULL || g_mouseLL != NULL) {
    g_hookThread = GetCurrentThreadId();
    return kHOOK_OKAY_LL;
  }

  return kHOOK_OKAY;
}

int MSWindowsHook::uninstall()
{
  // discard old dead keys
  g_deadVirtKey = 0;
  g_deadLParam = 0;

  // uninstall hooks
  if (g_keyboardLL != NULL) {
    UnhookWindowsHookEx(g_keyboardLL);
    g_keyboardLL = NULL;
  }
  if (g_mouseLL != NULL) {
    UnhookWindowsHookEx(g_mouseLL);
    g_mouseLL = NULL;
  }
  if (g_getMessage != NULL && !g_screenSaver) {
    UnhookWindowsHookEx(g_getMessage);
    g_getMessage = NULL;
  }

  return 1;
}

static LRESULT CALLBACK getMessageHook(int code, WPARAM wParam, LPARAM lParam)
{
  if (code >= 0) {
    if (g_screenSaver) {
      MSG *msg = reinterpret_cast<MSG *>(lParam);
      if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_SCREENSAVE) {
        // broadcast screen saver started message
        PostThreadMessage(g_threadID, DESKFLOW_MSG_SCREEN_SAVER, TRUE, 0);
      }
    }
  }

  return CallNextHookEx(g_getMessage, code, wParam, lParam);
}

int MSWindowsHook::installScreenSaver()
{
  // must be initialized
  if (g_threadID == 0) {
    return 0;
  }

  // generate screen saver messages
  g_screenSaver = true;

  // install hook unless it's already installed
  if (g_getMessage == NULL) {
    g_getMessage = SetWindowsHookEx(WH_GETMESSAGE, &getMessageHook, NULL, 0);
  }

  return (g_getMessage != NULL) ? 1 : 0;
}

int MSWindowsHook::uninstallScreenSaver()
{
  // uninstall hook unless the mouse wheel hook is installed
  if (g_getMessage != NULL) {
    UnhookWindowsHookEx(g_getMessage);
    g_getMessage = NULL;
  }

  // screen saver hook is no longer installed
  g_screenSaver = false;

  return 1;
}
