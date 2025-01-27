/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsScreen.h"

#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/Stopwatch.h"
#include "base/TMethodEventJob.h"
#include "deskflow/App.h"
#include "deskflow/ArgsBase.h"
#include "deskflow/ClientApp.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/Clipboard.h"
#include "deskflow/KeyMap.h"
#include "deskflow/XScreen.h"
#include "platform/XWindowsClipboard.h"
#include "platform/XWindowsEventQueueBuffer.h"
#include "platform/XWindowsKeyState.h"
#include "platform/XWindowsScreenSaver.h"
#include "platform/XWindowsUtil.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#if X_DISPLAY_MISSING
#error X11 is required to build deskflow
#else
#include <X11/X.h>
#include <X11/Xutil.h>
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <X11/keysymdef.h>
#if HAVE_X11_EXTENSIONS_DPMS_H
extern "C"
{
#include <X11/extensions/dpms.h>
}
#endif
#if HAVE_X11_EXTENSIONS_XTEST_H
#include <X11/extensions/XTest.h>
#else
#error The XTest extension is required to build deskflow
#endif
#if HAVE_X11_EXTENSIONS_XINERAMA_H
// Xinerama.h may lack extern "C" for inclusion by C++
extern "C"
{
#include <X11/extensions/Xinerama.h>
}
#endif
#if HAVE_X11_EXTENSIONS_XRANDR_H
#include <X11/extensions/Xrandr.h>
#endif
#if HAVE_XKB_EXTENSION
#include <X11/XKBlib.h>
#endif
#ifdef HAVE_XI2
#include <X11/extensions/XInput2.h>
#endif
#endif

static int xi_opcode;

//
// XWindowsScreen
//

// NOTE -- the X display is shared among several objects but is owned
// by the XWindowsScreen.  Xlib is not reentrant so we must ensure
// that no two objects can simultaneously call Xlib with the display.
// this is easy since we only make X11 calls from the main thread.
// we must also ensure that these objects do not use the display in
// their destructors or, if they do, we can tell them not to.  This
// is to handle unexpected disconnection of the X display, when any
// call on the display is invalid.  In that situation we discard the
// display and the X11 event queue buffer, ignore any calls that try
// to use the display, and wait to be destroyed.

XWindowsScreen *XWindowsScreen::s_screen = NULL;

XWindowsScreen::XWindowsScreen(
    const char *displayName, bool isPrimary, bool disableXInitThreads, int mouseScrollDelta, IEventQueue *events,
    deskflow::ClientScrollDirection scrollDirection
)
    : PlatformScreen(events, scrollDirection),
      m_isPrimary(isPrimary),
      m_mouseScrollDelta(mouseScrollDelta),
      m_display(NULL),
      m_root(None),
      m_window(None),
      m_isOnScreen(m_isPrimary),
      m_x(0),
      m_y(0),
      m_w(0),
      m_h(0),
      m_xCenter(0),
      m_yCenter(0),
      m_xCursor(0),
      m_yCursor(0),
      m_keyState(NULL),
      m_lastFocus(None),
      m_lastFocusRevert(RevertToNone),
      m_im(NULL),
      m_ic(NULL),
      m_lastKeycode(0),
      m_sequenceNumber(0),
      m_screensaver(NULL),
      m_screensaverNotify(false),
      m_xtestIsXineramaUnaware(true),
      m_preserveFocus(false),
      m_xkb(false),
      m_xi2detected(false),
      m_xrandr(false),
      m_events(events)
{
  assert(s_screen == NULL);

  if (mouseScrollDelta == 0)
    m_mouseScrollDelta = 120;
  s_screen = this;

  if (!disableXInitThreads) {
    // initializes Xlib support for concurrent threads.
    if (XInitThreads() == 0)
      throw XArch("XInitThreads() returned zero");
  } else {
    LOG((CLOG_DEBUG "skipping XInitThreads()"));
  }

  // set the X I/O error handler so we catch the display disconnecting
  XSetIOErrorHandler(&XWindowsScreen::ioErrorHandler);

  try {
    m_display = openDisplay(displayName);
    m_root = DefaultRootWindow(m_display);
    saveShape();
    m_window = openWindow();
    m_screensaver = new XWindowsScreenSaver(m_display, m_window, getEventTarget(), events);
    m_keyState = new XWindowsKeyState(m_display, m_xkb, events, m_keyMap);
    LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_xinerama ? "(xinerama)" : ""));
    LOG((CLOG_DEBUG "window is 0x%08x", m_window));
  } catch (...) {
    if (m_display != NULL) {
      XCloseDisplay(m_display);
    }
    throw;
  }

  // primary/secondary screen only initialization
  if (m_isPrimary) {
#ifdef HAVE_XI2
    m_xi2detected = detectXI2();
    if (m_xi2detected) {
      selectXIRawMotion();
    } else
#endif
    {
      // start watching for events on other windows
      selectEvents(m_root);
    }

    // prepare to use input methods
    openIM();
  } else {
    // become impervious to server grabs
    XTestGrabControl(m_display, True);
  }

  // disable sleep if the flag is set
  if (App::instance().argsBase().m_preventSleep) {
    m_powerManager.disableSleep();
  }

  // initialize the clipboards
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    m_clipboard[id] = new XWindowsClipboard(m_display, m_window, id);
  }

  // install event handlers
  m_events->adoptHandler(
      Event::kSystem, m_events->getSystemTarget(),
      new TMethodEventJob<XWindowsScreen>(this, &XWindowsScreen::handleSystemEvent)
  );

  // install the platform event queue
  m_events->adoptBuffer(new XWindowsEventQueueBuffer(m_display, m_window, m_events));
}

XWindowsScreen::~XWindowsScreen()
{
  assert(s_screen != NULL);
  assert(m_display != NULL);

  m_events->adoptBuffer(NULL);
  m_events->removeHandler(Event::kSystem, m_events->getSystemTarget());
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    delete m_clipboard[id];
  }
  delete m_keyState;
  delete m_screensaver;
  m_keyState = NULL;
  m_screensaver = NULL;
  if (m_display != NULL) {
    // FIXME -- is it safe to clean up the IC and IM without a display?
    if (m_ic != NULL) {
      XDestroyIC(m_ic);
    }
    if (m_im != NULL) {
      XCloseIM(m_im);
    }
    XDestroyWindow(m_display, m_window);
    XCloseDisplay(m_display);
  }
  XSetIOErrorHandler(NULL);

  s_screen = NULL;
}

void XWindowsScreen::enable()
{
  if (!m_isPrimary) {
    // get the keyboard control state
    XKeyboardState keyControl;
    XGetKeyboardControl(m_display, &keyControl);
    m_autoRepeat = (keyControl.global_auto_repeat == AutoRepeatModeOn);
    m_keyState->setAutoRepeat(keyControl);

    // move hider window under the cursor center
    XMoveWindow(m_display, m_window, m_xCenter, m_yCenter);

    // raise and show the window
    // FIXME -- take focus?
    XMapRaised(m_display, m_window);

    // warp the mouse to the cursor center
    fakeMouseMove(m_xCenter, m_yCenter);
  }
}

void XWindowsScreen::disable()
{
  // release input context focus
  if (m_ic != NULL) {
    XUnsetICFocus(m_ic);
  }

  // unmap the hider/grab window.  this also ungrabs the mouse and
  // keyboard if they're grabbed.
  XUnmapWindow(m_display, m_window);

  // restore auto-repeat state
  if (!m_isPrimary && m_autoRepeat) {
    // XAutoRepeatOn(m_display);
  }
}

