/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsComputer.h"

#include "arch/Arch.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "client/Client.h"
#include "common/Constants.h"
#include "common/Settings.h"
#include "deskflow/App.h"
#include "deskflow/ClientApp.h"
#include "deskflow/Clipboard.h"
#include "deskflow/DisplayException.h"
#include "deskflow/KeyMap.h"
#include "platform/MSWindowsClipboard.h"
#include "platform/MSWindowsDesks.h"
#include "platform/MSWindowsEventQueueBuffer.h"
#include "platform/MSWindowsKeyState.h"
#include "platform/MSWindowsScreenSaver.h"

#include <Shlobj.h>
#include <algorithm>
#include <comutil.h>
#include <string.h>

// suppress warning about GetVersionEx, which is used indirectly in this
// compilation unit.
#pragma warning(disable : 4996)

//
// add backwards compatible multihead support (and suppress bogus warning).
// this isn't supported on MinGW yet AFAICT.
//
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4706) // assignment within conditional
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#pragma warning(pop)
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

// WM_POWERBROADCAST stuff
#if !defined(PBT_APMRESUMEAUTOMATIC)
#define PBT_APMRESUMEAUTOMATIC 0x0012
#endif

//
// MSWindowsComputer
//

HINSTANCE MSWindowsComputer::s_windowInstance = nullptr;
MSWindowsComputer *MSWindowsComputer::s_computer = nullptr;

MSWindowsComputer::MSWindowsComputer(bool isPrimary, bool useHooks, IEventQueue *events, bool enableLangSync)
    : PlatformComputer(events),
      m_isPrimary(isPrimary),
      m_useHooks(useHooks),
      m_isOnComputer(m_isPrimary),
      m_hasMouse(GetSystemMetrics(SM_MOUSEPRESENT) != 0),
      m_events(events)
{
  LOG_DEBUG("settting up %s computer", m_isPrimary ? "primary" : "secondary");

  assert(s_windowInstance != nullptr);
  assert(s_computer == nullptr);

  s_computer = this;
  try {
    if (m_isPrimary && m_useHooks) {
      m_hook.loadLibrary();
    }

    m_screensaver = new MSWindowsScreenSaver();
    m_desks = new MSWindowsDesks(
        m_isPrimary, m_useHooks, m_screensaver, m_events,
        new TMethodJob<MSWindowsComputer>(this, &MSWindowsComputer::updateKeysCB)
    );
    m_keyState = new MSWindowsKeyState(
        m_desks, getEventTarget(), m_events, AppUtil::instance().getKeyboardLayoutList(), enableLangSync
    );

    updateComputerShape();
    m_class = createWindowClass();
    m_window = createWindow(m_class, kAppNameW);
    setupMouseKeys();
    LOG_DEBUG("computer shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : "");
    LOG_DEBUG("window is 0x%08x", m_window);

    if (Settings::value(Settings::Core::PreventSleep).toBool()) {
      m_powerManager.disableSleep();
    }

    OleInitialize(0);
  } catch (...) {
    delete m_keyState;
    delete m_desks;
    delete m_screensaver;
    destroyWindow(m_window);
    destroyClass(m_class);
    s_computer = nullptr;
    throw;
  }

  // install event handlers
  m_events->addHandler(EventTypes::System, m_events->getSystemTarget(), [this](const auto &e) {
    handleSystemEvent(e);
  });

  // install the platform event queue
  m_events->adoptBuffer(new MSWindowsEventQueueBuffer(m_events));
}

MSWindowsComputer::~MSWindowsComputer()
{
  assert(s_computer != nullptr);

  if (m_isEnabled) {
    disable();
  }
  m_events->adoptBuffer(nullptr);
  m_events->removeHandler(EventTypes::System, m_events->getSystemTarget());
  delete m_keyState;
  delete m_desks;
  delete m_screensaver;
  destroyWindow(m_window);
  destroyClass(m_class);

  OleUninitialize();

  s_computer = nullptr;
}

void MSWindowsComputer::init(HINSTANCE windowInstance)
{
  assert(s_windowInstance == nullptr);
  assert(windowInstance != nullptr);

  s_windowInstance = windowInstance;
}

HINSTANCE
MSWindowsComputer::getWindowInstance()
{
  return s_windowInstance;
}

void MSWindowsComputer::enable()
{
  LOG_DEBUG("enabling %s computer", m_isPrimary ? "primary" : "secondary");
  m_isEnabled = true;

  assert(m_isOnComputer == m_isPrimary);

  // we need to poll some things to fix them
  m_fixTimer = m_events->newTimer(1.0, nullptr);
  m_events->addHandler(EventTypes::Timer, m_fixTimer, [this](const auto &) { handleFixes(); });

  // install our clipboard snooper
  if (!AddClipboardFormatListener(m_window)) {
    LOG_WARN("failed to add the clipboard format listener: %d", GetLastError());
  }

  // track the active desk and (re)install the hooks
  m_desks->enable();

  if (m_isPrimary) {
    // set jump zones
    m_hook.setZone(m_x, m_y, m_w, m_h, getJumpZoneSize());

    // watch jump zones
    m_hook.setMode(kHOOK_WATCH_JUMP_ZONE);
  }
}

void MSWindowsComputer::disable()
{
  LOG_DEBUG("disabling %s computer", m_isPrimary ? "primary" : "secondary");
  m_isEnabled = false;

  // stop tracking the active desk
  m_desks->disable();

  if (m_isPrimary) {
    // disable hooks
    m_hook.setMode(kHOOK_DISABLE);

    // enable special key sequences on win95 family
    enableSpecialKeys(true);
  }

  // tell key state
  m_keyState->disable();

  // stop snooping the clipboard
  if (!RemoveClipboardFormatListener(m_window)) {
    LOG_WARN("failed to remove the clipboard format listener: %d", GetLastError());
  }

  // uninstall fix timer
  if (m_fixTimer != nullptr) {
    m_events->removeHandler(EventTypes::Timer, m_fixTimer);
    m_events->deleteTimer(m_fixTimer);
    m_fixTimer = nullptr;
  }

  m_isOnComputer = m_isPrimary;
}

void MSWindowsComputer::enter()
{
  m_desks->enter();
  if (m_isPrimary) {
    // enable special key sequences on win95 family
    enableSpecialKeys(true);

    // watch jump zones
    m_hook.setMode(kHOOK_WATCH_JUMP_ZONE);

    // all messages prior to now are invalid
    nextMark();

    m_primaryKeyDownList.clear();
  } else {
    // Entering a secondary computer. Ensure that no screensaver is active
    // and that the computer is not in powersave mode.
    ArchMiscWindows::wakeupDisplay();

    if (m_screensaver != nullptr && m_screensaverActive) {
      m_screensaver->deactivate();
      m_screensaverActive = 0;
    }
  }

  // now on computer
  m_isOnComputer = true;
  setupMouseKeys();
}

bool MSWindowsComputer::canLeave()
{
  POINT pos;
  if (!getThisCursorPos(&pos)) {
    // prevent computer leave when cursor position is not available; if unable to get cursor position,
    // computer will become inaccessible if the cursor leaves the computer.
    LOG_DEBUG("unable to leave computer, cursor position not available");
    return false;
  }

  return true;
}

void MSWindowsComputer::leave()
{
  // get keyboard layout of foreground window.  we'll use this
  // keyboard layout for translating keys sent to clients.
  m_keyLayout = AppUtilWindows::instance().getCurrentKeyboardLayout();

  // tell the key mapper about the keyboard layout
  m_keyState->setKeyLayout(m_keyLayout);

  // tell desk that we're leaving and tell it the keyboard layout
  m_desks->leave(m_keyLayout);

  if (m_isPrimary) {
    LOG_VERBOSE("centering cursor on leave: %+d, %+d", m_xCenter, m_yCenter);
    warpCursor(m_xCenter, m_yCenter);

    // disable special key sequences on win95 family
    enableSpecialKeys(false);

    // all messages prior to now are invalid
    nextMark();

    // remember the modifier state.  this is the modifier state
    // reflected in the internal keyboard state.
    m_keyState->saveModifiers();

    m_hook.setMode(kHOOK_RELAY_EVENTS);

    m_primaryKeyDownList.clear();
    for (KeyButton i = 0; i < IKeyState::s_numButtons; ++i) {
      if (m_keyState->isKeyDown(i)) {
        m_primaryKeyDownList.push_back(i);
        LOG_VERBOSE("key button %d is down before leaving to another computer", i);
      }
    }
  }

  // now off computer
  m_isOnComputer = false;
}

bool MSWindowsComputer::setClipboard(ClipboardID, const IClipboard *src)
{
  MSWindowsClipboard dst(m_window);
  if (src != nullptr) {
    // save clipboard data
    return Clipboard::copy(&dst, src);
  } else {
    // assert clipboard ownership
    if (!dst.open(0)) {
      return false;
    }
    dst.empty();
    dst.close();
    return true;
  }
}

void MSWindowsComputer::checkClipboards()
{
  // if we think we own the clipboard but we don't then somebody
  // grabbed the clipboard on this computer without us knowing.
  // tell the server that this computer grabbed the clipboard.
  //
  // this works around bugs in the clipboard viewer chain.
  // sometimes NT will simply never send WM_DRAWCLIPBOARD
  // messages for no apparent reason and rebooting fixes the
  // problem.  since we don't want a broken clipboard until the
  // next reboot we do this double check.  clipboard ownership
  // won't be reflected on other computers until we leave but at
  // least the clipboard itself will work.
  if (m_ownClipboard && !MSWindowsClipboard::isOwnedByDeskflow()) {
    LOG_DEBUG("clipboard changed: lost ownership and no notification received");
    m_ownClipboard = false;
    sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
    sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardSelection);
  }
}

