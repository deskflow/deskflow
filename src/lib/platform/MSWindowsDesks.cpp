/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsDesks.h"

#include "base/IEventQueue.h"
#include "base/IJob.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "deskflow/IScreenSaver.h"
#include "deskflow/ScreenException.h"
#include "deskflow/win32/AppUtilWindows.h"
#include "mt/Lock.h"
#include "mt/Thread.h"
#include "platform/MSWindowsHook.h"
#include "platform/MSWindowsScreen.h"

#include <malloc.h>

// these are only defined when WINVER >= 0x0500
#if !defined(SPI_GETMOUSESPEED)
#define SPI_GETMOUSESPEED 112
#endif
#if !defined(SPI_SETMOUSESPEED)
#define SPI_SETMOUSESPEED 113
#endif
#if !defined(SPI_GETSCREENSAVERRUNNING)
#define SPI_GETSCREENSAVERRUNNING 114
#endif

// X button stuff
#if !defined(WM_XBUTTONDOWN)
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP 0x020C
#define WM_XBUTTONDBLCLK 0x020D
#define WM_NCXBUTTONDOWN 0x00AB
#define WM_NCXBUTTONUP 0x00AC
#define WM_NCXBUTTONDBLCLK 0x00AD
#define MOUSEEVENTF_XDOWN 0x0080
#define MOUSEEVENTF_XUP 0x0100
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#endif
#if !defined(VK_XBUTTON1)
#define VK_XBUTTON1 0x05
#define VK_XBUTTON2 0x06
#endif

// <unused>; <unused>
#define DESKFLOW_MSG_SWITCH DESKFLOW_HOOK_LAST_MSG + 1
// <unused>; <unused>
#define DESKFLOW_MSG_ENTER DESKFLOW_HOOK_LAST_MSG + 2
// <unused>; <unused>
#define DESKFLOW_MSG_LEAVE DESKFLOW_HOOK_LAST_MSG + 3
// wParam = flags, HIBYTE(lParam) = virtual key, LOBYTE(lParam) = scan code
#define DESKFLOW_MSG_FAKE_KEY DESKFLOW_HOOK_LAST_MSG + 4
// flags, XBUTTON id
#define DESKFLOW_MSG_FAKE_BUTTON DESKFLOW_HOOK_LAST_MSG + 5
// x; y
#define DESKFLOW_MSG_FAKE_MOVE DESKFLOW_HOOK_LAST_MSG + 6
// xDelta; yDelta
#define DESKFLOW_MSG_FAKE_WHEEL DESKFLOW_HOOK_LAST_MSG + 7
// POINT*; <unused>
#define DESKFLOW_MSG_CURSOR_POS DESKFLOW_HOOK_LAST_MSG + 8
// IKeyState*; <unused>
#define DESKFLOW_MSG_SYNC_KEYS DESKFLOW_HOOK_LAST_MSG + 9
// install; <unused>
#define DESKFLOW_MSG_SCREENSAVER DESKFLOW_HOOK_LAST_MSG + 10
// dx; dy
#define DESKFLOW_MSG_FAKE_REL_MOVE DESKFLOW_HOOK_LAST_MSG + 11
// enable; <unused>
#define DESKFLOW_MSG_FAKE_INPUT DESKFLOW_HOOK_LAST_MSG + 12

static void send_keyboard_input(WORD wVk, WORD wScan, DWORD dwFlags)
{
  INPUT inp;
  inp.type = INPUT_KEYBOARD;
  inp.ki.wVk = (dwFlags & KEYEVENTF_UNICODE) ? 0 : wVk; // 1..254 inclusive otherwise
  inp.ki.wScan = wScan;
  inp.ki.dwFlags = dwFlags & 0xF;
  inp.ki.time = 0;
  inp.ki.dwExtraInfo = 0;
  SendInput(1, &inp, sizeof(inp));
}