void XWindowsScreen::enter()
{
  screensaver(false);

  // release input context focus
  if (m_ic != NULL) {
    XUnsetICFocus(m_ic);
  }

  // set the input focus to what it had been when we took it
  if (m_lastFocus != None) {
    // the window may not exist anymore so ignore errors
    XWindowsUtil::ErrorLock lock(m_display);
    XSetInputFocus(m_display, m_lastFocus, m_lastFocusRevert, CurrentTime);
  }

#if HAVE_X11_EXTENSIONS_DPMS_H
  // Force the DPMS to turn screen back on since we don't
  // actually cause physical hardware input to trigger it
  int dummy;
  CARD16 powerlevel;
  BOOL enabled;
  if (DPMSQueryExtension(m_display, &dummy, &dummy) && DPMSCapable(m_display) &&
      DPMSInfo(m_display, &powerlevel, &enabled)) {
    if (enabled && powerlevel != DPMSModeOn)
      DPMSForceLevel(m_display, DPMSModeOn);
  }
#endif

  // unmap the hider/grab window.  this also ungrabs the mouse and
  // keyboard if they're grabbed.
  XUnmapWindow(m_display, m_window);

  /* maybe call this if entering for the screensaver
          // set keyboard focus to root window.  the screensaver should then
          // pick up key events for when the user enters a password to unlock.
          XSetInputFocus(m_display, PointerRoot, PointerRoot, CurrentTime);
  */

  if (!m_isPrimary) {
    // get the keyboard control state
    XKeyboardState keyControl;
    XGetKeyboardControl(m_display, &keyControl);
    m_autoRepeat = (keyControl.global_auto_repeat == AutoRepeatModeOn);
    m_keyState->setAutoRepeat(keyControl);

    // turn off auto-repeat.  we do this so fake key press events don't
    // cause the local server to generate their own auto-repeats of
    // those keys.
    // XAutoRepeatOff(m_display);
  }

  // now on screen
  m_isOnScreen = true;
}

bool XWindowsScreen::canLeave()
{
  // raise and show the window, required to grab mouse and keyboard
  XMapRaised(m_display, m_window);

  // see if grabbing the mouse and keyboard, if primary, is possible
  return !(m_isPrimary && !grabMouseAndKeyboard());
}

void XWindowsScreen::leave()
{
  if (!m_isPrimary) {
    // restore the previous keyboard auto-repeat state.  if the user
    // changed the auto-repeat configuration while on the client then
    // that state is lost.  that's because we can't get notified by
    // the X server when the auto-repeat configuration is changed so
    // we can't track the desired configuration.
    if (m_autoRepeat) {
      // XAutoRepeatOn(m_display);
    }

    // move hider window under the cursor center
    XMoveWindow(m_display, m_window, m_xCenter, m_yCenter);
  }

  // raise and show the window
  XMapRaised(m_display, m_window);

  // grab the mouse and keyboard, if primary and possible
  if (m_isPrimary && !grabMouseAndKeyboard()) {
    XUnmapWindow(m_display, m_window);
  }

  // save current focus
  XGetInputFocus(m_display, &m_lastFocus, &m_lastFocusRevert);

  // take focus
  if (m_isPrimary || !m_preserveFocus) {
    XSetInputFocus(m_display, m_window, RevertToPointerRoot, CurrentTime);
  }

  // now warp the mouse.  we warp after showing the window so we're
  // guaranteed to get the mouse leave event and to prevent the
  // keyboard focus from changing under point-to-focus policies.
  if (m_isPrimary) {
    warpCursor(m_xCenter, m_yCenter);
  } else {
    fakeMouseMove(m_xCenter, m_yCenter);
  }

  // set input context focus to our window
  if (m_ic != NULL) {
    XmbResetIC(m_ic);
    XSetICFocus(m_ic);
    m_filtered.clear();
  }

  // now off screen
  m_isOnScreen = false;
}

bool XWindowsScreen::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  // fail if we don't have the requested clipboard
  if (m_clipboard[id] == NULL) {
    return false;
  }

  // get the actual time.  ICCCM does not allow CurrentTime.
  Time timestamp = XWindowsUtil::getCurrentTime(m_display, m_clipboard[id]->getWindow());

  if (clipboard != NULL) {
    // save clipboard data
    return Clipboard::copy(m_clipboard[id], clipboard, timestamp);
  } else {
    // assert clipboard ownership
    if (!m_clipboard[id]->open(timestamp)) {
      return false;
    }
    m_clipboard[id]->empty();
    m_clipboard[id]->close();
    return true;
  }
}

void XWindowsScreen::checkClipboards()
{
  // do nothing, we're always up to date
}

void XWindowsScreen::openScreensaver(bool notify)
{
  m_screensaverNotify = notify;
  if (!m_screensaverNotify) {
    m_screensaver->disable();
  }
}

void XWindowsScreen::closeScreensaver()
{
  if (!m_screensaverNotify) {
    m_screensaver->enable();
  }
}

void XWindowsScreen::screensaver(bool activate)
{
  if (activate) {
    m_screensaver->activate();
  } else {
    m_screensaver->deactivate();
  }
}

void XWindowsScreen::resetOptions()
{
  m_xtestIsXineramaUnaware = true;
  m_preserveFocus = false;
}

void XWindowsScreen::setOptions(const OptionsList &options)
{
  for (uint32_t i = 0, n = options.size(); i < n; i += 2) {
    if (options[i] == kOptionXTestXineramaUnaware) {
      m_xtestIsXineramaUnaware = (options[i + 1] != 0);
      LOG((CLOG_DEBUG1 "library, XTest is Xinerama unaware %s", m_xtestIsXineramaUnaware ? "true" : "false"));
    } else if (options[i] == kOptionScreenPreserveFocus) {
      m_preserveFocus = (options[i + 1] != 0);
      LOG((CLOG_DEBUG1 "preserve focus: %s", m_preserveFocus ? "true" : "false"));
    }
  }
}

void XWindowsScreen::setSequenceNumber(uint32_t seqNum)
{
  m_sequenceNumber = seqNum;
}

bool XWindowsScreen::isPrimary() const
{
  return m_isPrimary;
}

std::string XWindowsScreen::getSecureInputApp() const
{
  // ignore on Linux
  return "";
}

void *XWindowsScreen::getEventTarget() const
{
  return const_cast<XWindowsScreen *>(this);
}

bool XWindowsScreen::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  assert(clipboard != NULL);

  // fail if we don't have the requested clipboard
  if (m_clipboard[id] == NULL) {
    return false;
  }

  // get the actual time.  ICCCM does not allow CurrentTime.
  Time timestamp = XWindowsUtil::getCurrentTime(m_display, m_clipboard[id]->getWindow());

  // copy the clipboard
  return Clipboard::copy(clipboard, m_clipboard[id], timestamp);
}

void XWindowsScreen::getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const
{
  x = m_x;
  y = m_y;
  w = m_w;
  h = m_h;
}

void XWindowsScreen::getCursorPos(int32_t &x, int32_t &y) const
{
  Window root, window;
  int mx, my, xWindow, yWindow;
  unsigned int mask;
  if (XQueryPointer(m_display, m_root, &root, &window, &mx, &my, &xWindow, &yWindow, &mask)) {
    x = mx;
    y = my;
  } else {
    x = m_xCenter;
    y = m_yCenter;
  }
}

void XWindowsScreen::reconfigure(uint32_t)
{
  // do nothing
}

void XWindowsScreen::warpCursor(int32_t x, int32_t y)
{
  // warp mouse
  warpCursorNoFlush(x, y);

  // remove all input events before and including warp
  XEvent event;
  while (XCheckMaskEvent(
      m_display,
      PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | KeymapStateMask, &event
  )) {
    // do nothing
  }

  // save position as last position
  m_xCursor = x;
  m_yCursor = y;
}