void MSWindowsComputer::openScreensaver(bool notify)
{
  assert(m_screensaver != nullptr);

  m_screensaverNotify = notify;
  if (m_screensaverNotify) {
    m_desks->installScreensaverHooks(true);
  } else if (m_screensaver) {
    m_screensaver->disable();
  }
}

void MSWindowsComputer::closeScreensaver()
{
  if (m_screensaver != nullptr) {
    if (m_screensaverNotify) {
      m_desks->installScreensaverHooks(false);
    } else {
      m_screensaver->enable();
    }
  }
  m_screensaverNotify = false;
}

void MSWindowsComputer::screensaver(bool activate)
{
  assert(m_screensaver != nullptr);
  if (m_screensaver == nullptr)
    return;

  if (activate) {
    m_screensaver->activate();
  } else {
    m_screensaver->deactivate();
  }
}

void MSWindowsComputer::resetOptions()
{
  m_desks->resetOptions();
}

void MSWindowsComputer::setOptions(const OptionsList &options)
{
  m_desks->setOptions(options);
}

void MSWindowsComputer::setSequenceNumber(uint32_t seqNum)
{
  m_sequenceNumber = seqNum;
}

bool MSWindowsComputer::isPrimary() const
{
  return m_isPrimary;
}

void *MSWindowsComputer::getEventTarget() const
{
  return const_cast<MSWindowsComputer *>(this);
}

bool MSWindowsComputer::getClipboard(ClipboardID, IClipboard *dst) const
{
  MSWindowsClipboard src(m_window);
  Clipboard::copy(dst, &src);
  return true;
}

void MSWindowsComputer::getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const
{
  assert(m_class != 0);

  x = m_x;
  y = m_y;
  w = m_w;
  h = m_h;
}

void MSWindowsComputer::getCursorPos(int32_t &x, int32_t &y) const
{
  m_desks->getCursorPos(x, y);
}

/*
 * getThisCursorPos and setThisCursorPos will attempt to negotiate with the
 * system to try get the and set the mouse position, however on the logon screen
 * due to hooks this process has it may unable to work around the problem.
 * Although these functions did not fix the issue at hand (#5294) its worth
 * keeping them here anyway.
 */
bool MSWindowsComputer::getThisCursorPos(LPPOINT pos)
{
  auto result = GetCursorPos(pos);
  if (!result) {
    LOG_DEBUG("could not get cursor pos, error: %s", windowsErrorToString(GetLastError()).c_str());

    LOG_DEBUG("retrying get cursor pos");
    result = GetCursorPos(pos);
    if (!result) {
      LOG_DEBUG("could not get cursor pos, error: %s", windowsErrorToString(GetLastError()).c_str());

      updateDesktopThread();
    }
  }

  return result;
}

bool MSWindowsComputer::setThisCursorPos(int x, int y)
{
  auto result = SetCursorPos(x, y);
  if (!result) {
    LOG_DEBUG("could not set cursor pos, error: %s", windowsErrorToString(GetLastError()).c_str());

    LOG_DEBUG("retrying to set cursor pos");
    result = SetCursorPos(x, y);
    if (!result) {
      LOG_DEBUG("could not set cursor pos, error: %s", windowsErrorToString(GetLastError()).c_str());

      updateDesktopThread();
    }
  }

  return result;
}

void MSWindowsComputer::updateDesktopThread()
{
  LOG_DEBUG("updating desktop thread");

  HDESK hDesk = OpenInputDesktop(0, true, GENERIC_ALL);
  if (hDesk == nullptr) {
    LOG_DEBUG("could not open input desktop, error: %s", windowsErrorToString(GetLastError()).c_str());
    return;
  }

  if (!SetThreadDesktop(hDesk)) {
    LOG_DEBUG("could not set thread desktop, error: %s", windowsErrorToString(GetLastError()).c_str());
  }

  if (!CloseDesktop(hDesk)) {
    LOG_DEBUG("could not close desktop, error: %s", windowsErrorToString(GetLastError()).c_str());
  }
}

void MSWindowsComputer::reconfigure(uint32_t activeSides)
{
  assert(m_isPrimary);
  const static auto sidesText = sidesMaskToString(activeSides);
  LOG_DEBUG("active sides: %s (0x%02x)", sidesText.c_str(), activeSides);
  m_hook.setSides(activeSides);
}

uint32_t MSWindowsComputer::activeSides()
{
  return m_hook.getSides();
}

