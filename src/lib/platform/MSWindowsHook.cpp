/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsHook.h"
#include "base/DirectionTypes.h"
#include "base/Log.h"
#include "deskflow/ScreenException.h"
#include "platform/MSWindowsDeadKeyUtils.h"

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

static const char *g_name = "dfwhook";

static DWORD g_processID = 0;
static DWORD g_threadID = 0;
static HHOOK g_getMessage = nullptr;
static HHOOK g_keyboardLL = nullptr;
static HHOOK g_mouseLL = nullptr;
static bool g_screenSaver = false;
static EHookMode g_mode = kHOOK_DISABLE;
static uint32_t g_zoneSides = 0;
static int32_t g_zoneSize = 0;
static int32_t g_xScreen = 0;
static int32_t g_yScreen = 0;
static int32_t g_wScreen = 0;
static int32_t g_hScreen = 0;

struct DeadRuntimeState
{
  WPARAM m_virtKey = 0;
  WPARAM m_releaseVirtKey = 0;
  LPARAM m_lParam = 0;
  WCHAR m_char = 0;
  UINT m_ownerButton = 0;
  BYTE m_keyState[256] = {0};
};

static DeadRuntimeState g_dead;
static BYTE g_keyState[256] = {0};
static DWORD g_hookThread = 0;
static bool g_fakeServerInput = false;
static BOOL g_isPrimary = TRUE;

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
    LOG_ERR("failed to init %s.dll, another program may be using it", g_name);
    LOG_INFO("restarting your computer may solve this error");
    throw ScreenOpenFailureException();
  }
}