uint32_t XWindowsScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
  // only allow certain modifiers
  if ((mask & ~(KeyModifierShift | KeyModifierControl | KeyModifierAlt | KeyModifierSuper)) != 0) {
    LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
    return 0;
  }

  // fail if no keys
  if (key == kKeyNone && mask == 0) {
    return 0;
  }

  // convert to X
  unsigned int modifiers;
  if (!m_keyState->mapModifiersToX(mask, modifiers)) {
    // can't map all modifiers
    LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
    return 0;
  }
  XWindowsKeyState::KeycodeList keycodes;
  m_keyState->mapKeyToKeycodes(key, keycodes);
  if (key != kKeyNone && keycodes.empty()) {
    // can't map key
    LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
    return 0;
  }

  // choose hotkey id
  uint32_t id;
  if (!m_oldHotKeyIDs.empty()) {
    id = m_oldHotKeyIDs.back();
    m_oldHotKeyIDs.pop_back();
  } else {
    id = m_hotKeys.size() + 1;
  }
  HotKeyList &hotKeys = m_hotKeys[id];

  // all modifier hotkey must be treated specially.  for each modifier
  // we need to grab the modifier key in combination with all the other
  // requested modifiers.
  bool err = false;
  {
    XWindowsUtil::ErrorLock lock(m_display, &err);
    if (key == kKeyNone) {
      static const KeyModifierMask s_hotKeyModifiers[] = {
          KeyModifierShift, KeyModifierControl, KeyModifierAlt, KeyModifierMeta, KeyModifierSuper
      };

      XModifierKeymap *modKeymap = XGetModifierMapping(m_display);
      for (size_t j = 0; j < sizeof(s_hotKeyModifiers) / sizeof(s_hotKeyModifiers[0]) && !err; ++j) {
        // skip modifier if not in mask
        if ((mask & s_hotKeyModifiers[j]) == 0) {
          continue;
        }

        // skip with error if we can't map remaining modifiers
        unsigned int modifiers2;
        KeyModifierMask mask2 = (mask & ~s_hotKeyModifiers[j]);
        if (!m_keyState->mapModifiersToX(mask2, modifiers2)) {
          err = true;
          continue;
        }

        // compute modifier index for modifier.  there should be
        // exactly one X modifier missing
        int index;
        switch (modifiers ^ modifiers2) {
        case ShiftMask:
          index = ShiftMapIndex;
          break;

        case LockMask:
          index = LockMapIndex;
          break;

        case ControlMask:
          index = ControlMapIndex;
          break;

        case Mod1Mask:
          index = Mod1MapIndex;
          break;

        case Mod2Mask:
          index = Mod2MapIndex;
          break;

        case Mod3Mask:
          index = Mod3MapIndex;
          break;

        case Mod4Mask:
          index = Mod4MapIndex;
          break;

        case Mod5Mask:
          index = Mod5MapIndex;
          break;

        default:
          err = true;
          continue;
        }

        // grab each key for the modifier
        const KeyCode *modifiermap = modKeymap->modifiermap + index * modKeymap->max_keypermod;
        for (int k = 0; k < modKeymap->max_keypermod && !err; ++k) {
          KeyCode code = modifiermap[k];
          if (modifiermap[k] != 0) {
            XGrabKey(m_display, code, modifiers2, m_root, False, GrabModeAsync, GrabModeAsync);
            if (!err) {
              hotKeys.push_back(std::make_pair(code, modifiers2));
              m_hotKeyToIDMap[HotKeyItem(code, modifiers2)] = id;
            }
          }
        }
      }
      XFreeModifiermap(modKeymap);
    }

    // a non-modifier key must be insensitive to CapsLock, NumLock and
    // ScrollLock, so we have to grab the key with every combination of
    // those.
    else {
      // collect available toggle modifiers
      unsigned int modifier;
      unsigned int toggleModifiers[3];
      size_t numToggleModifiers = 0;
      if (m_keyState->mapModifiersToX(KeyModifierCapsLock, modifier)) {
        toggleModifiers[numToggleModifiers++] = modifier;
      }
      if (m_keyState->mapModifiersToX(KeyModifierNumLock, modifier)) {
        toggleModifiers[numToggleModifiers++] = modifier;
      }
      if (m_keyState->mapModifiersToX(KeyModifierScrollLock, modifier)) {
        toggleModifiers[numToggleModifiers++] = modifier;
      }

      for (XWindowsKeyState::KeycodeList::iterator j = keycodes.begin(); j != keycodes.end() && !err; ++j) {
        for (size_t i = 0; i < (1u << numToggleModifiers); ++i) {
          // add toggle modifiers for index i
          unsigned int tmpModifiers = modifiers;
          if ((i & 1) != 0) {
            tmpModifiers |= toggleModifiers[0];
          }
          if ((i & 2) != 0) {
            tmpModifiers |= toggleModifiers[1];
          }
          if ((i & 4) != 0) {
            tmpModifiers |= toggleModifiers[2];
          }

          // add grab
          XGrabKey(m_display, *j, tmpModifiers, m_root, False, GrabModeAsync, GrabModeAsync);
          if (!err) {
            hotKeys.push_back(std::make_pair(*j, tmpModifiers));
            m_hotKeyToIDMap[HotKeyItem(*j, tmpModifiers)] = id;
          }
        }
      }
    }
  }

  if (err) {
    // if any failed then unregister any we did get
    for (HotKeyList::iterator j = hotKeys.begin(); j != hotKeys.end(); ++j) {
      XUngrabKey(m_display, j->first, j->second, m_root);
      m_hotKeyToIDMap.erase(HotKeyItem(j->first, j->second));
    }

    m_oldHotKeyIDs.push_back(id);
    m_hotKeys.erase(id);
    LOG(
        (CLOG_WARN "failed to register hotkey %s (id=%04x mask=%04x)", deskflow::KeyMap::formatKey(key, mask).c_str(),
         key, mask)
    );
    return 0;
  }

  LOG(
      (CLOG_DEBUG "registered hotkey %s (id=%04x mask=%04x) as id=%d", deskflow::KeyMap::formatKey(key, mask).c_str(),
       key, mask, id)
  );
  return id;
}

void XWindowsScreen::unregisterHotKey(uint32_t id)
{
  // look up hotkey
  HotKeyMap::iterator i = m_hotKeys.find(id);
  if (i == m_hotKeys.end()) {
    return;
  }

  // unregister with OS
  bool err = false;
  {
    XWindowsUtil::ErrorLock lock(m_display, &err);
    HotKeyList &hotKeys = i->second;
    for (HotKeyList::iterator j = hotKeys.begin(); j != hotKeys.end(); ++j) {
      XUngrabKey(m_display, j->first, j->second, m_root);
      m_hotKeyToIDMap.erase(HotKeyItem(j->first, j->second));
    }
  }
  if (err) {
    LOG((CLOG_WARN "failed to unregister hotkey id=%d", id));
  } else {
    LOG((CLOG_DEBUG "unregistered hotkey id=%d", id));
  }

  // discard hot key from map and record old id for reuse
  m_hotKeys.erase(i);
  m_oldHotKeyIDs.push_back(id);
}

void XWindowsScreen::fakeInputBegin()
{
  // FIXME -- not implemented
}

void XWindowsScreen::fakeInputEnd()
{
  // FIXME -- not implemented
}

int32_t XWindowsScreen::getJumpZoneSize() const
{
  return 1;
}

bool XWindowsScreen::isAnyMouseButtonDown(uint32_t &buttonID) const
{
  // query the pointer to get the button state
  Window root, window;
  int xRoot, yRoot, xWindow, yWindow;
  unsigned int state;
  if (XQueryPointer(m_display, m_root, &root, &window, &xRoot, &yRoot, &xWindow, &yWindow, &state)) {
    return ((state & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) != 0);
  }

  return false;
}

void XWindowsScreen::getCursorCenter(int32_t &x, int32_t &y) const
{
  x = m_xCenter;
  y = m_yCenter;
}

void XWindowsScreen::fakeMouseButton(ButtonID button, bool press)
{
  const unsigned int xButton = mapButtonToX(button);
  if (xButton > 0 && xButton < 11) {
    XTestFakeButtonEvent(m_display, xButton, press ? True : False, CurrentTime);
    XFlush(m_display);
  }
}

void XWindowsScreen::fakeMouseMove(int32_t x, int32_t y)
{
  if (m_xinerama && m_xtestIsXineramaUnaware) {
    XWarpPointer(m_display, None, m_root, 0, 0, 0, 0, x, y);
  } else {
    XTestFakeMotionEvent(m_display, DefaultScreen(m_display), x, y, CurrentTime);
  }
  XFlush(m_display);
}

void XWindowsScreen::fakeMouseRelativeMove(int32_t dx, int32_t dy) const
{
  // FIXME -- ignore xinerama for now
  XTestFakeRelativeMotionEvent(m_display, dx, dy, CurrentTime);
  XFlush(m_display);
}