void MSWindowsComputer::warpCursor(int32_t x, int32_t y)
{
  // warp mouse
  warpCursorNoFlush(x, y);

  // remove all input events before and including warp
  MSG msg;
  while (PeekMessage(&msg, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_MSG_INPUT_LAST, PM_REMOVE)) {
    // do nothing
  }

  // save position to compute delta of next motion
  saveMousePosition(x, y);
}

void MSWindowsComputer::saveMousePosition(int32_t x, int32_t y)
{
  m_xCursor = x;
  m_yCursor = y;

  LOG_VERBOSE("saved mouse position for next delta: %+d,%+d", x, y);
}

uint32_t MSWindowsComputer::registerHotKey(KeyID key, KeyModifierMask mask)
{
  // only allow certain modifiers
  if ((mask & ~(KeyModifierShift | KeyModifierControl | KeyModifierAlt | KeyModifierSuper)) != 0) {
    // this should be a warning, but this can confuse users,
    // as this warning happens almost always.
    LOG_DEBUG("could not map hotkey id=%04x mask=%04x", key, mask);
    return 0;
  }

  // fail if no keys
  if (key == kKeyNone && mask == 0) {
    return 0;
  }

  // convert to win32
  UINT modifiers = 0;
  if ((mask & KeyModifierShift) != 0) {
    modifiers |= MOD_SHIFT;
  }
  if ((mask & KeyModifierControl) != 0) {
    modifiers |= MOD_CONTROL;
  }
  if ((mask & KeyModifierAlt) != 0) {
    modifiers |= MOD_ALT;
  }
  if ((mask & KeyModifierSuper) != 0) {
    modifiers |= MOD_WIN;
  }
  UINT vk = m_keyState->mapKeyToVirtualKey(key);
  if (key != kKeyNone && vk == 0) {
    // can't map key
    // this should be a warning, but this can confuse users,
    // as this warning happens almost always.
    LOG_DEBUG("could not map hotkey id=%04x mask=%04x", key, mask);
    return 0;
  }

  // choose hotkey id
  uint32_t id;
  if (!m_oldHotKeyIDs.empty()) {
    id = m_oldHotKeyIDs.back();
    m_oldHotKeyIDs.pop_back();
  } else {
    // id = m_hotKeys.size() + 1;
    id = (uint32_t)m_hotKeys.size() + 1;
  }

  // if this hot key has modifiers only then we'll handle it specially
  bool err;
  if (key == kKeyNone) {
    // check if already registered
    err = (m_hotKeyToIDMap.count(HotKeyItem(vk, modifiers)) > 0);
  } else {
    // register with OS
    err = (RegisterHotKey(nullptr, id, modifiers, vk) == 0);
  }

  if (!err) {
    m_hotKeys.try_emplace(id, HotKeyItem(vk, modifiers));
    m_hotKeyToIDMap[HotKeyItem(vk, modifiers)] = id;
  } else {
    m_oldHotKeyIDs.push_back(id);
    m_hotKeys.erase(id);
    LOG_WARN(
        "failed to register hotkey %s (id=%04x mask=%04x)", deskflow::KeyMap::formatKey(key, mask).c_str(), key, mask
    );

    return 0;
  }

  LOG_DEBUG(
      "registered hotkey %s (id=%04x mask=%04x) as id=%d", deskflow::KeyMap::formatKey(key, mask).c_str(), key, mask, id
  );
  return id;
}

void MSWindowsComputer::unregisterHotKey(uint32_t id)
{
  // look up hotkey
  HotKeyMap::iterator i = m_hotKeys.find(id);
  if (i == m_hotKeys.end()) {
    return;
  }

  // unregister with OS
  bool err;
  if (i->second.getVirtualKey() != 0) {
    err = !UnregisterHotKey(nullptr, id);
  } else {
    err = false;
  }
  if (err) {
    LOG_WARN("failed to unregister hotkey id=%d", id);
  } else {
    LOG_DEBUG("unregistered hotkey id=%d", id);
  }

  // discard hot key from map and record old id for reuse
  m_hotKeyToIDMap.erase(i->second);
  m_hotKeys.erase(i);
  m_oldHotKeyIDs.push_back(id);
}

void MSWindowsComputer::fakeInputBegin()
{
  assert(m_isPrimary);

  if (!m_isOnComputer) {
    m_keyState->useSavedModifiers(true);
  }
  m_desks->fakeInputBegin();
}

void MSWindowsComputer::fakeInputEnd()
{
  assert(m_isPrimary);

  m_desks->fakeInputEnd();
  if (!m_isOnComputer) {
    m_keyState->useSavedModifiers(false);
  }
}

int32_t MSWindowsComputer::getJumpZoneSize() const
{
  return 1;
}

bool MSWindowsComputer::isAnyMouseButtonDown(uint32_t &buttonID) const
{
  static const char *buttonToName[] = {"<invalid>",    "Left Button", "Middle Button",
                                       "Right Button", "X Button 1",  "X Button 2"};

  for (uint32_t i = 1; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
    if (m_buttons[i]) {
      buttonID = i;
      LOG_DEBUG("locked by \"%s\"", buttonToName[i]);
      return true;
    }
  }

  return false;
}

void MSWindowsComputer::getCursorCenter(int32_t &x, int32_t &y) const
{
  x = m_xCenter;
  y = m_yCenter;
}

void MSWindowsComputer::fakeMouseButton(ButtonID id, bool press)
{
  m_desks->fakeMouseButton(id, press);

  if (id == kButtonLeft) {
    m_buttons[kButtonLeft] = press;
  }
}

void MSWindowsComputer::fakeMouseMove(int32_t x, int32_t y)
{
  m_desks->fakeMouseMove(x, y);
}

void MSWindowsComputer::fakeMouseRelativeMove(int32_t dx, int32_t dy) const
{
  m_desks->fakeMouseRelativeMove(dx, dy);
}

void MSWindowsComputer::fakeMouseWheel(ScrollDelta delta) const
{
  delta = applyScrollModifier(delta);
  m_desks->fakeMouseWheel(delta.x, delta.y);
}

void MSWindowsComputer::updateKeys()
{
  m_desks->updateKeys();
}

void MSWindowsComputer::fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang)
{
  PlatformComputer::fakeKeyDown(id, mask, button, lang);
  updateMouseKeys();
}

bool MSWindowsComputer::fakeKeyRepeat(
    KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang
)
{
  bool result = PlatformComputer::fakeKeyRepeat(id, mask, count, button, lang);
  updateMouseKeys();
  return result;
}

bool MSWindowsComputer::fakeKeyUp(KeyButton button)
{
  bool result = PlatformComputer::fakeKeyUp(button);
  updateMouseKeys();
  return result;
}

void MSWindowsComputer::fakeAllKeysUp()
{
  PlatformComputer::fakeAllKeysUp();
  updateMouseKeys();
}

HCURSOR
MSWindowsComputer::createBlankCursor() const
{
  // create a transparent cursor
  int cw = GetSystemMetrics(SM_CXCURSOR);
  int ch = GetSystemMetrics(SM_CYCURSOR);

  uint8_t *cursorAND = new uint8_t[ch * ((cw + 31) >> 2)];
  uint8_t *cursorXOR = new uint8_t[ch * ((cw + 31) >> 2)];
  memset(cursorAND, 0xff, ch * ((cw + 31) >> 2));
  memset(cursorXOR, 0x00, ch * ((cw + 31) >> 2));
  HCURSOR c = CreateCursor(s_windowInstance, 0, 0, cw, ch, cursorAND, cursorXOR);
  delete[] cursorXOR;
  delete[] cursorAND;
  return c;
}