static void send_mouse_input(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData)
{
  INPUT inp;
  inp.type = INPUT_MOUSE;
  inp.mi.dwFlags = dwFlags;
  inp.mi.dx = dx;
  inp.mi.dy = dy;
  inp.mi.mouseData = dwData;
  inp.mi.time = 0;
  inp.mi.dwExtraInfo = 0;
  SendInput(1, &inp, sizeof(inp));
}

//
// MSWindowsDesks
//

MSWindowsDesks::MSWindowsDesks(
    bool isPrimary, bool noHooks, const IScreenSaver *screensaver, IEventQueue *events, IJob *updateKeys
)
    : m_isPrimary(isPrimary),
      m_noHooks(noHooks),
      m_isOnScreen(m_isPrimary),
      m_screensaver(screensaver),
      m_deskReady(&m_mutex, false),
      m_updateKeys(updateKeys),
      m_events(events)
{

  m_cursor = createBlankCursor();
  m_deskClass = createDeskWindowClass(m_isPrimary);
  m_keyLayout = AppUtilWindows::instance().getCurrentKeyboardLayout();
  resetOptions();
}

MSWindowsDesks::~MSWindowsDesks()
{
  disable();
  destroyClass(m_deskClass);
  destroyCursor(m_cursor);
  delete m_updateKeys;
}

void MSWindowsDesks::enable()
{
  m_threadID = GetCurrentThreadId();

  // set the active desk and (re)install the hooks
  checkDesk();

  // install the desk timer.  this timer periodically checks
  // which desk is active and reinstalls the hooks as necessary.
  // we wouldn't need this if windows notified us of a desktop
  // change but as far as i can tell it doesn't.
  m_timer = m_events->newTimer(0.2, nullptr);
  m_events->addHandler(EventTypes::Timer, m_timer, [this](const auto &) { handleCheckDesk(); });

  updateKeys();
}

void MSWindowsDesks::disable()
{
  // remove timer
  if (m_timer != nullptr) {
    m_events->removeHandler(EventTypes::Timer, m_timer);
    m_events->deleteTimer(m_timer);
    m_timer = nullptr;
  }

  // destroy desks
  removeDesks();

  m_isOnScreen = m_isPrimary;
}

void MSWindowsDesks::enter()
{
  sendMessage(DESKFLOW_MSG_ENTER, 0, 0);
}

void MSWindowsDesks::leave(HKL keyLayout)
{
  sendMessage(DESKFLOW_MSG_LEAVE, (WPARAM)keyLayout, 0);
}

void MSWindowsDesks::resetOptions()
{
  m_leaveForegroundOption = false;
}

void MSWindowsDesks::setOptions(const OptionsList &options)
{
  for (uint32_t i = 0, n = (uint32_t)options.size(); i < n; i += 2) {
    if (options[i] == kOptionWin32KeepForeground) {
      m_leaveForegroundOption = (options[i + 1] != 0);
      LOG_DEBUG1("%s the foreground window", m_leaveForegroundOption ? "don\'t grab" : "grab");
    }
  }
}

void MSWindowsDesks::updateKeys()
{
  sendMessage(DESKFLOW_MSG_SYNC_KEYS, 0, 0);
}

void MSWindowsDesks::setShape(
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t xCenter, int32_t yCenter, bool isMultimon
)
{
  m_x = x;
  m_y = y;
  m_w = width;
  m_h = height;
  m_xCenter = xCenter;
  m_yCenter = yCenter;
  m_multimon = isMultimon;
}

void MSWindowsDesks::installScreensaverHooks(bool install)
{
  if (m_isPrimary && m_screensaverNotify != install) {
    m_screensaverNotify = install;
    sendMessage(DESKFLOW_MSG_SCREENSAVER, install, 0);
  }
}

void MSWindowsDesks::fakeInputBegin()
{
  sendMessage(DESKFLOW_MSG_FAKE_INPUT, 1, 0);
}

void MSWindowsDesks::fakeInputEnd()
{
  sendMessage(DESKFLOW_MSG_FAKE_INPUT, 0, 0);
}