void XWindowsScreen::fakeMouseWheel(int32_t, int32_t yDelta) const
{
  // XXX -- support x-axis scrolling
  if (yDelta == 0) {
    return;
  }

  yDelta = mapClientScrollDirection(yDelta);

  // choose button depending on rotation direction
  const unsigned int xButton = mapButtonToX(static_cast<ButtonID>((yDelta >= 0) ? -1 : -2));
  if (xButton == 0) {
    // If we get here, then the XServer does not support the scroll
    // wheel buttons, so send PageUp/PageDown keystrokes instead.
    // Patch by Tom Chadwick.
    KeyCode keycode = 0;
    if (yDelta >= 0) {
      keycode = XKeysymToKeycode(m_display, XK_Page_Up);
    } else {
      keycode = XKeysymToKeycode(m_display, XK_Page_Down);
    }
    if (keycode != 0) {
      XTestFakeKeyEvent(m_display, keycode, True, CurrentTime);
      XTestFakeKeyEvent(m_display, keycode, False, CurrentTime);
    }
    return;
  }

  // now use absolute value of delta
  if (yDelta < 0) {
    yDelta = -yDelta;
  }

  if (yDelta < m_mouseScrollDelta) {
    LOG((CLOG_WARN "wheel scroll delta (%d) smaller than threshold (%d)", yDelta, m_mouseScrollDelta));
  }

  // send as many clicks as necessary
  for (; yDelta >= m_mouseScrollDelta; yDelta -= m_mouseScrollDelta) {
    XTestFakeButtonEvent(m_display, xButton, True, CurrentTime);
    XTestFakeButtonEvent(m_display, xButton, False, CurrentTime);
  }
  XFlush(m_display);
}

Display *XWindowsScreen::openDisplay(const char *displayName)
{
  // get the DISPLAY
  if (displayName == NULL) {
    displayName = getenv("DISPLAY");
    if (displayName == NULL) {
      displayName = ":0.0";
    }
  }

  // open the display
  LOG((CLOG_DEBUG3 "calling XOpenDisplay(\"%s\")", displayName));
  Display *display = XOpenDisplay(displayName);
  if (display == NULL) {
    throw XScreenUnavailable(60.0);
  }

  // verify the availability of the XTest extension
  if (!m_isPrimary) {
    int majorOpcode, firstEvent, firstError;
    if (!XQueryExtension(display, XTestExtensionName, &majorOpcode, &firstEvent, &firstError)) {
      LOG((CLOG_ERR "the XTest extension is not available"));
      XCloseDisplay(display);
      throw XScreenOpenFailure();
    }
  }

#if HAVE_XKB_EXTENSION
  {
    m_xkb = false;
    int major = XkbMajorVersion, minor = XkbMinorVersion;
    if (XkbLibraryVersion(&major, &minor)) {
      int opcode, firstError;
      if (XkbQueryExtension(display, &opcode, &m_xkbEventBase, &firstError, &major, &minor)) {
        m_xkb = true;
        XkbSelectEvents(display, XkbUseCoreKbd, XkbMapNotifyMask, XkbMapNotifyMask);
        XkbSelectEventDetails(display, XkbUseCoreKbd, XkbStateNotifyMask, XkbGroupStateMask, XkbGroupStateMask);
      }
    }
  }
#endif

#if HAVE_X11_EXTENSIONS_XRANDR_H
  // query for XRandR extension
  int dummyError;
  m_xrandr = XRRQueryExtension(display, &m_xrandrEventBase, &dummyError);
  if (m_xrandr) {
    // enable XRRScreenChangeNotifyEvent
    XRRSelectInput(display, DefaultRootWindow(display), RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask);
  }
#endif

  return display;
}

void XWindowsScreen::saveShape()
{
  // get shape of default screen
  m_x = 0;
  m_y = 0;

  m_w = WidthOfScreen(DefaultScreenOfDisplay(m_display));
  m_h = HeightOfScreen(DefaultScreenOfDisplay(m_display));

  // get center of default screen
  m_xCenter = m_x + (m_w >> 1);
  m_yCenter = m_y + (m_h >> 1);

  // check if xinerama is enabled and there is more than one screen.
  // get center of first Xinerama screen.  Xinerama appears to have
  // a bug when XWarpPointer() is used in combination with
  // XGrabPointer().  in that case, the warp is successful but the
  // next pointer motion warps the pointer again, apparently to
  // constrain it to some unknown region, possibly the region from
  // 0,0 to Wm,Hm where Wm (Hm) is the minimum width (height) over
  // all physical screens.  this warp only seems to happen if the
  // pointer wasn't in that region before the XWarpPointer().  the
  // second (unexpected) warp causes deskflow to think the pointer
  // has been moved when it hasn't.  to work around the problem,
  // we warp the pointer to the center of the first physical
  // screen instead of the logical screen.
  m_xinerama = false;
#if HAVE_X11_EXTENSIONS_XINERAMA_H
  int eventBase, errorBase;
  if (XineramaQueryExtension(m_display, &eventBase, &errorBase) && XineramaIsActive(m_display)) {
    int numScreens;
    XineramaScreenInfo *screens;
    screens = XineramaQueryScreens(m_display, &numScreens);
    if (screens != NULL) {
      if (numScreens > 1) {
        m_xinerama = true;
        m_xCenter = screens[0].x_org + (screens[0].width >> 1);
        m_yCenter = screens[0].y_org + (screens[0].height >> 1);
      }
      XFree(screens);
    }
  }
#endif
}

void XWindowsScreen::setShape(int32_t width, int32_t height)
{
  // set shape
  m_x = 0;
  m_y = 0;

  m_w = width;
  m_h = height;

  // get center of default screen
  m_xCenter = m_x + (m_w >> 1);
  m_yCenter = m_y + (m_h >> 1);

  // check if xinerama is enabled and there is more than one screen.
  // get center of first Xinerama screen.  Xinerama appears to have
  // a bug when XWarpPointer() is used in combination with
  // XGrabPointer().  in that case, the warp is successful but the
  // next pointer motion warps the pointer again, apparently to
  // constrain it to some unknown region, possibly the region from
  // 0,0 to Wm,Hm where Wm (Hm) is the minimum width (height) over
  // all physical screens.  this warp only seems to happen if the
  // pointer wasn't in that region before the XWarpPointer().  the
  // second (unexpected) warp causes deskflow to think the pointer
  // has been moved when it hasn't.  to work around the problem,
  // we warp the pointer to the center of the first physical
  // screen instead of the logical screen.
  m_xinerama = false;
#if HAVE_X11_EXTENSIONS_XINERAMA_H
  int eventBase, errorBase;
  if ((XineramaQueryExtension(m_display, &eventBase, &errorBase) != 0) && (XineramaIsActive(m_display) != 0)) {
    int numScreens;
    XineramaScreenInfo *screens;
    screens = XineramaQueryScreens(m_display, &numScreens);
    if (screens != nullptr) {
      if (numScreens > 1) {
        m_xinerama = true;
        m_xCenter = screens[0].x_org + (screens[0].width >> 1);
        m_yCenter = screens[0].y_org + (screens[0].height >> 1);
      }
      XFree(screens);
    }
  }
#endif
}

Window XWindowsScreen::openWindow() const
{
  // default window attributes.  we don't want the window manager
  // messing with our window and we don't want the cursor to be
  // visible inside the window.
  XSetWindowAttributes attr;
  attr.do_not_propagate_mask = 0;
  attr.override_redirect = True;
  attr.cursor = createBlankCursor();

  // adjust attributes and get size and shape
  int32_t x, y, w, h;
  if (m_isPrimary) {
    // grab window attributes.  this window is used to capture user
    // input when the user is focused on another client.  it covers
    // the whole screen.
    attr.event_mask = PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask |
                      KeymapStateMask | PropertyChangeMask;
    x = m_x;
    y = m_y;
    w = m_w;
    h = m_h;
  } else {
    // cursor hider window attributes.  this window is used to hide the
    // cursor when it's not on the screen.  the window is hidden as soon
    // as the cursor enters the screen or the display's real mouse is
    // moved.  we'll reposition the window as necessary so its
    // position here doesn't matter.  it only needs to be 1x1 because
    // it only needs to contain the cursor's hotspot.
    attr.event_mask = LeaveWindowMask;
    x = 0;
    y = 0;
    w = 1;
    h = 1;
  }

  // create and return the window
  Window window = XCreateWindow(
      m_display, m_root, x, y, w, h, 0, 0, InputOnly, CopyFromParent,
      CWDontPropagate | CWEventMask | CWOverrideRedirect | CWCursor, &attr
  );
  if (window == None) {
    throw XScreenOpenFailure();
  }
  return window;
}