void MSWindowsComputer::destroyCursor(HCURSOR cursor) const
{
  if (cursor != nullptr) {
    DestroyCursor(cursor);
  }
}

ATOM MSWindowsComputer::createWindowClass() const
{
  WNDCLASSEX classInfo;
  classInfo.cbSize = sizeof(classInfo);
  classInfo.style = CS_DBLCLKS | CS_NOCLOSE;
  classInfo.lpfnWndProc = &MSWindowsComputer::wndProc;
  classInfo.cbClsExtra = 0;
  classInfo.cbWndExtra = 0;
  classInfo.hInstance = s_windowInstance;
  classInfo.hIcon = nullptr;
  classInfo.hCursor = nullptr;
  classInfo.hbrBackground = nullptr;
  classInfo.lpszMenuName = nullptr;
  classInfo.lpszClassName = kAppNameW;
  classInfo.hIconSm = nullptr;
  return RegisterClassEx(&classInfo);
}

void MSWindowsComputer::destroyClass(ATOM windowClass) const
{
  if (windowClass != 0) {
    UnregisterClass(MAKEINTATOM(windowClass), s_windowInstance);
  }
}

HWND MSWindowsComputer::createWindow(ATOM windowClass, const wchar_t *name) const
{
  HWND window = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, MAKEINTATOM(windowClass), name, WS_POPUP, 0, 0, 1, 1,
      nullptr, nullptr, s_windowInstance, nullptr
  );
  if (window == nullptr) {
    LOG_ERR("failed to create window: %d", GetLastError());
    throw DisplayOpenFailureException();
  }
  return window;
}

void MSWindowsComputer::destroyWindow(HWND hwnd) const
{
  if (hwnd != nullptr) {
    DestroyWindow(hwnd);
  }
}

void MSWindowsComputer::sendEvent(EventTypes type, void *data)
{
  m_events->addEvent(Event(type, getEventTarget(), data));
}

void MSWindowsComputer::sendClipboardEvent(EventTypes type, ClipboardID id)
{
  ClipboardInfo *info = (ClipboardInfo *)malloc(sizeof(ClipboardInfo));
  if (info == nullptr) {
    LOG_ERR("malloc failed on %s:%s", __FILE__, __LINE__);
    return;
  }
  info->m_id = id;
  info->m_sequenceNumber = m_sequenceNumber;
  sendEvent(type, info);
}

void MSWindowsComputer::handleSystemEvent(const Event &event)
{
  MSG *msg = static_cast<MSG *>(event.getData());
  assert(msg != nullptr);

  if (onPreDispatch(msg->hwnd, msg->message, msg->wParam, msg->lParam)) {
    return;
  }
  TranslateMessage(msg);
  DispatchMessage(msg);
}

void MSWindowsComputer::updateButtons()
{
  int numButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
  m_buttons[kButtonNone] = false;
  m_buttons[kButtonLeft] = (GetKeyState(VK_LBUTTON) < 0);
  m_buttons[kButtonRight] = (GetKeyState(VK_RBUTTON) < 0);
  m_buttons[kButtonMiddle] = (GetKeyState(VK_MBUTTON) < 0);
  m_buttons[kButtonExtra0 + 0] = (numButtons >= 4) && (GetKeyState(VK_XBUTTON1) < 0);
  m_buttons[kButtonExtra0 + 1] = (numButtons >= 5) && (GetKeyState(VK_XBUTTON2) < 0);
}

IKeyState *MSWindowsComputer::getKeyState() const
{
  return m_keyState;
}

bool MSWindowsComputer::onPreDispatch(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  // handle event
  switch (message) {
  case DESKFLOW_MSG_SCREEN_SAVER:
    return onScreensaver(wParam != 0);

  case DESKFLOW_MSG_DEBUG:
    LOG_VERBOSE("hook: 0x%08x 0x%08x", wParam, lParam);
    return true;
  }

  if (m_isPrimary) {
    return onPreDispatchPrimary(hwnd, message, wParam, lParam);
  }

  return false;
}

bool MSWindowsComputer::onPreDispatchPrimary(HWND, UINT message, WPARAM wParam, LPARAM lParam)
{
  LOG_VERBOSE("handling pre-dispatch primary");

  // handle event
  switch (message) {
  case DESKFLOW_MSG_MARK:
    return onMark(static_cast<uint32_t>(wParam));

  case DESKFLOW_MSG_KEY:
    return onKey(wParam, lParam);

  case DESKFLOW_MSG_MOUSE_BUTTON:
    return onMouseButton(wParam, lParam);

  case DESKFLOW_MSG_MOUSE_MOVE:
    return onMouseMove(static_cast<int32_t>(wParam), static_cast<int32_t>(lParam));

  case DESKFLOW_MSG_MOUSE_WHEEL:
    return onMouseWheel(static_cast<int32_t>(lParam), static_cast<int32_t>(wParam));

  case DESKFLOW_MSG_PRE_WARP: {
    // save position to compute delta of next motion
    saveMousePosition(static_cast<int32_t>(wParam), static_cast<int32_t>(lParam));

    // we warped the mouse.  discard events until we find the
    // matching post warp event.  see warpCursorNoFlush() for
    // where the events are sent.  we discard the matching
    // post warp event and can be sure we've skipped the warp
    // event.
    MSG msg;
    do {
      GetMessage(&msg, nullptr, DESKFLOW_MSG_MOUSE_MOVE, DESKFLOW_MSG_POST_WARP);
    } while (msg.message != DESKFLOW_MSG_POST_WARP);
  }
    return true;

  case DESKFLOW_MSG_POST_WARP:
    LOG_WARN("unmatched post warp");
    return true;

  case WM_HOTKEY:
    // we discard these messages.  we'll catch the hot key in the
    // regular key event handling, where we can detect both key
    // press and release.  we only register the hot key so no other
    // app will act on the key combination.
    break;
  }

  return false;
}