void MSWindowsDesks::getCursorPos(int32_t &x, int32_t &y) const
{
  POINT pos{0, 0};
  sendMessage(DESKFLOW_MSG_CURSOR_POS, reinterpret_cast<WPARAM>(&pos), 0);
  x = pos.x;
  y = pos.y;
}

void MSWindowsDesks::fakeKeyEvent(WORD virtualKey, WORD scanCode, DWORD flags, bool /*isAutoRepeat*/) const
{
  sendMessage(DESKFLOW_MSG_FAKE_KEY, flags, MAKELPARAM(scanCode, virtualKey));
}

void MSWindowsDesks::fakeMouseButton(ButtonID button, bool press)
{
  // the system will swap the meaning of left/right for us if
  // the user has configured a left-handed mouse but we don't
  // want it to swap since we want the handedness of the
  // server's mouse.  so pre-swap for a left-handed mouse.
  if (GetSystemMetrics(SM_SWAPBUTTON)) {
    switch (button) {
    case kButtonLeft:
      button = kButtonRight;
      break;

    case kButtonRight:
      button = kButtonLeft;
      break;
    }
  }

  // map button id to button flag and button data
  DWORD data = 0;
  DWORD flags;
  switch (button) {
  case kButtonLeft:
    flags = press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    break;

  case kButtonMiddle:
    flags = press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    break;

  case kButtonRight:
    flags = press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    break;

  case kButtonExtra0 + 0:
    data = XBUTTON1;
    flags = press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    break;

  case kButtonExtra0 + 1:
    data = XBUTTON2;
    flags = press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    break;

  default:
    return;
  }

  // do it
  sendMessage(DESKFLOW_MSG_FAKE_BUTTON, flags, data);
}

void MSWindowsDesks::fakeMouseMove(int32_t x, int32_t y) const
{
  sendMessage(DESKFLOW_MSG_FAKE_MOVE, static_cast<WPARAM>(x), static_cast<LPARAM>(y));
}

void MSWindowsDesks::fakeMouseRelativeMove(int32_t dx, int32_t dy) const
{
  sendMessage(DESKFLOW_MSG_FAKE_REL_MOVE, static_cast<WPARAM>(dx), static_cast<LPARAM>(dy));
}

void MSWindowsDesks::fakeMouseWheel(int32_t xDelta, int32_t yDelta) const
{
  sendMessage(DESKFLOW_MSG_FAKE_WHEEL, xDelta, yDelta);
}

void MSWindowsDesks::sendMessage(UINT msg, WPARAM wParam, LPARAM lParam) const
{
  if (m_activeDesk != nullptr && m_activeDesk->m_window != nullptr) {
    PostThreadMessage(m_activeDesk->m_threadID, msg, wParam, lParam);
    waitForDesk();
  }
}

HCURSOR
MSWindowsDesks::createBlankCursor() const
{
  // create a transparent cursor
  int cw = GetSystemMetrics(SM_CXCURSOR);
  int ch = GetSystemMetrics(SM_CYCURSOR);
  uint8_t *cursorAND = new uint8_t[ch * ((cw + 31) >> 2)];
  uint8_t *cursorXOR = new uint8_t[ch * ((cw + 31) >> 2)];
  memset(cursorAND, 0xff, ch * ((cw + 31) >> 2));
  memset(cursorXOR, 0x00, ch * ((cw + 31) >> 2));
  HCURSOR c = CreateCursor(MSWindowsScreen::getWindowInstance(), 0, 0, cw, ch, cursorAND, cursorXOR);
  delete[] cursorXOR;
  delete[] cursorAND;
  return c;
}

void MSWindowsDesks::destroyCursor(HCURSOR cursor) const
{
  if (cursor != nullptr) {
    DestroyCursor(cursor);
  }
}