void XWindowsScreen::openIM()
{
  // open the input methods
  XIM im = XOpenIM(m_display, NULL, NULL, NULL);
  if (im == NULL) {
    LOG((CLOG_INFO "no support for IM"));
    return;
  }

  // find the appropriate style.  deskflow supports XIMPreeditNothing
  // only at the moment.
  XIMStyles *styles;
  if (XGetIMValues(im, XNQueryInputStyle, &styles, NULL) != NULL || styles == NULL) {
    LOG((CLOG_WARN "cannot get IM styles"));
    XCloseIM(im);
    return;
  }
  XIMStyle style = 0;
  for (unsigned short i = 0; i < styles->count_styles; ++i) {
    style = styles->supported_styles[i];
    if ((style & XIMPreeditNothing) != 0) {
      if ((style & (XIMStatusNothing | XIMStatusNone)) != 0) {
        break;
      }
    }
  }
  XFree(styles);
  if (style == 0) {
    LOG((CLOG_INFO "no supported IM styles"));
    XCloseIM(im);
    return;
  }

  // create an input context for the style and tell it about our window
  XIC ic = XCreateIC(im, XNInputStyle, style, XNClientWindow, m_window, NULL);
  if (ic == NULL) {
    LOG((CLOG_WARN "cannot create IC"));
    XCloseIM(im);
    return;
  }

  // find out the events we must select for and do so
  unsigned long mask;
  if (XGetICValues(ic, XNFilterEvents, &mask, NULL) != NULL) {
    LOG((CLOG_WARN "cannot get IC filter events"));
    XDestroyIC(ic);
    XCloseIM(im);
    return;
  }

  // we have IM
  m_im = im;
  m_ic = ic;
  m_lastKeycode = 0;

  // select events on our window that IM requires
  XWindowAttributes attr;
  XGetWindowAttributes(m_display, m_window, &attr);
  XSelectInput(m_display, m_window, attr.your_event_mask | mask);

  // listen for screen-resize messages
  XSelectInput(m_display, m_root, StructureNotifyMask);
}

void XWindowsScreen::sendEvent(Event::Type type, void *data)
{
  m_events->addEvent(Event(type, getEventTarget(), data));
}

void XWindowsScreen::sendClipboardEvent(Event::Type type, ClipboardID id)
{
  ClipboardInfo *info = (ClipboardInfo *)malloc(sizeof(ClipboardInfo));
  info->m_id = id;
  info->m_sequenceNumber = m_sequenceNumber;
  sendEvent(type, info);
}

IKeyState *XWindowsScreen::getKeyState() const
{
  return m_keyState;
}

Bool XWindowsScreen::findKeyEvent(Display *, XEvent *xevent, XPointer arg)
{
  KeyEventFilter *filter = reinterpret_cast<KeyEventFilter *>(arg);
  return (xevent->type == filter->m_event && xevent->xkey.window == filter->m_window &&
          xevent->xkey.time == filter->m_time && xevent->xkey.keycode == filter->m_keycode)
             ? True
             : False;
}

void XWindowsScreen::handleSystemEvent(const Event &event, void *)
{
  XEvent *xevent = static_cast<XEvent *>(event.getData());
  assert(xevent != NULL);

  // update key state
  bool isRepeat = false;
  if (m_isPrimary) {
    if (xevent->type == KeyRelease) {
      // check if this is a key repeat by getting the next
      // KeyPress event that has the same key and time as
      // this release event, if any.  first prepare the
      // filter info.
      KeyEventFilter filter;
      filter.m_event = KeyPress;
      filter.m_window = xevent->xkey.window;
      filter.m_time = xevent->xkey.time;
      filter.m_keycode = xevent->xkey.keycode;
      XEvent xevent2;
      isRepeat = (XCheckIfEvent(m_display, &xevent2, &XWindowsScreen::findKeyEvent, (XPointer)&filter) == True);
    }

    if (xevent->type == KeyPress || xevent->type == KeyRelease) {
      if (xevent->xkey.window == m_root) {
        // this is a hot key
        onHotKey(xevent->xkey, isRepeat);
        return;
      } else if (!m_isOnScreen) {
        // this might be a hot key
        if (onHotKey(xevent->xkey, isRepeat)) {
          return;
        }
      }

      bool down = (isRepeat || xevent->type == KeyPress);
      KeyModifierMask state = m_keyState->mapModifiersFromX(xevent->xkey.state);
      m_keyState->onKey(xevent->xkey.keycode, down, state);
    }
  }

  // let input methods try to handle event first
  if (m_ic != NULL) {
    // XFilterEvent() may eat the event and generate a new KeyPress
    // event with a keycode of 0 because there isn't an actual key
    // associated with the keysym.  but the KeyRelease may pass
    // through XFilterEvent() and keep its keycode.  this means
    // there's a mismatch between KeyPress and KeyRelease keycodes.
    // since we use the keycode on the client to detect when a key
    // is released this won't do.  so we remember the keycode on
    // the most recent KeyPress (and clear it on a matching
    // KeyRelease) so we have a keycode for a synthesized KeyPress.
    if (xevent->type == KeyPress && xevent->xkey.keycode != 0) {
      m_lastKeycode = xevent->xkey.keycode;
    } else if (xevent->type == KeyRelease && xevent->xkey.keycode == m_lastKeycode) {
      m_lastKeycode = 0;
    }

    // now filter the event
    if (XFilterEvent(xevent, DefaultRootWindow(m_display))) {
      if (xevent->type == KeyPress) {
        // add filtered presses to the filtered list
        m_filtered.insert(m_lastKeycode);
      }
      return;
    }

    // discard matching key releases for key presses that were
    // filtered and remove them from our filtered list.
    else if (xevent->type == KeyRelease && m_filtered.count(xevent->xkey.keycode) > 0) {
      m_filtered.erase(xevent->xkey.keycode);
      return;
    }
  }

  // let screen saver have a go
  if (m_screensaver->handleXEvent(xevent)) {
    // screen saver handled it
    return;
  }

#ifdef HAVE_XI2
  if (m_xi2detected) {
    // Process RawMotion
    XGenericEventCookie *cookie = (XGenericEventCookie *)&xevent->xcookie;
    if (XGetEventData(m_display, cookie) && cookie->type == GenericEvent && cookie->extension == xi_opcode) {
      if (cookie->evtype == XI_RawMotion) {
        // Get current pointer's position
        Window root, child;
        XMotionEvent xmotion;
        xmotion.type = MotionNotify;
        xmotion.send_event = False; // Raw motion
        xmotion.display = m_display;
        xmotion.window = m_window;
        /* xmotion's time, state and is_hint are not used */
        unsigned int msk;
        xmotion.same_screen = XQueryPointer(
            m_display, m_root, &xmotion.root, &xmotion.subwindow, &xmotion.x_root, &xmotion.y_root, &xmotion.x,
            &xmotion.y, &msk
        );
        onMouseMove(xmotion);
        XFreeEventData(m_display, cookie);
        return;
      }
      XFreeEventData(m_display, cookie);
    }
  }
#endif

  // handle the event ourself
  switch (xevent->type) {
  case CreateNotify:
    if (m_isPrimary && !m_xi2detected) {
      // select events on new window
      selectEvents(xevent->xcreatewindow.window);
    }
    break;

  case MappingNotify:
    refreshKeyboard(xevent);
    break;

  case LeaveNotify:
    if (!m_isPrimary) {
      // mouse moved out of hider window somehow.  hide the window.
      XUnmapWindow(m_display, m_window);
    }
    break;

  case SelectionClear: {
    // we just lost the selection.  that means someone else
    // grabbed the selection so this screen is now the
    // selection owner.  report that to the receiver.
    ClipboardID id = getClipboardID(xevent->xselectionclear.selection);
    if (id != kClipboardEnd) {
      m_clipboard[id]->lost(xevent->xselectionclear.time);
      sendClipboardEvent(m_events->forClipboard().clipboardGrabbed(), id);
      return;
    }
  } break;

  case SelectionNotify:
    // notification of selection transferred.  we shouldn't
    // get this here because we handle them in the selection
    // retrieval methods.  we'll just delete the property
    // with the data (satisfying the usual ICCCM protocol).
    if (xevent->xselection.property != None) {
      XDeleteProperty(m_display, xevent->xselection.requestor, xevent->xselection.property);
    }
    break;

  case SelectionRequest: {
    // somebody is asking for clipboard data
    ClipboardID id = getClipboardID(xevent->xselectionrequest.selection);
    if (id != kClipboardEnd) {
      m_clipboard[id]->addRequest(
          xevent->xselectionrequest.owner, xevent->xselectionrequest.requestor, xevent->xselectionrequest.target,
          xevent->xselectionrequest.time, xevent->xselectionrequest.property
      );
      return;
    }
  } break;

  case PropertyNotify:
    // property delete may be part of a selection conversion
    if (xevent->xproperty.state == PropertyDelete) {
      processClipboardRequest(xevent->xproperty.window, xevent->xproperty.time, xevent->xproperty.atom);
    }
    break;

  case DestroyNotify:
    // looks like one of the windows that requested a clipboard
    // transfer has gone bye-bye.
    destroyClipboardRequest(xevent->xdestroywindow.window);
    break;

  case KeyPress:
    if (m_isPrimary) {
      onKeyPress(xevent->xkey);
    }
    return;

  case KeyRelease:
    if (m_isPrimary) {
      onKeyRelease(xevent->xkey, isRepeat);
    }
    return;

  case ButtonPress:
    if (m_isPrimary) {
      onMousePress(xevent->xbutton);
    }
    return;

  case ButtonRelease:
    if (m_isPrimary) {
      onMouseRelease(xevent->xbutton);
    }
    return;

  case MotionNotify:
    if (m_isPrimary) {
      onMouseMove(xevent->xmotion);
    }
    return;

  case ConfigureNotify:
    if (!m_isPrimary && xevent->xconfigure.window == m_root) {
      setShape(xevent->xconfigure.width, xevent->xconfigure.height);
      sendEvent(m_events->forIScreen().shapeChanged());
    }
    return;

  default:
#if HAVE_XKB_EXTENSION
    if (m_xkb && xevent->type == m_xkbEventBase) {
      XkbEvent *xkbEvent = reinterpret_cast<XkbEvent *>(xevent);
      switch (xkbEvent->any.xkb_type) {
      case XkbMapNotify:
        refreshKeyboard(xevent);
        return;

      case XkbStateNotify:
        LOG((CLOG_INFO "group change: %d", xkbEvent->state.group));
        m_keyState->setActiveGroup((int32_t)xkbEvent->state.group);
        return;
      }
    }
#endif

#if HAVE_X11_EXTENSIONS_XRANDR_H
    if (m_xrandr) {
      if (xevent->type == m_xrandrEventBase + RRScreenChangeNotify ||
          xevent->type == m_xrandrEventBase + RRNotify &&
              reinterpret_cast<XRRNotifyEvent *>(xevent)->subtype == RRNotify_CrtcChange) {
        LOG((CLOG_INFO "either XRRScreenChangeNotifyEvent or "
                       "RRNotify_CrtcChange received"));

        // we're required to call back into XLib so XLib can update its internal
        // state
        XRRUpdateConfiguration(xevent);

        // requery/recalculate the screen shape
        saveShape();

        // we need to resize m_window, otherwise we'll get a weird problem where
        // moving off the server onto the client causes the pointer to warp to
        // the center of the server (so you can't move the pointer off the
        // server)
        if (m_isPrimary) {
          XMoveWindow(m_display, m_window, m_x, m_y);
          XResizeWindow(m_display, m_window, m_w, m_h);
        }

        sendEvent(m_events->forIScreen().shapeChanged());
      }
    }
#endif

    break;
  }
}