bool MSWindowsComputer::onEvent(HWND, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result)
{
  switch (msg) {

  case WM_CLIPBOARDUPDATE: {
    DWORD clipboardSequenceNumber = GetClipboardSequenceNumber();
    LOG_DEBUG("clipboard update: sequence number %d, current %d", clipboardSequenceNumber, m_clipboardSequenceNumber);

    if (clipboardSequenceNumber && (clipboardSequenceNumber != m_clipboardSequenceNumber)) {
      m_clipboardSequenceNumber = clipboardSequenceNumber;
      onClipboardChange();
    }
    return 0; // message processed
  }

  case WM_DISPLAYCHANGE:
    return onDisplayChange();

  /* On windows 10 we don't receive WM_POWERBROADCAST after sleep.
   We receive only WM_TIMECHANGE hence this message is used to resume.*/
  case WM_TIMECHANGE:
    m_events->addEvent( //
        Event(EventTypes::ComputerResume, getEventTarget(), nullptr, Event::EventFlags::DeliverImmediately)
    );
    break;

  case WM_POWERBROADCAST:
    switch (wParam) {
    case PBT_APMRESUMEAUTOMATIC:
    case PBT_APMRESUMECRITICAL:
    case PBT_APMRESUMESUSPEND:
      m_events->addEvent(
          Event(EventTypes::ComputerResume, getEventTarget(), nullptr, Event::EventFlags::DeliverImmediately)
      );
      break;

    case PBT_APMSUSPEND:
      m_events->addEvent(
          Event(EventTypes::ComputerSuspend, getEventTarget(), nullptr, Event::EventFlags::DeliverImmediately)
      );
      break;
    }
    *result = TRUE;
    return true;

  case WM_DEVICECHANGE: {
    // re-run mouse keys setup in case a mouse was plugged in or unplugged; i.e. if a mouse was
    // unplugged from the client, make sure the mouse cursor is still visible.
    // the device change event happens for every discreet hardware change, so if you're using a
    // usb switcher, this generates many device change events. it would be nice to log here but
    // the log would be too noisy.
    setupMouseKeys();
  } break;

  case WM_SETTINGCHANGE:
    // sometimes fired when the mouse keys setting is changed, but doesn't seem very reliable.
    // these events may arrive at any time (e.g. when the program is shutting down) if the message
    // loop stops being processed for any reason. this may be a bug or something out of our control.
    // forcing mouse keys on may help in scenarios where mouse keys are being turned off by another app.
    if (wParam == SPI_SETMOUSEKEYS) {
      LOG_DEBUG("mouse keys setting was changed");
      setupMouseKeys();
    }
    break;
  }

  return false;
}

bool MSWindowsComputer::onMark(uint32_t mark)
{
  m_markReceived = mark;
  return true;
}

bool MSWindowsComputer::onKey(WPARAM wParam, LPARAM lParam)
{
  static const KeyModifierMask s_ctrlAlt = KeyModifierControl | KeyModifierAlt;

  LOG_VERBOSE(
      "event: Key char=%d, vk=0x%02x, nagr=%d, lParam=0x%08x", (wParam & 0xffffu), (wParam >> 16) & 0xffu,
      (wParam & 0x1000000u) ? 1 : 0, lParam
  );

  // get event info
  KeyButton button = (KeyButton)((lParam & 0x01ff0000) >> 16);
  bool down = ((lParam & 0x80000000u) == 0x00000000u);
  bool wasDown = isKeyDown(button);
  KeyModifierMask oldState = pollActiveModifiers();

  // check for autorepeat
  if (m_keyState->testAutoRepeat(down, (lParam & 0x40000000u), button)) {
    lParam |= 0x40000000u;
  }

  // if the button is zero then guess what the button should be.
  // these are badly synthesized key events and logitech software
  // that maps mouse buttons to keys is known to do this.
  // alternatively, we could just throw these events out.
  if (button == 0) {
    button = m_keyState->virtualKeyToButton((wParam >> 16) & 0xffu);
    if (button == 0) {
      return true;
    }
    wasDown = isKeyDown(button);
  }

  // record keyboard state
  m_keyState->onKey(button, down, oldState);

  if (!down && m_isPrimary && !m_isOnComputer) {
    PrimaryKeyDownList::iterator find = std::find(m_primaryKeyDownList.begin(), m_primaryKeyDownList.end(), button);
    if (find != m_primaryKeyDownList.end()) {
      LOG_VERBOSE("release key button %d on primary", *find);
      m_hook.setMode(kHOOK_WATCH_JUMP_ZONE);
      fakeLocalKey(*find, false);
      m_primaryKeyDownList.erase(find);
      m_hook.setMode(kHOOK_RELAY_EVENTS);
      return true;
    }
  }

  // windows doesn't tell us the modifier key state on mouse or key
  // events so we have to figure it out.  most apps would use
  // GetKeyState() or even GetAsyncKeyState() for that but we can't
  // because our hook doesn't pass on key events for several modifiers.
  // it can't otherwise the system would interpret them normally on
  // the primary computer even when on a secondary computer.  so tapping
  // alt would activate menus and tapping the windows key would open
  // the start menu.  if you don't pass those events on in the hook
  // then GetKeyState() understandably doesn't reflect the effect of
  // the event.  curiously, neither does GetAsyncKeyState(), which is
  // surprising.
  //
  // so anyway, we have to track the modifier state ourselves for
  // at least those modifiers we don't pass on.  pollActiveModifiers()
  // does that but we have to update the keyboard state before calling
  // pollActiveModifiers() to get the right answer.  but the only way
  // to set the modifier state or to set the up/down state of a key
  // is via onKey().  so we have to call onKey() twice.
  KeyModifierMask state = pollActiveModifiers();
  m_keyState->onKey(button, down, state);

  // check for hot keys
  if (oldState != state) {
    // modifier key was pressed/released
    if (onHotKey(0, lParam)) {
      return true;
    }
  } else {
    // non-modifier was pressed/released
    if (onHotKey(wParam, lParam)) {
      return true;
    }
  }

  // stop sending modifier keys over and over again
  // but allow up to be sent if the previous state was down
  // to prevent stuck modifier keys on the client
  if (down && wasDown && isModifierRepeat(oldState, state, wParam)) {
    return true;
  }

  // ignore message if posted prior to last mark change
  if (!ignore()) {
    // check for ctrl+alt+del.  we do not want to pass that to the
    // client.  the user can use ctrl+alt+pause to emulate it.
    UINT virtKey = ((wParam >> 16) & 0xffu);
    if (virtKey == VK_DELETE && (state & s_ctrlAlt) == s_ctrlAlt) {
      LOG_DEBUG("discard ctrl+alt+del");
      return true;
    }

    // check for ctrl+alt+del emulation
    if ((virtKey == VK_PAUSE || virtKey == VK_CANCEL) && (state & s_ctrlAlt) == s_ctrlAlt) {
      LOG_DEBUG("emulate ctrl+alt+del");
      // switch wParam and lParam to be as if VK_DELETE was
      // pressed or released.  when mapping the key we require that
      // we not use AltGr (the 0x10000 flag in wParam) and we not
      // use the keypad delete key (the 0x01000000 flag in lParam).
      wParam = (VK_DELETE << 16) | 0x01000000u;
      lParam &= 0xfe000000;
      lParam |= m_keyState->virtualKeyToButton(VK_DELETE) << 16;
      lParam |= 0x01000001;
    }

    // process key
    KeyModifierMask mask;
    KeyID key = m_keyState->mapKeyFromEvent(wParam, lParam, &mask);
    const KeyButton mappedButton = static_cast<KeyButton>((lParam & 0x01ff0000u) >> 16);
    if (mappedButton != 0) {
      // Ctrl+Alt+Del emulation deliberately substitutes its button in lParam.
      // Otherwise retain the virtual-key fallback above for malformed input
      // that has no scan code, so its key-up matches the key-down on clients.
      button = mappedButton;
    }
    if (key != kKeyNone) {
      // do it
      m_keyState->sendKeyEvent(
          getEventTarget(), ((lParam & 0x80000000u) == 0), ((lParam & 0x40000000u) != 0), key, mask,
          (int32_t)(lParam & 0xffff), button
      );
    } else {
      LOG_VERBOSE("cannot map key");
    }
  }

  return true;
}