ATOM MSWindowsDesks::createDeskWindowClass(bool isPrimary) const
{
  WNDCLASSEX classInfo;
  classInfo.cbSize = sizeof(classInfo);
  classInfo.style = CS_DBLCLKS | CS_NOCLOSE;
  classInfo.lpfnWndProc = isPrimary ? &MSWindowsDesks::primaryDeskProc : &MSWindowsDesks::secondaryDeskProc;
  classInfo.cbClsExtra = 0;
  classInfo.cbWndExtra = 0;
  classInfo.hInstance = MSWindowsScreen::getWindowInstance();
  classInfo.hIcon = nullptr;
  classInfo.hCursor = m_cursor;
  classInfo.hbrBackground = nullptr;
  classInfo.lpszMenuName = nullptr;
  classInfo.lpszClassName = L"DeskflowDesk";
  classInfo.hIconSm = nullptr;
  return RegisterClassEx(&classInfo);
}

void MSWindowsDesks::destroyClass(ATOM windowClass) const
{
  if (windowClass != 0) {
    UnregisterClass(MAKEINTATOM(windowClass), MSWindowsScreen::getWindowInstance());
  }
}

HWND MSWindowsDesks::createWindow(ATOM windowClass, const wchar_t *name) const
{
  HWND window = CreateWindowEx(
      WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, MAKEINTATOM(windowClass), name, WS_POPUP, 0, 0, 1, 1, nullptr, nullptr,
      MSWindowsScreen::getWindowInstance(), nullptr
  );
  if (window == nullptr) {
    LOG_ERR("failed to create window: %d", GetLastError());
    throw ScreenOpenFailureException();
  }
  return window;
}

void MSWindowsDesks::destroyWindow(HWND hwnd) const
{
  if (hwnd != nullptr) {
    DestroyWindow(hwnd);
  }
}