void XWindowsScreen::onKeyPress(XKeyEvent &xkey)
{
  LOG((CLOG_DEBUG1 "event: KeyPress code=%d, state=0x%04x", xkey.keycode, xkey.state));
  const KeyModifierMask mask = m_keyState->mapModifiersFromX(xkey.state);
  KeyID key = mapKeyFromX(&xkey);
  if (key != kKeyNone) {
    // check for ctrl+alt+del emulation
    if ((key == kKeyPause || key == kKeyBreak) &&
        (mask & (KeyModifierControl | KeyModifierAlt)) == (KeyModifierControl | KeyModifierAlt)) {
      // pretend it's ctrl+alt+del
      LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
      key = kKeyDelete;
    }

    // get which button.  see call to XFilterEvent() in onEvent()
    // for more info.
    bool isFake = false;
    KeyButton keycode = static_cast<KeyButton>(xkey.keycode);
    if (keycode == 0) {
      isFake = true;
      keycode = static_cast<KeyButton>(m_lastKeycode);
      if (keycode == 0) {
        // no keycode
        LOG((CLOG_DEBUG1 "event: KeyPress no keycode"));
        return;
      }
    }

    // handle key
    m_keyState->sendKeyEvent(getEventTarget(), true, false, key, mask, 1, keycode);

    // do fake release if this is a fake press
    if (isFake) {
      m_keyState->sendKeyEvent(getEventTarget(), false, false, key, mask, 1, keycode);
    }
  } else {
    LOG((CLOG_DEBUG1 "can't map keycode to key id"));
  }
}

void XWindowsScreen::onKeyRelease(XKeyEvent &xkey, bool isRepeat)
{
  const KeyModifierMask mask = m_keyState->mapModifiersFromX(xkey.state);
  KeyID key = mapKeyFromX(&xkey);
  if (key != kKeyNone) {
    // check for ctrl+alt+del emulation
    if ((key == kKeyPause || key == kKeyBreak) &&
        (mask & (KeyModifierControl | KeyModifierAlt)) == (KeyModifierControl | KeyModifierAlt)) {
      // pretend it's ctrl+alt+del and ignore autorepeat
      LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
      key = kKeyDelete;
      isRepeat = false;
    }

    KeyButton keycode = static_cast<KeyButton>(xkey.keycode);
    if (!isRepeat) {
      // no press event follows so it's a plain release
      LOG((CLOG_DEBUG1 "event: KeyRelease code=%d, state=0x%04x", keycode, xkey.state));
      m_keyState->sendKeyEvent(getEventTarget(), false, false, key, mask, 1, keycode);
    } else {
      // found a press event following so it's a repeat.
      // we could attempt to count the already queued
      // repeats but we'll just send a repeat of 1.
      // note that we discard the press event.
      LOG((CLOG_DEBUG1 "event: repeat code=%d, state=0x%04x", keycode, xkey.state));
      m_keyState->sendKeyEvent(getEventTarget(), false, true, key, mask, 1, keycode);
    }
  }
}

bool XWindowsScreen::onHotKey(XKeyEvent &xkey, bool isRepeat)
{
  // find the hot key id
  HotKeyToIDMap::const_iterator i = m_hotKeyToIDMap.find(HotKeyItem(xkey.keycode, xkey.state));
  if (i == m_hotKeyToIDMap.end()) {
    return false;
  }

  // find what kind of event
  Event::Type type;
  if (xkey.type == KeyPress) {
    type = m_events->forIPrimaryScreen().hotKeyDown();
  } else if (xkey.type == KeyRelease) {
    type = m_events->forIPrimaryScreen().hotKeyUp();
  } else {
    return false;
  }

  // generate event (ignore key repeats)
  if (!isRepeat) {
    m_events->addEvent(Event(type, getEventTarget(), HotKeyInfo::alloc(i->second)));
  }
  return true;
}

void XWindowsScreen::onMousePress(const XButtonEvent &xbutton)
{
  LOG((CLOG_DEBUG1 "event: ButtonPress button=%d", xbutton.button));
  ButtonID button = mapButtonFromX(&xbutton);
  KeyModifierMask mask = m_keyState->mapModifiersFromX(xbutton.state);
  if (button != kButtonNone) {
    sendEvent(m_events->forIPrimaryScreen().buttonDown(), ButtonInfo::alloc(button, mask));
  }
}

void XWindowsScreen::onMouseRelease(const XButtonEvent &xbutton)
{
  LOG((CLOG_DEBUG1 "event: ButtonRelease button=%d", xbutton.button));
  ButtonID button = mapButtonFromX(&xbutton);
  KeyModifierMask mask = m_keyState->mapModifiersFromX(xbutton.state);
  if (button != kButtonNone) {
    sendEvent(m_events->forIPrimaryScreen().buttonUp(), ButtonInfo::alloc(button, mask));
  } else if (xbutton.button == 4) {
    // wheel forward (away from user)
    sendEvent(m_events->forIPrimaryScreen().wheel(), WheelInfo::alloc(0, 120));
  } else if (xbutton.button == 5) {
    // wheel backward (toward user)
    sendEvent(m_events->forIPrimaryScreen().wheel(), WheelInfo::alloc(0, -120));
  }
  // XXX -- support x-axis scrolling
}