int MSWindowsHook::init(DWORD threadID)
{
  // try to open process that last called init() to see if it's
  // still running or if it died without cleaning up.
  if (g_processID != 0 && g_processID != GetCurrentProcessId()) {
    HANDLE process = OpenProcess(STANDARD_RIGHTS_REQUIRED, FALSE, g_processID);
    if (process != nullptr) {
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
    g_getMessage = nullptr;
    g_keyboardLL = nullptr;
    g_mouseLL = nullptr;
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

uint32_t MSWindowsHook::getSides()
{
  return g_zoneSides;
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
  if (g_dead.m_virtKey != 0) {
    auto virtualKey = static_cast<UINT>(g_dead.m_virtKey);
    auto scanCode = static_cast<UINT>((g_dead.m_lParam & 0x10ff0000u) >> 16);
    if (ToUnicode(virtualKey, scanCode, g_dead.m_keyState, wc, size, flags) >= 2) {
      // If ToUnicode returned >=2, it means that we accidentally removed
      // a double dead key instead of restoring it. Thus, we call
      // ToUnicode again with the same parameters to restore the
      // internal dead key state.
      ToUnicode(virtualKey, scanCode, g_dead.m_keyState, wc, size, flags);

      // We need to keep track of this because g_dead.m_virtKey will be
      // cleared later on; this would cause the dead key release to
      // incorrectly restore the dead key state.
      g_dead.m_releaseVirtKey = g_dead.m_virtKey;
    }
  }
}

static void clearToUnicodeDeadKeyState(UINT flags)
{
  BYTE emptyState[256] = {};
  WCHAR unicode[2] = {0, 0};
  const UINT scanCode = MapVirtualKey(VK_SPACE, MAPVK_VK_TO_VSC);

  // ToUnicode maintains an internal dead-key compose state.
  // Flush it defensively when a dead key has just been consumed.
  while (ToUnicode(VK_SPACE, scanCode, emptyState, unicode, 2, flags) < 0) {
  }
}

static void clearTrackedDeadKeyState(deskflow::win32::DeadKeyState &deadState, UINT flags)
{
  if (g_mode == kHOOK_RELAY_EVENTS) {
    clearToUnicodeDeadKeyState(flags);
  }

  deskflow::win32::clearDeadKeyState(deadState);
  g_dead.m_virtKey = deadState.m_deadVirtKey;
  g_dead.m_lParam = deadState.m_deadLParam;
  g_dead.m_releaseVirtKey = 0;
  g_dead.m_char = 0;
  g_dead.m_ownerButton = 0;
}

static deskflow::win32::DeadKeyState makeTrackedDeadKeyStateSnapshot()
{
  deskflow::win32::DeadKeyState deadState;
  deadState.m_deadVirtKey = g_dead.m_virtKey;
  deadState.m_deadLParam = g_dead.m_lParam;
  deadState.m_deadChar = g_dead.m_char;
  for (size_t i = 0; i < 256; ++i) {
    deadState.m_deadKeyState[i] = g_dead.m_keyState[i];
  }
  return deadState;
}

static void storeCapturedDeadKeyState(
    deskflow::win32::DeadKeyState &deadState, WPARAM wParam, LPARAM lParam, const BYTE keys[256], WCHAR deadChar
)
{
  deskflow::win32::captureDeadKeyState(deadState, wParam, lParam, keys, deadChar);
  g_dead.m_virtKey = deadState.m_deadVirtKey;
  g_dead.m_lParam = deadState.m_deadLParam;
  g_dead.m_char = deadState.m_deadChar;
  g_dead.m_ownerButton = 0;
  for (size_t i = 0; i < 256; ++i) {
    g_dead.m_keyState[i] = deadState.m_deadKeyState[i];
  }
}

static void normalizeControlAltForTranslation(BYTE keys[256], UINT &control, UINT &menu)
{
  control = keys[VK_CONTROL] | keys[VK_LCONTROL] | keys[VK_RCONTROL];
  menu = keys[VK_MENU] | keys[VK_LMENU] | keys[VK_RMENU];
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
}

static UINT getTranslationFlags(UINT menu)
{
  UINT flags = 0;
  if ((menu & 0x80) != 0) {
    flags |= 1;
  }
  return flags;
}

struct TranslationResult
{
  int m_count = 0;
  WCHAR m_chars[2] = {0, 0};
  bool m_noAltGr = false;
};

static TranslationResult translateToUnicodeWithAltGrFallback(
    WPARAM wParam, LPARAM lParam, const BYTE keys[256], UINT flags, UINT control, UINT menu, bool restorePendingDead
)
{
  TranslationResult result;
  BYTE activeKeys[256];
  for (size_t i = 0; i < 256; ++i) {
    activeKeys[i] = keys[i];
  }

  if (restorePendingDead) {
    setDeadKey(result.m_chars, 2, flags);
  }

  const UINT scanCode = static_cast<UINT>((lParam & 0x10ff0000u) >> 16);
  result.m_count = ToUnicode((UINT)wParam, scanCode, activeKeys, result.m_chars, 2, flags);

  if (result.m_count == 0 && (control & 0x80) != 0 && (menu & 0x80) != 0) {
    result.m_noAltGr = true;
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, wParam | 0x05000000, lParam);

    if (restorePendingDead) {
      setDeadKey(result.m_chars, 2, flags);
    }

    activeKeys[VK_LCONTROL] = 0;
    activeKeys[VK_RCONTROL] = 0;
    activeKeys[VK_CONTROL] = 0;
    activeKeys[VK_LMENU] = 0;
    activeKeys[VK_RMENU] = 0;
    activeKeys[VK_MENU] = 0;
    result.m_count = ToUnicode((UINT)wParam, scanCode, activeKeys, result.m_chars, 2, flags);
  }

  return result;
}

static bool keyboardHookHandlerLegacy(WPARAM wParam, LPARAM lParam)
{
  deskflow::win32::DeadKeyState deadState = makeTrackedDeadKeyStateSnapshot();

  DWORD vkCode = static_cast<DWORD>(wParam);
  bool kf_up = (lParam & (KF_UP << 16)) != 0;

  if (wParam == DESKFLOW_HOOK_FAKE_INPUT_VIRTUAL_KEY && ((lParam >> 16) & 0xffu) == DESKFLOW_HOOK_FAKE_INPUT_SCANCODE) {
    g_fakeServerInput = ((lParam & 0x80000000u) == 0);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, 0xff000000u | wParam, lParam);
    return true;
  }

  if (g_fakeServerInput) {
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, 0xfe000000u | wParam, lParam);
    return false;
  }

  if (wParam == VK_RSHIFT) {
    lParam &= ~0x01000000u;
  }

  PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, wParam, lParam);

  if ((g_dead.m_virtKey == wParam || g_dead.m_releaseVirtKey == wParam) && (lParam & 0x80000000u) != 0) {
    g_dead.m_releaseVirtKey = 0;
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, wParam | 0x04000000, lParam);
    return false;
  }

  BYTE keys[256];
  keyboardGetState(keys, vkCode, kf_up);

  UINT control = 0;
  UINT menu = 0;
  normalizeControlAltForTranslation(keys, control, menu);
  UINT flags = getTranslationFlags(menu);

  if (g_mode != kHOOK_RELAY_EVENTS) {
    UINT sc = (lParam & 0x01ff0000u) >> 16;
    if (menu && (sc >= 0x47u && sc <= 0x52u && sc != 0x4au && sc != 0x4eu)) {
      return false;
    }
  }

  TranslationResult translation =
      translateToUnicodeWithAltGrFallback(wParam, lParam, keys, flags, control, menu, true);
  WCHAR *wc = translation.m_chars;
  int n = translation.m_count;
  bool noAltGr = translation.m_noAltGr;

  PostThreadMessage(
      g_threadID, DESKFLOW_MSG_DEBUG, (wc[0] & 0xffff) | ((wParam & 0xff) << 16) | ((n & 0xf) << 24) | 0x60000000,
      lParam
  );
  WPARAM charAndVirtKey = 0;
  bool clearDeadKey = false;
  bool isKeyUpEvent = ((lParam & 0x80000000u) != 0);
  switch (n) {
  default:
    if (lParam & 0x80000000u) {
      break;
    }
    storeCapturedDeadKeyState(deadState, wParam, lParam, keys, wc[0]);
    break;

  case 0:
    if (!isKeyUpEvent && g_dead.m_virtKey != 0 && g_dead.m_virtKey != wParam) {
      WPARAM deadCharAndVirtKey = makeKeyMsg((UINT)g_dead.m_virtKey, wc[0], noAltGr);
      PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_dead.m_lParam & 0x7fffffffu);
      PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_dead.m_lParam | 0x80000000u);
      clearDeadKey = true;
    }
    charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    break;

  case 1:
    if (isKeyUpEvent) {
      charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    } else {
      charAndVirtKey = makeKeyMsg((UINT)wParam, wc[0], noAltGr);
      clearDeadKey = deskflow::win32::shouldClearDeadKeyState(1, isKeyUpEvent);
    }
    break;

  case 2: {
    if (isKeyUpEvent) {
      charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    } else {
      WPARAM deadCharAndVirtKey = makeKeyMsg((UINT)g_dead.m_virtKey, wc[0], noAltGr);
      PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_dead.m_lParam & 0x7fffffffu);
      PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_dead.m_lParam | 0x80000000u);
      charAndVirtKey = makeKeyMsg((UINT)wParam, wc[1], noAltGr);
      clearDeadKey = deskflow::win32::shouldClearDeadKeyState(2, isKeyUpEvent);
    }
    break;
  }
  }

  const bool keepDeadKeyForLocalApp = (g_mode != kHOOK_RELAY_EVENTS);
  if (g_dead.m_virtKey != 0 && (!clearDeadKey || keepDeadKeyForLocalApp)) {
    ToUnicode((UINT)g_dead.m_virtKey, (g_dead.m_lParam & 0x10ff0000u) >> 16, g_dead.m_keyState, wc, 2, flags);
  }

  if (clearDeadKey) {
    clearTrackedDeadKeyState(deadState, flags);
  }

  if (charAndVirtKey != 0) {
    PostThreadMessage(g_threadID, DESKFLOW_MSG_DEBUG, charAndVirtKey | 0x07000000, lParam);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, charAndVirtKey, lParam);
  }

  return false;
}