bool MSWindowsComputer::onHotKey(WPARAM wParam, LPARAM lParam)
{
  // get the key info
  KeyModifierMask state = getActiveModifiers();
  UINT virtKey = ((wParam >> 16) & 0xffu);
  UINT modifiers = 0;
  if ((state & KeyModifierShift) != 0) {
    modifiers |= MOD_SHIFT;
  }
  if ((state & KeyModifierControl) != 0) {
    modifiers |= MOD_CONTROL;
  }
  if ((state & KeyModifierAlt) != 0) {
    modifiers |= MOD_ALT;
  }
  if ((state & KeyModifierSuper) != 0) {
    modifiers |= MOD_WIN;
  }

  // find the hot key id
  HotKeyToIDMap::const_iterator i = m_hotKeyToIDMap.find(HotKeyItem(virtKey, modifiers));
  if (i == m_hotKeyToIDMap.end()) {
    return false;
  }

  // find what kind of event
  EventTypes type;
  if ((lParam & 0x80000000u) == 0u) {
    if ((lParam & 0x40000000u) != 0u) {
      // ignore key repeats but it counts as a hot key
      return true;
    }
    type = EventTypes::PrimaryComputerHotkeyDown;
  } else {
    type = EventTypes::PrimaryComputerHotkeyUp;
  }

  // generate event
  m_events->addEvent(Event(type, getEventTarget(), HotKeyInfo::alloc(i->second)));

  return true;
}

bool MSWindowsComputer::onMouseButton(WPARAM wParam, LPARAM lParam)
{
  // get which button
  bool pressed = mapPressFromEvent(wParam, lParam);
  ButtonID button = mapButtonFromEvent(wParam, lParam);

  // keep our shadow key state up to date
  if (button >= kButtonLeft && button <= kButtonExtra0 + 1) {
    m_buttons[button] = pressed;
  }

  // ignore message if posted prior to last mark change
  if (!ignore()) {
    KeyModifierMask mask = m_keyState->getActiveModifiers();
    if (pressed) {
      LOG_VERBOSE("event: button press button=%d", button);
      if (button != kButtonNone) {
        sendEvent(EventTypes::PrimaryComputerButtonDown, ButtonInfo::alloc(button, mask));
      }
    } else {
      LOG_VERBOSE("event: button release button=%d", button);
      if (button != kButtonNone) {
        sendEvent(EventTypes::PrimaryComputerButtonUp, ButtonInfo::alloc(button, mask));
      }
    }
  }

  return true;
}

// here's how mouse movements are sent across the network to a client:
//   1. deskflow checks the mouse position on server computer
//   2. records the delta (current x,y minus last x,y)
//   3. records the current x,y as "last" (so we can calc delta next time)
//   4. on the server, puts the cursor back to the center of the computer
//      - remember the cursor is hidden on the server at this point
//      - this actually records the current x,y as "last" a second time (it
//      seems)
//   5. sends the delta movement to the client (could be +1,+1 or -1,+4 for
//   example)
bool MSWindowsComputer::onMouseMove(int32_t mx, int32_t my)
{
  // compute motion delta (relative to the last known
  // mouse position)
  int32_t x = mx - m_xCursor;
  int32_t y = my - m_yCursor;

  LOG_VERBOSE("mouse move - motion delta: %+d=(%+d - %+d),%+d=(%+d - %+d)", x, mx, m_xCursor, y, my, m_yCursor);

  // ignore if the mouse didn't move or if message posted prior
  // to last mark change.
  if (ignore() || (x == 0 && y == 0)) {
    return true;
  }

  // save position to compute delta of next motion
  saveMousePosition(mx, my);

  if (m_isOnComputer) {
    // motion on primary computer
    sendEvent(EventTypes::PrimaryComputerMotionOnPrimary, MotionInfo::alloc(m_xCursor, m_yCursor));
  } else {
    // the motion is on the secondary computer, so we warp mouse back to
    // center on the server computer. if we don't do this, then the mouse
    // will always try to return to the original entry point on the
    // secondary computer.
    LOG_VERBOSE("centering cursor on motion: %+d,%+d", m_xCenter, m_yCenter);
    warpCursorNoFlush(m_xCenter, m_yCenter);

    // examine the motion.  if it's about the distance
    // from the center of the computer to an edge then
    // it's probably a bogus motion that we want to
    // ignore (see warpCursorNoFlush() for a further
    // description).
    static int32_t bogusZoneSize = 10;
    if (-x + bogusZoneSize > m_xCenter - m_x || x + bogusZoneSize > m_x + m_w - m_xCenter ||
        -y + bogusZoneSize > m_yCenter - m_y || y + bogusZoneSize > m_y + m_h - m_yCenter) {

      LOG_DEBUG("dropped bogus delta motion: %+d,%+d", x, y);
    } else {
      // send motion
      sendEvent(EventTypes::PrimaryComputerMotionOnSecondary, MotionInfo::alloc(x, y));
    }
  }

  return true;
}

bool MSWindowsComputer::onMouseWheel(int32_t xDelta, int32_t yDelta)
{
  // ignore message if posted prior to last mark change
  if (!ignore()) {
    LOG_VERBOSE("event: button wheel delta=%+d,%+d", xDelta, yDelta);
    sendEvent(EventTypes::PrimaryComputerWheel, WheelInfo::alloc(xDelta, yDelta));
  }
  return true;
}

bool MSWindowsComputer::onScreensaver(bool activated)
{
  // ignore this message if there are any other screen saver
  // messages already in the queue.  this is important because
  // our checkStarted() function has a deliberate delay, so it
  // can't respond to events at full CPU speed and will fall
  // behind if a lot of screen saver events are generated.
  // that can easily happen because windows will continually
  // send SC_SCREENSAVE until the screen saver starts, even if
  // the screen saver is disabled!
  MSG msg;
  if (PeekMessage(&msg, nullptr, DESKFLOW_MSG_SCREEN_SAVER, DESKFLOW_MSG_SCREEN_SAVER, PM_NOREMOVE)) {
    return true;
  }

  if (activated) {
    if (!m_screensaverActive && m_screensaver->checkStarted(DESKFLOW_MSG_SCREEN_SAVER, FALSE, 0)) {
      m_screensaverActive = true;
      sendEvent(EventTypes::PrimaryScreenSaverActivated);
    }
  } else {
    if (m_screensaverActive) {
      m_screensaverActive = false;
      sendEvent(EventTypes::PrimaryScreenSaverDeactivated);
    }
  }

  return true;
}