void XWindowsScreen::onMouseMove(const XMotionEvent &xmotion)
{
  LOG((CLOG_DEBUG2 "event: MotionNotify %d,%d", xmotion.x_root, xmotion.y_root));

  // compute motion delta (relative to the last known
  // mouse position)
  int32_t x = xmotion.x_root - m_xCursor;
  int32_t y = xmotion.y_root - m_yCursor;

  // save position to compute delta of next motion
  m_xCursor = xmotion.x_root;
  m_yCursor = xmotion.y_root;

  if (xmotion.send_event) {
    // we warped the mouse.  discard events until we
    // find the matching sent event.  see
    // warpCursorNoFlush() for where the events are
    // sent.  we discard the matching sent event and
    // can be sure we've skipped the warp event.
    XEvent xevent;
    char cntr = 0;
    do {
      XMaskEvent(m_display, PointerMotionMask, &xevent);
      if (cntr++ > 10) {
        LOG((CLOG_WARN "too many discarded events! %d", cntr));
        break;
      }
    } while (!xevent.xany.send_event);
    cntr = 0;
  } else if (m_isOnScreen) {
    // motion on primary screen
    sendEvent(m_events->forIPrimaryScreen().motionOnPrimary(), MotionInfo::alloc(m_xCursor, m_yCursor));
  } else {
    // motion on secondary screen.  warp mouse back to
    // center.
    //
    // my lombard (powerbook g3) running linux and
    // using the adbmouse driver has two problems:
    // first, the driver only sends motions of +/-2
    // pixels and, second, it seems to discard some
    // physical input after a warp.  the former isn't a
    // big deal (we're just limited to every other
    // pixel) but the latter is a PITA.  to work around
    // it we only warp when the mouse has moved more
    // than s_size pixels from the center.
    static const int32_t s_size = 32;
    if (xmotion.x_root - m_xCenter < -s_size || xmotion.x_root - m_xCenter > s_size ||
        xmotion.y_root - m_yCenter < -s_size || xmotion.y_root - m_yCenter > s_size) {
      warpCursorNoFlush(m_xCenter, m_yCenter);
    }

    // send event if mouse moved.  do this after warping
    // back to center in case the motion takes us onto
    // the primary screen.  if we sent the event first
    // in that case then the warp would happen after
    // warping to the primary screen's enter position,
    // effectively overriding it.
    if (x != 0 || y != 0) {
      sendEvent(m_events->forIPrimaryScreen().motionOnSecondary(), MotionInfo::alloc(x, y));
    }
  }
}

Cursor XWindowsScreen::createBlankCursor() const
{
  // this seems just a bit more complicated than really necessary

  // get the closet cursor size to 1x1
  unsigned int w = 0, h = 0;
  XQueryBestCursor(m_display, m_root, 1, 1, &w, &h);
  w = std::max(1u, w);
  h = std::max(1u, h);

  // make bitmap data for cursor of closet size.  since the cursor
  // is blank we can use the same bitmap for shape and mask:  all
  // zeros.
  const int size = ((w + 7) >> 3) * h;
  char *data = new char[size];
  memset(data, 0, size);

  // make bitmap
  Pixmap bitmap = XCreateBitmapFromData(m_display, m_root, data, w, h);

  // need an arbitrary color for the cursor
  XColor color;
  color.pixel = 0;
  color.red = color.green = color.blue = 0;
  color.flags = DoRed | DoGreen | DoBlue;

  // make cursor from bitmap
  Cursor cursor = XCreatePixmapCursor(m_display, bitmap, bitmap, &color, &color, 0, 0);

  // don't need bitmap or the data anymore
  delete[] data;
  XFreePixmap(m_display, bitmap);

  return cursor;
}

ClipboardID XWindowsScreen::getClipboardID(Atom selection) const
{
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    if (m_clipboard[id] != NULL && m_clipboard[id]->getSelection() == selection) {
      return id;
    }
  }
  return kClipboardEnd;
}

void XWindowsScreen::processClipboardRequest(Window requestor, Time time, Atom property)
{
  // check every clipboard until one returns success
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    if (m_clipboard[id] != NULL && m_clipboard[id]->processRequest(requestor, time, property)) {
      break;
    }
  }
}

void XWindowsScreen::destroyClipboardRequest(Window requestor)
{
  // check every clipboard until one returns success
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    if (m_clipboard[id] != NULL && m_clipboard[id]->destroyRequest(requestor)) {
      break;
    }
  }
}

void XWindowsScreen::onError()
{
  // prevent further access to the X display
  m_events->adoptBuffer(NULL);
  m_screensaver->destroy();
  m_screensaver = NULL;
  m_display = NULL;

  // notify of failure
  sendEvent(m_events->forIScreen().error(), NULL);

  // FIXME -- should ensure that we ignore operations that involve
  // m_display from now on.  however, Xlib will simply exit the
  // application in response to the X I/O error so there's no
  // point in trying to really handle the error.  if we did want
  // to handle the error, it'd probably be easiest to delegate to
  // one of two objects.  one object would take the implementation
  // from this class.  the other object would be stub methods that
  // don't use X11.  on error, we'd switch to the latter.
}

int XWindowsScreen::ioErrorHandler(Display *)
{
  // the display has disconnected, probably because X is shutting
  // down.  X forces us to exit at this point which is annoying.
  // we'll pretend as if we won't exit so we try to make sure we
  // don't access the display anymore.
  LOG((CLOG_CRIT "x display has unexpectedly disconnected"));
  s_screen->onError();
  return 0;
}

void XWindowsScreen::selectEvents(Window w) const
{
  // ignore errors while we adjust event masks.  windows could be
  // destroyed at any time after the XQueryTree() in doSelectEvents()
  // so we must ignore BadWindow errors.
  XWindowsUtil::ErrorLock lock(m_display);

  // adjust event masks
  doSelectEvents(w);
}

void XWindowsScreen::doSelectEvents(Window w) const
{
  // we want to track the mouse everywhere on the display.  to achieve
  // that we select PointerMotionMask on every window.  we also select
  // SubstructureNotifyMask in order to get CreateNotify events so we
  // select events on new windows too.

  // we don't want to adjust our grab window
  if (w == m_window) {
    return;
  }

  // X11 has a design flaw. If *no* client selected PointerMotionMask for
  // a window, motion events will be delivered to that window's parent.
  // If *any* client, not necessarily the owner, selects PointerMotionMask
  // on such a window, X will stop propagating motion events to its
  // parent. This breaks applications that rely on event propagation
  // behavior.
  //
  // Avoid selecting PointerMotionMask unless some other client selected
  // it already.
  long mask = SubstructureNotifyMask;
  XWindowAttributes attr;
  XGetWindowAttributes(m_display, w, &attr);
  if ((attr.all_event_masks & PointerMotionMask) == PointerMotionMask) {
    mask |= PointerMotionMask;
  }

  // select events of interest.  do this before querying the tree so
  // we'll get notifications of children created after the XQueryTree()
  // so we won't miss them.
  XSelectInput(m_display, w, mask);

  // recurse on child windows
  Window rw, pw, *cw;
  unsigned int nc;
  if (XQueryTree(m_display, w, &rw, &pw, &cw, &nc)) {
    for (unsigned int i = 0; i < nc; ++i) {
      doSelectEvents(cw[i]);
    }
    XFree(cw);
  }
}