static bool keyboardHookHandler(WPARAM wParam, LPARAM lParam)
{
  if (g_mode != kHOOK_RELAY_EVENTS) {
    return keyboardHookHandlerLegacy(wParam, lParam);
  }

  deskflow::win32::DeadKeyState deadState;
  deadState.m_deadVirtKey = g_dead.m_virtKey;
  deadState.m_deadLParam = g_dead.m_lParam;
  deadState.m_deadChar = g_dead.m_char;
  for (size_t i = 0; i < 256; ++i) {
    deadState.m_deadKeyState[i] = g_dead.m_keyState[i];
  }

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
  if ((g_dead.m_virtKey == wParam || g_dead.m_releaseVirtKey == wParam) && (lParam & 0x80000000u) != 0) {
    g_dead.m_releaseVirtKey = 0;
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
  UINT control = 0;
  UINT menu = 0;
  normalizeControlAltForTranslation(keys, control, menu);

  // ToAscii() needs to know if a menu is active for some reason.
  // we don't know and there doesn't appear to be any way to find
  // out.  so we'll just assume a menu is active if the menu key
  // is down.
  // FIXME -- figure out some way to check if a menu is active
  UINT flags = getTranslationFlags(menu);

  const bool isKeyUpEvent = ((lParam & 0x80000000u) != 0);
  const UINT button = static_cast<UINT>((lParam & 0x01ff0000u) >> 16);
  const bool hasPendingDead = (g_dead.m_virtKey != 0);
  const bool isModifierOrToggle = deskflow::win32::isModifierOrToggleVK(static_cast<UINT>(wParam));

  WCHAR wc[] = {0, 0};
  bool noAltGr = false;
  int n = 0;
  bool translatedWithPendingDead = false;
  if (!isKeyUpEvent) {
    translatedWithPendingDead = hasPendingDead;
    TranslationResult translation =
        translateToUnicodeWithAltGrFallback(wParam, lParam, keys, flags, control, menu, hasPendingDead);
    wc[0] = translation.m_chars[0];
    wc[1] = translation.m_chars[1];
    noAltGr = translation.m_noAltGr;
    n = translation.m_count;
  }

  PostThreadMessage(
      g_threadID, DESKFLOW_MSG_DEBUG, (wc[0] & 0xffff) | ((wParam & 0xff) << 16) | ((n & 0xf) << 24) | 0x60000000,
      lParam
  );

  deskflow::win32::DeadKeyTransitionInput transitionInput;
  transitionInput.m_eventType = isKeyUpEvent ? deskflow::win32::DeadKeyEventType::KeyUp
                                             : deskflow::win32::DeadKeyEventType::KeyDown;
  transitionInput.m_toUnicodeResult = n;
  transitionInput.m_vk = static_cast<UINT>(wParam);
  transitionInput.m_button = button;
  transitionInput.m_isModifierOrToggle = isModifierOrToggle;
  transitionInput.m_hasPendingDead = hasPendingDead;
  transitionInput.m_deadOwnerButton = g_dead.m_ownerButton;
  const auto transition = deskflow::win32::decideDeadKeyTransition(transitionInput);

  if (transition.m_setOwner) {
    g_dead.m_ownerButton = button;
  }
  if (transition.m_clearOwner) {
    g_dead.m_ownerButton = 0;
  }

  auto emitPendingDeadFallback = [&](bool noAltGrFallback, WCHAR deadChar) {
    if (g_dead.m_virtKey == 0) {
      return;
    }

    const WCHAR effectiveDeadChar = (deadChar != 0) ? deadChar : g_dead.m_char;
    WPARAM deadCharAndVirtKey = makeKeyMsg((UINT)g_dead.m_virtKey, effectiveDeadChar, noAltGrFallback);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_dead.m_lParam & 0x7fffffffu);
    PostThreadMessage(g_threadID, DESKFLOW_MSG_KEY, deadCharAndVirtKey, g_dead.m_lParam | 0x80000000u);
  };

  WPARAM charAndVirtKey = 0;
  bool clearDeadAfterRestore = transition.m_clearPending;
  switch (transition.m_action) {
  case deskflow::win32::DeadKeyAction::store_dead:
    if (!isKeyUpEvent) {
      storeCapturedDeadKeyState(deadState, wParam, lParam, keys, wc[0]);
    }
    break;

  case deskflow::win32::DeadKeyAction::keep_pending:
    charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    break;

  case deskflow::win32::DeadKeyAction::consume_with_composed_char:
    charAndVirtKey = makeKeyMsg((UINT)wParam, wc[0], noAltGr);
    break;

  case deskflow::win32::DeadKeyAction::consume_with_dead_fallback:
    emitPendingDeadFallback(noAltGr, wc[0]);
    if (!isKeyUpEvent && n == 2) {
      charAndVirtKey = makeKeyMsg((UINT)wParam, wc[1], noAltGr);
    } else {
      charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    }
    break;

  case deskflow::win32::DeadKeyAction::consume_before_new_candidate: {
    emitPendingDeadFallback(noAltGr, g_dead.m_char);
    clearTrackedDeadKeyState(deadState, flags);
    translatedWithPendingDead = false;
    clearDeadAfterRestore = false;

    WCHAR recomputedWc[] = {0, 0};
    TranslationResult recomputedTranslation =
        translateToUnicodeWithAltGrFallback(wParam, lParam, keys, flags, control, menu, false);
    recomputedWc[0] = recomputedTranslation.m_chars[0];
    recomputedWc[1] = recomputedTranslation.m_chars[1];
    const bool recomputedNoAltGr = recomputedTranslation.m_noAltGr;
    const int recomputed = recomputedTranslation.m_count;
    if (recomputed < 0) {
      storeCapturedDeadKeyState(deadState, wParam, lParam, keys, recomputedWc[0]);
    } else if (recomputed == 0) {
      charAndVirtKey = makeKeyMsg((UINT)wParam, 0, recomputedNoAltGr);
    } else {
      charAndVirtKey = makeKeyMsg((UINT)wParam, recomputedWc[0], recomputedNoAltGr);
    }
    break;
  }

  case deskflow::win32::DeadKeyAction::no_dead_interaction:
    if (isKeyUpEvent) {
      charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    } else if (n == 0) {
      charAndVirtKey = makeKeyMsg((UINT)wParam, 0, noAltGr);
    } else if (n > 0) {
      charAndVirtKey = makeKeyMsg((UINT)wParam, wc[0], noAltGr);
    }
    break;
  }

  // put back the dead key for local apps unless we're relaying and the
  // dead key was consumed by this event. In relay mode, restoring after
  // consume can leak compose state to the next key.
  const bool keepDeadKeyForLocalApp = (g_mode != kHOOK_RELAY_EVENTS);
  if (translatedWithPendingDead && g_dead.m_virtKey != 0 && (!clearDeadAfterRestore || keepDeadKeyForLocalApp)) {
    ToUnicode((UINT)g_dead.m_virtKey, (g_dead.m_lParam & 0x10ff0000u) >> 16, g_dead.m_keyState, wc, 2, flags);
  }

  // clear out old dead key state
  if (clearDeadAfterRestore) {
    clearTrackedDeadKeyState(deadState, flags);
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

  case WM_MOUSEHWHEEL:
    if (g_mode == kHOOK_RELAY_EVENTS) {
      // relay event
      PostThreadMessage(g_threadID, DESKFLOW_MSG_MOUSE_WHEEL, 0, data);
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
      using enum DirectionMask;
      if (!inside && (g_zoneSides & static_cast<int>(LeftMask)) != 0) {
        inside = (x < g_xScreen + g_zoneSize);
      }
      if (!inside && (g_zoneSides & static_cast<int>(RightMask)) != 0) {
        inside = (x >= g_xScreen + g_wScreen - g_zoneSize);
      }
      if (!inside && (g_zoneSides & static_cast<int>(TopMask)) != 0) {
        inside = (y < g_yScreen + g_zoneSize);
      }
      if (!inside && (g_zoneSides & static_cast<int>(BottomMask)) != 0) {
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
  assert(g_getMessage == nullptr || g_screenSaver);

  // must be initialized
  if (g_threadID == 0) {
    return kHOOK_FAILED;
  }

  // discard old dead keys
  g_dead.m_virtKey = 0;
  g_dead.m_lParam = 0;
  g_dead.m_releaseVirtKey = 0;
  g_dead.m_char = 0;
  g_dead.m_ownerButton = 0;

  // reset fake input flag
  g_fakeServerInput = false;

  // install low-level hooks.  we require that they both get installed.
  g_mouseLL = SetWindowsHookEx(WH_MOUSE_LL, &mouseLLHook, nullptr, 0);
#if !NO_GRAB_KEYBOARD
  g_keyboardLL = SetWindowsHookEx(WH_KEYBOARD_LL, &keyboardLLHook, nullptr, 0);
  if (g_mouseLL == nullptr || g_keyboardLL == nullptr) {
    if (g_keyboardLL != nullptr) {
      UnhookWindowsHookEx(g_keyboardLL);
      g_keyboardLL = nullptr;
    }
    if (g_mouseLL != nullptr) {
      UnhookWindowsHookEx(g_mouseLL);
      g_mouseLL = nullptr;
    }
  }
#endif
  // clang-format off
  // check that we got all the hooks we wanted
  if (
      (g_mouseLL == nullptr) ||
#if !NO_GRAB_KEYBOARD
      (g_keyboardLL == nullptr)
#endif
  ) {
    uninstall();
    return kHOOK_FAILED;
  }
  // clang-format on
  if (g_keyboardLL != nullptr || g_mouseLL != nullptr) {
    g_hookThread = GetCurrentThreadId();
    return kHOOK_OKAY_LL;
  }

  return kHOOK_OKAY;
}

int MSWindowsHook::uninstall()
{
  // discard old dead keys
  g_dead.m_virtKey = 0;
  g_dead.m_lParam = 0;
  g_dead.m_releaseVirtKey = 0;
  g_dead.m_char = 0;
  g_dead.m_ownerButton = 0;

  // uninstall hooks
  if (g_keyboardLL != nullptr) {
    UnhookWindowsHookEx(g_keyboardLL);
    g_keyboardLL = nullptr;
  }
  if (g_mouseLL != nullptr) {
    UnhookWindowsHookEx(g_mouseLL);
    g_mouseLL = nullptr;
  }
  if (g_getMessage != nullptr && !g_screenSaver) {
    UnhookWindowsHookEx(g_getMessage);
    g_getMessage = nullptr;
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
  if (g_getMessage == nullptr) {
    g_getMessage = SetWindowsHookEx(WH_GETMESSAGE, &getMessageHook, nullptr, 0);
  }

  return (g_getMessage != nullptr) ? 1 : 0;
}

int MSWindowsHook::uninstallScreenSaver()
{
  // uninstall hook unless the mouse wheel hook is installed
  if (g_getMessage != nullptr) {
    UnhookWindowsHookEx(g_getMessage);
    g_getMessage = nullptr;
  }

  // screen saver hook is no longer installed
  g_screenSaver = false;

  return 1;
}