bool MSWindowsComputer::onDisplayChange()
{
  // computer resolution may have changed.  save old shape.
  int32_t xOld = m_x, yOld = m_y, wOld = m_w, hOld = m_h;

  // update shape
  updateComputerShape();

  // do nothing if resolution hasn't changed
  if (xOld != m_x || yOld != m_y || wOld != m_w || hOld != m_h) {
    if (m_isPrimary) {
      if (!m_isOnComputer) {
        LOG_VERBOSE("centering cursor on display change: %+d, %+d", m_xCenter, m_yCenter);
        warpCursor(m_xCenter, m_yCenter);
      }

      // tell hook about resize if on computer
      else {
        m_hook.setZone(m_x, m_y, m_w, m_h, getJumpZoneSize());
      }
    }

    // send new computer info
    sendEvent(EventTypes::ComputerShapeChanged);

    LOG_DEBUG("computer shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : "");
  }

  return true;
}

void MSWindowsComputer::onClipboardChange()
{
  // now notify client that somebody changed the clipboard (unless
  // we're the owner).
  if (!MSWindowsClipboard::isOwnedByDeskflow()) {
    if (m_ownClipboard) {
      LOG_DEBUG("clipboard changed: lost ownership");
      m_ownClipboard = false;
      sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
      sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardSelection);
    }
  } else if (!m_ownClipboard) {
    LOG_DEBUG("clipboard changed: %s owned", kAppId);
    m_ownClipboard = true;
  }
}

void MSWindowsComputer::warpCursorNoFlush(int32_t x, int32_t y)
{
  // send an event that we can recognize before the mouse warp
  PostThreadMessage(GetCurrentThreadId(), DESKFLOW_MSG_PRE_WARP, x, y);

  // warp mouse.  hopefully this inserts a mouse motion event
  // between the previous message and the following message.
  setThisCursorPos(x, y);

  // check to see if the mouse pos was set correctly
  POINT cursorPos;
  getThisCursorPos(&cursorPos);

  // there is a bug or round error in SetCursorPos and GetCursorPos on
  // a high DPI setting. The check here is for Vista/7 login screen.
  // since this feature is mainly for client, so only check on client.
  if (!isPrimary()) {
    if ((cursorPos.x != x) && (cursorPos.y != y)) {
      LOG_DEBUG("function 'SetCursorPos' failed; trying 'fakeMouseMove'");
      LOG_DEBUG("cursor pos %d, %d expected pos %d, %d", cursorPos.x, cursorPos.y, x, y);
      // when at Vista/7 login screen, SetCursorPos does not work (which could
      // be an MS security feature). instead we can use fakeMouseMove, which
      // calls mouse_event. IMPORTANT: as of implementing this function, it has
      // an annoying side effect; instead of the mouse returning to the correct
      // exit point, it returns to the center of the computer. this could have
      // something to do with the center computer warping technique used (see
      // comments for onMouseMove definition).
      fakeMouseMove(x, y);
    }
  }

  // yield the CPU.  there's a race condition when warping:
  //   a hardware mouse event occurs
  //   the mouse hook is not called because that process doesn't have the CPU
  //   we send PRE_WARP, SetCursorPos(), send POST_WARP
  //   we process all of those events and update m_x, m_y
  //   we finish our time slice
  //   the hook is called
  //   the hook sends us a mouse event from the pre-warp position
  //   we get the CPU
  //   we compute a bogus warp
  // we need the hook to process all mouse events that occur
  // before we warp before we do the warp but i'm not sure how
  // to guarantee that.  yielding the CPU here may reduce the
  // chance of undesired behavior.  we'll also check for very
  // large motions that look suspiciously like about half width
  // or height of the computer.
  Arch::sleep(0.0);

  // send an event that we can recognize after the mouse warp
  PostThreadMessage(GetCurrentThreadId(), DESKFLOW_MSG_POST_WARP, 0, 0);
}

void MSWindowsComputer::nextMark()
{
  // next mark
  ++m_mark;

  // mark point in message queue where the mark was changed
  PostThreadMessage(GetCurrentThreadId(), DESKFLOW_MSG_MARK, m_mark, 0);
}

bool MSWindowsComputer::ignore() const
{
  return (m_mark != m_markReceived);
}

void MSWindowsComputer::updateComputerShape()
{
  // get shape and center
  m_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  m_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  m_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
  m_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
  m_xCenter = GetSystemMetrics(SM_CXSCREEN) >> 1;
  m_yCenter = GetSystemMetrics(SM_CYSCREEN) >> 1;

  // check for multiple monitors
  m_multimon = (m_w != GetSystemMetrics(SM_CXSCREEN) || m_h != GetSystemMetrics(SM_CYSCREEN));

  // tell the desks
  m_desks->setShape(m_x, m_y, m_w, m_h, m_xCenter, m_yCenter, m_multimon);
}

void MSWindowsComputer::handleFixes()
{
  // fix clipboard chain
  fixClipboardViewer();

  // update keys if keyboard layouts have changed
  if (m_keyState->didGroupsChange()) {
    updateKeys();
  }
}

void MSWindowsComputer::fixClipboardViewer()
{
  // XXX -- disable this code for now.  somehow it can cause an infinite
  // recursion in the WM_DRAWCLIPBOARD handler.  either we're sending
  // the message to our own window or some window farther down the chain
  // forwards the message to our window or a window farther up the chain.
  // i'm not sure how that could happen.  the m_nextClipboardWindow = nullptr
  // was not in the code that infinite loops and may fix the bug but i
  // doubt it.
  /*
      ChangeClipboardChain(m_window, m_nextClipboardWindow);
      m_nextClipboardWindow = nullptr;
      m_nextClipboardWindow = SetClipboardViewer(m_window);
  */
}

void MSWindowsComputer::enableSpecialKeys(bool enable) const
{
}

ButtonID MSWindowsComputer::mapButtonFromEvent(WPARAM msg, LPARAM button) const
{
  switch (msg) {
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_LBUTTONUP:
  case WM_NCLBUTTONDOWN:
  case WM_NCLBUTTONDBLCLK:
  case WM_NCLBUTTONUP:
    return kButtonLeft;

  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
  case WM_MBUTTONUP:
  case WM_NCMBUTTONDOWN:
  case WM_NCMBUTTONDBLCLK:
  case WM_NCMBUTTONUP:
    return kButtonMiddle;

  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
  case WM_RBUTTONUP:
  case WM_NCRBUTTONDOWN:
  case WM_NCRBUTTONDBLCLK:
  case WM_NCRBUTTONUP:
    return kButtonRight;

  case WM_XBUTTONDOWN:
  case WM_XBUTTONDBLCLK:
  case WM_XBUTTONUP:
  case WM_NCXBUTTONDOWN:
  case WM_NCXBUTTONDBLCLK:
  case WM_NCXBUTTONUP:
    switch (button) {
    case XBUTTON1:
      if (GetSystemMetrics(SM_CMOUSEBUTTONS) >= 4) {
        return kButtonExtra0;
      }
      break;

    case XBUTTON2:
      if (GetSystemMetrics(SM_CMOUSEBUTTONS) >= 5) {
        return kButtonExtra1;
      }
      break;
    }
    return kButtonNone;

  default:
    return kButtonNone;
  }
}