KeyID XWindowsScreen::mapKeyFromX(XKeyEvent *event) const
{
  // convert to a keysym
  KeySym keysym;
  if (event->type == KeyPress && m_ic != NULL) {
    // do multibyte lookup.  can only call XmbLookupString with a
    // key press event and a valid XIC so we checked those above.
    char scratch[32];
    int n = sizeof(scratch) / sizeof(scratch[0]);
    char *buffer = scratch;
    int status;
    n = XmbLookupString(m_ic, event, buffer, n, &keysym, &status);
    if (status == XBufferOverflow) {
      // not enough space.  grow buffer and try again.
      buffer = new char[n];
      n = XmbLookupString(m_ic, event, buffer, n, &keysym, &status);
      delete[] buffer;
    }

    // see what we got.  since we don't care about the string
    // we'll just look for a keysym.
    switch (status) {
    default:
    case XLookupNone:
    case XLookupChars:
      keysym = 0;
      break;

    case XLookupKeySym:
    case XLookupBoth:
      break;
    }
  } else {
    // plain old lookup
    char dummy[1];
    XLookupString(event, dummy, 0, &keysym, NULL);
  }

  LOG((CLOG_DEBUG2 "mapped code=%d to keysym=0x%04x", event->keycode, keysym));

  // convert key
  KeyID result = XWindowsUtil::mapKeySymToKeyID(keysym);
  LOG((CLOG_DEBUG2 "mapped keysym=0x%04x to keyID=%d", keysym, result));
  return result;
}

ButtonID XWindowsScreen::mapButtonFromX(const XButtonEvent *event) const
{
  unsigned int button = event->button;

  // first three buttons map to 1, 2, 3 (kButtonLeft, Middle, Right)
  if (button >= 1 && button <= 3) {
    return static_cast<ButtonID>(button);
  }

  // buttons 4 and 5 are ignored here.  they're used for the wheel.
  // buttons 6, 7, etc and up map to 4, 5, etc.
  else if (button >= 6) {
    return static_cast<ButtonID>(button - 2);
  }

  // unknown button
  else {
    return kButtonNone;
  }
}

unsigned int XWindowsScreen::mapButtonToX(ButtonID id) const
{
  // map button -1 to button 4 (+wheel)
  if (id == static_cast<ButtonID>(-1)) {
    id = 4;
  }

  // map button -2 to button 5 (-wheel)
  else if (id == static_cast<ButtonID>(-2)) {
    id = 5;
  }

  // map buttons 4, 5, etc. to 6, 7, etc. to make room for buttons
  // 4 and 5 used to simulate the mouse wheel.
  else if (id >= 4) {
    id += 2;
  }

  // check button is in legal range
  if (id < 1 || id > m_buttons.size()) {
    // out of range
    return 0;
  }

  // map button
  return static_cast<unsigned int>(id);
}

void XWindowsScreen::warpCursorNoFlush(int32_t x, int32_t y)
{
  assert(m_window != None);

  // send an event that we can recognize before the mouse warp
  XEvent eventBefore;
  eventBefore.type = MotionNotify;
  eventBefore.xmotion.display = m_display;
  eventBefore.xmotion.window = m_window;
  eventBefore.xmotion.root = m_root;
  eventBefore.xmotion.subwindow = m_window;
  eventBefore.xmotion.time = CurrentTime;
  eventBefore.xmotion.x = x;
  eventBefore.xmotion.y = y;
  eventBefore.xmotion.x_root = x;
  eventBefore.xmotion.y_root = y;
  eventBefore.xmotion.state = 0;
  eventBefore.xmotion.is_hint = NotifyNormal;
  eventBefore.xmotion.same_screen = True;
  XEvent eventAfter = eventBefore;
  XSendEvent(m_display, m_window, False, 0, &eventBefore);

  // warp mouse
  XWarpPointer(m_display, None, m_root, 0, 0, 0, 0, x, y);

  // send an event that we can recognize after the mouse warp
  XSendEvent(m_display, m_window, False, 0, &eventAfter);
  XSync(m_display, False);

  LOG((CLOG_DEBUG2 "warped to %d,%d", x, y));
}

void XWindowsScreen::updateButtons()
{
  // query the button mapping
  uint32_t numButtons = XGetPointerMapping(m_display, NULL, 0);
  unsigned char *tmpButtons = new unsigned char[numButtons];
  XGetPointerMapping(m_display, tmpButtons, numButtons);

  // find the largest logical button id
  unsigned char maxButton = 0;
  for (uint32_t i = 0; i < numButtons; ++i) {
    if (tmpButtons[i] > maxButton) {
      maxButton = tmpButtons[i];
    }
  }

  // allocate button array
  m_buttons.resize(maxButton);

  // fill in button array values.  m_buttons[i] is the physical
  // button number for logical button i+1.
  for (uint32_t i = 0; i < numButtons; ++i) {
    m_buttons[i] = 0;
  }
  for (uint32_t i = 0; i < numButtons; ++i) {
    m_buttons[tmpButtons[i] - 1] = i + 1;
  }

  // clean up
  delete[] tmpButtons;
}

bool XWindowsScreen::grabMouseAndKeyboard()
{
  unsigned int event_mask = ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask;

  // grab the mouse and keyboard.  keep trying until we get them.
  // if we can't grab one after grabbing the other then ungrab
  // and wait before retrying.  give up after s_timeout seconds.
  static const double s_timeout = 1.0;
  int result;
  Stopwatch timer;
  do {
    // keyboard first
    do {
      result = XGrabKeyboard(m_display, m_window, True, GrabModeAsync, GrabModeAsync, CurrentTime);
      assert(result != GrabNotViewable);
      if (result != GrabSuccess) {
        LOG((CLOG_DEBUG2 "waiting to grab keyboard"));
        ARCH->sleep(0.05);
        if (timer.getTime() >= s_timeout) {
          LOG((CLOG_DEBUG2 "grab keyboard timed out"));
          return false;
        }
      }
    } while (result != GrabSuccess);
    LOG((CLOG_DEBUG2 "grabbed keyboard"));

    // now the mouse --- use event_mask to get EnterNotify, LeaveNotify events
    result =
        XGrabPointer(m_display, m_window, False, event_mask, GrabModeAsync, GrabModeAsync, m_window, None, CurrentTime);
    assert(result != GrabNotViewable);
    if (result != GrabSuccess) {
      // back off to avoid grab deadlock
      XUngrabKeyboard(m_display, CurrentTime);
      LOG((CLOG_DEBUG2 "ungrabbed keyboard, waiting to grab pointer"));
      ARCH->sleep(0.05);
      if (timer.getTime() >= s_timeout) {
        LOG((CLOG_DEBUG2 "grab pointer timed out"));
        return false;
      }
    }
  } while (result != GrabSuccess);

  LOG((CLOG_DEBUG1 "grabbed pointer and keyboard"));
  return true;
}

void XWindowsScreen::refreshKeyboard(XEvent *event)
{
  if (XPending(m_display) > 0) {
    XEvent tmpEvent;
    XPeekEvent(m_display, &tmpEvent);
    if (tmpEvent.type == MappingNotify) {
      // discard this event since another follows.
      // we tend to get a bunch of these in a row.
      return;
    }
  }

  // keyboard mapping changed
#if HAVE_XKB_EXTENSION
  if (m_xkb && event->type == m_xkbEventBase) {
    XkbRefreshKeyboardMapping((XkbMapNotifyEvent *)event);
  } else
#else
  {
    XRefreshKeyboardMapping(&event->xmapping);
  }
#endif
    m_keyState->updateKeyMap();
  m_keyState->updateKeyState();
}

//
// XWindowsScreen::HotKeyItem
//

XWindowsScreen::HotKeyItem::HotKeyItem(int keycode, unsigned int mask) : m_keycode(keycode), m_mask(mask)
{
  // do nothing
}

bool XWindowsScreen::HotKeyItem::operator<(const HotKeyItem &x) const
{
  return (m_keycode < x.m_keycode || (m_keycode == x.m_keycode && m_mask < x.m_mask));
}

bool XWindowsScreen::detectXI2()
{
  int event, error;
  return XQueryExtension(m_display, "XInputExtension", &xi_opcode, &event, &error);
}

#ifdef HAVE_XI2
void XWindowsScreen::selectXIRawMotion()
{
  XIEventMask mask;

  mask.deviceid = XIAllDevices;
  mask.mask_len = XIMaskLen(XI_RawMotion);
  mask.mask = (unsigned char *)calloc(mask.mask_len, sizeof(char));
  mask.deviceid = XIAllMasterDevices;
  memset(mask.mask, 0, 2);
  XISetMask(mask.mask, XI_RawKeyRelease);
  XISetMask(mask.mask, XI_RawMotion);
  XISelectEvents(m_display, DefaultRootWindow(m_display), &mask, 1);
  free(mask.mask);
}
#endif