LRESULT CALLBACK MSWindowsDesks::primaryDeskProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MSWindowsDesks::secondaryDeskProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // would like to detect any local user input and hide the hider
  // window but for now we just detect mouse motion.
  bool hide = false;
  switch (msg) {
  case WM_MOUSEMOVE:
    if (LOWORD(lParam) != 0 || HIWORD(lParam) != 0) {
      hide = true;
    }
    break;
  }

  if (hide && IsWindowVisible(hwnd)) {
    ReleaseCapture();
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MSWindowsDesks::deskMouseMove(int32_t x, int32_t y) const
{
  // when using absolute positioning with mouse_event(),
  // the normalized device coordinates range over only
  // the primary screen.
  int32_t w = GetSystemMetrics(SM_CXSCREEN);
  int32_t h = GetSystemMetrics(SM_CYSCREEN);
  send_mouse_input(
      MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, (DWORD)((65535.0f * x) / (w - 1) + 0.5f),
      (DWORD)((65535.0f * y) / (h - 1) + 0.5f), 0
  );
}

void MSWindowsDesks::deskMouseRelativeMove(int32_t dx, int32_t dy) const
{
  // relative moves are subject to cursor acceleration which we don't
  // want.so we disable acceleration, do the relative move, then
  // restore acceleration.  there's a slight chance we'll end up in
  // the wrong place if the user moves the cursor using this system's
  // mouse while simultaneously moving the mouse on the server
  // system.  that defeats the purpose of deskflow so we'll assume
  // that won't happen.  even if it does, the next mouse move will
  // correct the position.

  // save mouse speed & acceleration
  int oldSpeed[4];
  bool accelChanged =
      SystemParametersInfo(SPI_GETMOUSE, 0, oldSpeed, 0) && SystemParametersInfo(SPI_GETMOUSESPEED, 0, oldSpeed + 3, 0);

  // use 1:1 motion
  if (accelChanged) {
    int newSpeed[4] = {0, 0, 0, 1};
    accelChanged = SystemParametersInfo(SPI_SETMOUSE, 0, newSpeed, 0) ||
                   SystemParametersInfo(SPI_SETMOUSESPEED, 0, newSpeed + 3, 0);
  }

  // move relative to mouse position
  send_mouse_input(MOUSEEVENTF_MOVE, dx, dy, 0);

  // restore mouse speed & acceleration
  if (accelChanged) {
    SystemParametersInfo(SPI_SETMOUSE, 0, oldSpeed, 0);
    SystemParametersInfo(SPI_SETMOUSESPEED, 0, oldSpeed + 3, 0);
  }
}

/*!
 * Wraps the `ShowCursor` function and calls it repeatedly until the cursor visibility is at
 * the desired state. Windows maintains an internal counter for cursor visibility, and only
 * shows or hides the cursor when it reaches a certain threshold.
 */
void setCursorVisibility(bool visible)
{
  LOG_DEBUG("%s cursor", visible ? "showing" : "hiding");

  const int max = 10;
  int attempts = 0;
  while (attempts++ < max) {
    const auto displayCounter = ShowCursor(visible ? TRUE : FALSE);
    LOG_DEBUG1("cursor display counter: %d", displayCounter);

    if (visible) {
      if (displayCounter < 0) {
        LOG_DEBUG1("cursor still hidden, retrying, attempt: %d", attempts);
      } else {
        LOG_DEBUG1("cursor is now visible, attempts: %d", attempts);
        return;
      }
    } else {
      if (displayCounter >= 0) {
        LOG_DEBUG1("cursor still visible, retrying, attempt: %d", attempts);
      } else {
        LOG_DEBUG1("cursor is now hidden, attempts: %d", attempts);
        return;
      }
    }
  }

  LOG_ERR("unable to set cursor visibility after %d attempts", attempts);
}

void MSWindowsDesks::deskEnter(Desk *desk)
{
  if (!m_isPrimary) {
    ReleaseCapture();
  }

  setCursorVisibility(true);

  SetWindowPos(desk->m_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW);

  // restore the foreground window
  // XXX -- this raises the window to the top of the Z-order.  we
  // want it to stay wherever it was to properly support X-mouse
  // (mouse over activation) but i've no idea how to do that.
  // the obvious workaround of using SetWindowPos() to move it back
  // after being raised doesn't work.
  DWORD thisThread = GetWindowThreadProcessId(desk->m_window, nullptr);
  DWORD thatThread = GetWindowThreadProcessId(desk->m_foregroundWindow, nullptr);
  AttachThreadInput(thatThread, thisThread, TRUE);
  SetForegroundWindow(desk->m_foregroundWindow);
  AttachThreadInput(thatThread, thisThread, FALSE);
  EnableWindow(desk->m_window, desk->m_lowLevel ? FALSE : TRUE);
  desk->m_foregroundWindow = nullptr;
}

void MSWindowsDesks::deskLeave(Desk *desk, HKL keyLayout)
{
  setCursorVisibility(false);

  if (m_isPrimary) {
    // map a window to hide the cursor and to use whatever keyboard
    // layout we choose rather than the keyboard layout of the last
    // active window.
    int x, y, w, h;
    if (desk->m_lowLevel) {
      // with a low level hook the cursor will never budge so
      // just a 1x1 window is sufficient.
      x = m_xCenter;
      y = m_yCenter;
      w = 1;
      h = 1;
    } else {
      // with regular hooks the cursor will jitter as it's moved
      // by the user then back to the center by us.  to be sure
      // we never lose it, cover all the monitors with the window.
      x = m_x;
      y = m_y;
      w = m_w;
      h = m_h;
    }
    SetWindowPos(desk->m_window, HWND_TOP, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    // switch to requested keyboard layout
    ActivateKeyboardLayout(keyLayout, 0);

    // if not using low-level hooks we have to also activate the
    // window to ensure we don't lose keyboard focus.
    // FIXME -- see if this can be avoided.  if so then always
    // disable the window (see handling of DESKFLOW_MSG_SWITCH).
    if (!desk->m_lowLevel) {
      SetActiveWindow(desk->m_window);
    }

    // if using low-level hooks then disable the foreground window
    // so it can't mess up any of our keyboard events.  the console
    // program, for example, will cause characters to be reported as
    // unshifted, regardless of the shift key state.  interestingly
    // we do see the shift key go down and up.
    //
    // note that we must enable the window to activate it and we
    // need to disable the window on deskEnter.
    else {
      desk->m_foregroundWindow = getForegroundWindow();
      if (desk->m_foregroundWindow != nullptr) {
        EnableWindow(desk->m_window, TRUE);
        SetActiveWindow(desk->m_window);
        DWORD thisThread = GetWindowThreadProcessId(desk->m_window, nullptr);
        DWORD thatThread = GetWindowThreadProcessId(desk->m_foregroundWindow, nullptr);

        AttachThreadInput(thatThread, thisThread, TRUE);
        SetForegroundWindow(desk->m_window);
        AttachThreadInput(thatThread, thisThread, FALSE);
      }
    }
  } else {
    // move hider window under the cursor center, raise, and show it
    SetWindowPos(desk->m_window, HWND_TOP, m_xCenter, m_yCenter, 1, 1, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    // watch for mouse motion.  if we see any then we hide the
    // hider window so the user can use the physically attached
    // mouse if desired.  we'd rather not capture the mouse but
    // we aren't notified when the mouse leaves our window.
    SetCapture(desk->m_window);

    // windows can take a while to hide the cursor, so wait a few milliseconds to ensure the cursor
    // is hidden before centering. this doesn't seem to affect the fluidity of the transition.
    // without this, the cursor appears to flicker in the center of the screen which is annoying.
    // a slightly more elegant but complex solution could be to use a timed event.
    // 30 ms seems to work well enough without making the transition feel janky; a lower number
    // would be better but 10 ms doesn't seem to be quite long enough, as we get noticeable flicker.
    // this is largely a balance and out of our control, since windows can be unpredictable...
    // maybe another approach would be to repeatedly check the cursor visibility until it is hidden.
    LOG_DEBUG1("centering cursor on leave: %+d,%+d", m_xCenter, m_yCenter);
    ARCH->sleep(0.03);
    deskMouseMove(m_xCenter, m_yCenter);
  }
}

void MSWindowsDesks::deskThread(void *vdesk)
{
  MSG msg;

  // use given desktop for this thread
  Desk *desk = static_cast<Desk *>(vdesk);
  desk->m_threadID = GetCurrentThreadId();
  desk->m_window = nullptr;
  desk->m_foregroundWindow = nullptr;
  if (desk->m_desk != nullptr && SetThreadDesktop(desk->m_desk) != 0) {
    // create a message queue
    PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

    // create a window.  we use this window to hide the cursor.
    try {
      desk->m_window = createWindow(m_deskClass, L"DeskflowDesk");
      LOG_DEBUG("desk %s window is 0x%08x", desk->m_name.c_str(), desk->m_window);
    } catch (...) {
      // ignore
      LOG_DEBUG("can't create desk window for %s", desk->m_name.c_str());
    }
  }

  // tell main thread that we're ready
  {
    Lock lock(&m_mutex);
    m_deskReady = true;
    m_deskReady.broadcast();
  }

  while (GetMessage(&msg, nullptr, 0, 0)) {
    switch (msg.message) {
    default:
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      continue;

    case DESKFLOW_MSG_SWITCH:
      if (!m_noHooks) {
        MSWindowsHook::uninstall();
        if (m_screensaverNotify) {
          MSWindowsHook::uninstallScreenSaver();
          MSWindowsHook::installScreenSaver();
        }
        switch (MSWindowsHook::install()) {
        case kHOOK_FAILED:
          // we won't work on this desk
          desk->m_lowLevel = false;
          break;

        case kHOOK_OKAY:
          desk->m_lowLevel = false;
          break;

        case kHOOK_OKAY_LL:
          desk->m_lowLevel = true;
          break;
        }

        // a window on the primary screen with low-level hooks
        // should never activate.
        if (desk->m_window)
          EnableWindow(desk->m_window, desk->m_lowLevel ? FALSE : TRUE);
      }
      break;

    case DESKFLOW_MSG_ENTER:
      m_isOnScreen = true;
      deskEnter(desk);
      break;

    case DESKFLOW_MSG_LEAVE:
      m_isOnScreen = false;
      m_keyLayout = (HKL)msg.wParam;
      deskLeave(desk, m_keyLayout);
      break;

    case DESKFLOW_MSG_FAKE_KEY:
      // Note, this is intended to be HI/LOWORD and not HI/LOBYTE
      send_keyboard_input(HIWORD(msg.lParam), LOWORD(msg.lParam), (DWORD)msg.wParam);
      break;

    case DESKFLOW_MSG_FAKE_BUTTON:
      if (msg.wParam != 0) {
        send_mouse_input((DWORD)msg.wParam, 0, 0, (DWORD)msg.lParam);
      }
      break;

    case DESKFLOW_MSG_FAKE_MOVE:
      deskMouseMove(static_cast<int32_t>(msg.wParam), static_cast<int32_t>(msg.lParam));
      break;

    case DESKFLOW_MSG_FAKE_REL_MOVE:
      deskMouseRelativeMove(static_cast<int32_t>(msg.wParam), static_cast<int32_t>(msg.lParam));
      break;

    case DESKFLOW_MSG_FAKE_WHEEL:
      // XXX -- add support for x-axis scrolling
      if (msg.lParam != 0) {
        send_mouse_input(MOUSEEVENTF_WHEEL, 0, 0, (DWORD)msg.lParam);
      }
      break;

    case DESKFLOW_MSG_CURSOR_POS: {
      POINT *pos = reinterpret_cast<POINT *>(msg.wParam);
      if (!GetCursorPos(pos)) {
        pos->x = m_xCenter;
        pos->y = m_yCenter;
      }
      break;
    }

    case DESKFLOW_MSG_SYNC_KEYS:
      m_updateKeys->run();
      break;

    case DESKFLOW_MSG_SCREENSAVER:
      if (!m_noHooks) {
        if (msg.wParam != 0) {
          MSWindowsHook::installScreenSaver();
        } else {
          MSWindowsHook::uninstallScreenSaver();
        }
      }
      break;

    case DESKFLOW_MSG_FAKE_INPUT:
      send_keyboard_input(
          DESKFLOW_HOOK_FAKE_INPUT_VIRTUAL_KEY, DESKFLOW_HOOK_FAKE_INPUT_SCANCODE, msg.wParam ? 0 : KEYEVENTF_KEYUP
      );
      break;
    }

    // notify that message was processed
    Lock lock(&m_mutex);
    m_deskReady = true;
    m_deskReady.broadcast();
  }

  // clean up
  deskEnter(desk);
  if (desk->m_window != nullptr) {
    DestroyWindow(desk->m_window);
  }
  if (desk->m_desk != nullptr) {
    closeDesktop(desk->m_desk);
  }
}

MSWindowsDesks::Desk *MSWindowsDesks::addDesk(const std::wstring &name, HDESK hdesk)
{
  Desk *desk = new Desk;
  desk->m_name = name;
  desk->m_desk = hdesk;
  desk->m_targetID = GetCurrentThreadId();
  desk->m_thread = new Thread(new TMethodJob<MSWindowsDesks>(this, &MSWindowsDesks::deskThread, desk));
  waitForDesk();
  m_desks.insert(std::make_pair(name, desk));
  return desk;
}

void MSWindowsDesks::removeDesks()
{
  for (Desks::iterator index = m_desks.begin(); index != m_desks.end(); ++index) {
    Desk *desk = index->second;
    PostThreadMessage(desk->m_threadID, WM_QUIT, 0, 0);
    desk->m_thread->wait();
    delete desk->m_thread;
    delete desk;
  }
  m_desks.clear();
  m_activeDesk = nullptr;
  m_activeDeskName = L"";
}

void MSWindowsDesks::checkDesk()
{
  // get current desktop.  if we already know about it then return.
  Desk *desk;
  HDESK hdesk = openInputDesktop();
  std::wstring name = getDesktopName(hdesk);
  Desks::const_iterator index = m_desks.find(name);
  if (index == m_desks.end()) {
    desk = addDesk(name, hdesk);
    // hold on to hdesk until thread exits so the desk can't
    // be removed by the system
  } else {
    closeDesktop(hdesk);
    desk = index->second;
  }

  // if active desktop changed then tell the old and new desk threads
  // about the change.  don't switch desktops when the screensaver is
  // active becaue we'd most likely switch to the screensaver desktop
  // which would have the side effect of forcing the screensaver to
  // stop.
  if (name != m_activeDeskName && !m_screensaver->isActive()) {
    // show cursor on previous desk
    bool wasOnScreen = m_isOnScreen;
    if (!wasOnScreen) {
      sendMessage(DESKFLOW_MSG_ENTER, 0, 0);
    }

    // check for desk accessibility change.  we don't get events
    // from an inaccessible desktop so when we switch from an
    // inaccessible desktop to an accessible one we have to
    // update the keyboard state.
    LOG_DEBUG("switched to desk \"%s\"", name.c_str());
    bool syncKeys = false;
    bool isAccessible = isDeskAccessible(desk);
    if (isDeskAccessible(m_activeDesk) != isAccessible) {
      if (isAccessible) {
        LOG_DEBUG("desktop is now accessible");
        syncKeys = true;
      } else {
        LOG_DEBUG("desktop is now inaccessible");
      }
    }

    // switch desk
    m_activeDesk = desk;
    m_activeDeskName = name;
    sendMessage(DESKFLOW_MSG_SWITCH, 0, 0);

    // hide cursor on new desk
    if (!wasOnScreen) {
      sendMessage(DESKFLOW_MSG_LEAVE, (WPARAM)m_keyLayout, 0);
    }

    // update keys if necessary
    if (syncKeys) {
      updateKeys();
    }
  } else if (name != m_activeDeskName) {
    // screen saver might have started
    PostThreadMessage(m_threadID, DESKFLOW_MSG_SCREEN_SAVER, TRUE, 0);
  }
}

bool MSWindowsDesks::isDeskAccessible(const Desk *desk) const
{
  return (desk != nullptr && desk->m_desk != nullptr);
}

void MSWindowsDesks::waitForDesk() const
{
  MSWindowsDesks *self = const_cast<MSWindowsDesks *>(this);

  Lock lock(&m_mutex);
  while (!(bool)m_deskReady) {
    m_deskReady.wait();
  }
  self->m_deskReady = false;
}

void MSWindowsDesks::handleCheckDesk()
{
  checkDesk();

  // also check if screen saver is running if on a modern OS and
  // this is the primary screen.
  if (m_isPrimary) {
    BOOL running;
    SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &running, FALSE);
    PostThreadMessage(m_threadID, DESKFLOW_MSG_SCREEN_SAVER, running, 0);
  }
}

HDESK
MSWindowsDesks::openInputDesktop()
{
  return OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, TRUE, DESKTOP_CREATEWINDOW | DESKTOP_HOOKCONTROL | GENERIC_WRITE);
}

void MSWindowsDesks::closeDesktop(HDESK desk)
{
  if (desk != nullptr) {
    CloseDesktop(desk);
  }
}

std::wstring MSWindowsDesks::getDesktopName(HDESK desk)
{
  if (desk == nullptr) {
    return std::wstring();
  } else {
    DWORD size;
    GetUserObjectInformation(desk, UOI_NAME, nullptr, 0, &size);
    TCHAR *name = (TCHAR *)alloca(size + sizeof(TCHAR));
    GetUserObjectInformation(desk, UOI_NAME, name, size, &size);
    std::wstring result(name);
    return result;
  }
}

HWND MSWindowsDesks::getForegroundWindow() const
{
  // Ideally we'd return nullptr as much as possible, only returning
  // the actual foreground window when we know it's going to mess
  // up our keyboard input.  For now we'll just let the user
  // decide.
  if (m_leaveForegroundOption) {
    return nullptr;
  }
  return GetForegroundWindow();
}