bool MSWindowsComputer::mapPressFromEvent(WPARAM msg, LPARAM) const
{
  switch (msg) {
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_XBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_XBUTTONDBLCLK:
  case WM_NCLBUTTONDOWN:
  case WM_NCMBUTTONDOWN:
  case WM_NCRBUTTONDOWN:
  case WM_NCXBUTTONDOWN:
  case WM_NCLBUTTONDBLCLK:
  case WM_NCMBUTTONDBLCLK:
  case WM_NCRBUTTONDBLCLK:
  case WM_NCXBUTTONDBLCLK:
    return true;

  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_XBUTTONUP:
  case WM_NCLBUTTONUP:
  case WM_NCMBUTTONUP:
  case WM_NCRBUTTONUP:
  case WM_NCXBUTTONUP:
    return false;

  default:
    return false;
  }
}

void MSWindowsComputer::updateKeysCB(const void *)
{
  // record which keys we think are down
  bool down[IKeyState::s_numButtons];
  bool sendFixes = (isPrimary() && !m_isOnComputer);
  if (sendFixes) {
    for (KeyButton i = 0; i < IKeyState::s_numButtons; ++i) {
      down[i] = m_keyState->isKeyDown(i);
    }
  }

  // update layouts if necessary
  if (m_keyState->didGroupsChange()) {
    PlatformComputer::updateKeyMap();
  }

  // now update the keyboard state
  PlatformComputer::updateKeyState();

  // now see which keys we thought were down but now think are up.
  // send key releases for these keys to the active client.
  if (sendFixes) {
    KeyModifierMask mask = pollActiveModifiers();
    for (KeyButton i = 0; i < IKeyState::s_numButtons; ++i) {
      if (down[i] && !m_keyState->isKeyDown(i)) {
        m_keyState->sendKeyEvent(getEventTarget(), false, false, kKeyNone, mask, 1, i);
      }
    }
  }
}

void MSWindowsComputer::setupMouseKeys()
{
  // we only need to enable the mouse keys feature when on a secondary computer.
  // this tricks windows into showing the mouse cursor when there is no real mouse.
  if (m_isPrimary) {
    // silent return to avoid noise.
    return;
  }

  // this is the case when there is some kind of a mouse (real or simulated by mouse keys).
  m_hasMouse = (GetSystemMetrics(SM_MOUSEPRESENT) != 0);
  if (m_hasMouse) {
    // silent return to avoid noise.
    return;
  }

  // prevents mouse keys being configured again when the program is shutting down since this function
  // is called based on system events such as system setting changes or hardware changes which
  // can occur at any time.
  if (!m_isEnabled) {
    LOG_DEBUG("mouse keys setup skipped, computer is not enabled");
    return;
  }

  m_mouseKeys.cbSize = sizeof(m_mouseKeys);
  m_gotMouseKeys = (SystemParametersInfo(SPI_GETMOUSEKEYS, m_mouseKeys.cbSize, &m_mouseKeys, 0) != 0);
  if (!m_gotMouseKeys) {
    LOG_ERR("unable to get old mouse keys settings, error: %d", GetLastError());
    return;
  }

  updateMouseKeys();
}

void MSWindowsComputer::updateMouseKeys()
{
  if (m_hasMouse || !m_gotMouseKeys || m_isPrimary) {
    // silent return to avoid noise.
    return;
  }

  DWORD oldFlags = m_mouseKeys.dwFlags;

  // turn on the windows mouse keys accessibility feature.
  // this is referred to as 'MouseKeys' in the docs.
  // makes the mouse cursor visible if there is no real mouse.
  //
  // historically, we would only set the `MKF_REPLACENUMBERS` flag when num lock is on.
  // however, this was a strange hidden feature that the user will most likely not expect;
  // it's probably more sensible to always set the `MKF_REPLACENUMBERS` flag, so that when the
  // mouse keys feature is left on after the program exits, the num pad on a local keyboard still
  // types numbers instead of moving the mouse cursor around (which would surprise most users).
  //
  // by default, windows 11 shows the mouse keys status in the system tray, but turning this on
  // might actually cause confusion for users who are not familiar with the mouse keys feature.
  m_mouseKeys.dwFlags = MKF_AVAILABLE | MKF_MOUSEKEYSON | MKF_REPLACENUMBERS;

  // only update the mouse keys settings if different to avoid noise.
  if (oldFlags == m_mouseKeys.dwFlags) {
    // silent return to avoid noise.
    return;
  }

  // we used to restore the old mouse keys settings but toggling the mouse keys feature on and off
  // causes the mouse cursor to be come stuck in an invisible state even when there is a real mouse.
  // we may want to reintroduce it (restore old mouse keys flags) as a user option in the future,
  // e.g. for users who use periodically use their windows client directly and use the numpad for cursor keys.
  LOG_INFO("enabling mouse keys for cursor visibility");
  LOG_DEBUG("setting mouse keys flags: 0x%08x", m_mouseKeys.dwFlags);
  const auto ok = SystemParametersInfo(SPI_SETMOUSEKEYS, m_mouseKeys.cbSize, &m_mouseKeys, SPIF_SENDCHANGE);
  if (!ok) {
    LOG_ERR("failed to set mouse keys, error: %d", GetLastError());
  } else {
    LOG_VERBOSE("mouse keys enabled successfully");
  }
}

LRESULT CALLBACK MSWindowsComputer::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  assert(s_computer != nullptr);

  LRESULT result = 0;
  if (!s_computer->onEvent(hwnd, msg, wParam, lParam, &result)) {
    result = DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return result;
}

void MSWindowsComputer::fakeLocalKey(KeyButton button, bool press) const
{
  INPUT input;
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = m_keyState->mapButtonToVirtualKey(button);
  DWORD pressFlag = press ? KEYEVENTF_EXTENDEDKEY : KEYEVENTF_KEYUP;
  input.ki.dwFlags = pressFlag;
  input.ki.time = 0;
  input.ki.dwExtraInfo = 0;
  SendInput(1, &input, sizeof(input));
}

//
// MSWindowsComputer::HotKeyItem
//

MSWindowsComputer::HotKeyItem::HotKeyItem(UINT keycode, UINT mask) : m_keycode(keycode), m_mask(mask)
{
  // do nothing
}

UINT MSWindowsComputer::HotKeyItem::getVirtualKey() const
{
  return m_keycode;
}

bool MSWindowsComputer::HotKeyItem::operator<(const HotKeyItem &x) const
{
  return (m_keycode < x.m_keycode || (m_keycode == x.m_keycode && m_mask < x.m_mask));
}

std::string MSWindowsComputer::getSecureInputApp() const
{
  // ignore on Windows
  return "";
}

bool MSWindowsComputer::isModifierRepeat(KeyModifierMask oldState, KeyModifierMask state, WPARAM wParam) const
{
  bool result = false;

  if (oldState == state && state != 0) {
    UINT virtKey = ((wParam >> 16) & 0xffu);
    if ((state & KeyModifierShift) != 0 && (virtKey == VK_LSHIFT || virtKey == VK_RSHIFT)) {
      result = true;
    }
    if ((state & KeyModifierControl) != 0 && (virtKey == VK_LCONTROL || virtKey == VK_RCONTROL)) {
      result = true;
    }
    if ((state & KeyModifierAlt) != 0 && (virtKey == VK_LMENU || virtKey == VK_RMENU)) {
      result = true;
    }
    if ((state & KeyModifierSuper) != 0 && (virtKey == VK_LWIN || virtKey == VK_RWIN)) {
      result = true;
    }
  }

  return result;
}
