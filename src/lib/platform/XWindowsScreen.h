/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/KeyMap.h"
#include "deskflow/PlatformScreen.h"
#include "platform/XWindowsPowerManager.h"

#include <set>
#include <vector>

#include <QString>

#include <X11/Xlib.h>

class XWindowsClipboard;
class XWindowsKeyState;
class XWindowsScreenSaver;

//! Implementation of IPlatformScreen for X11
class XWindowsScreen : public PlatformScreen
{
public:
  XWindowsScreen(
      const char *displayName, bool isPrimary, int mouseScrollDelta, IEventQueue *events,
      deskflow::ClientScrollDirection m_clientScrollDirection = deskflow::ClientScrollDirection::Normal
  );
  ~XWindowsScreen() override;

  //! @name manipulators
  //@{

  //@}

  // IScreen overrides
  void *getEventTarget() const final;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override;
  void getCursorPos(int32_t &x, int32_t &y) const override;

  // IPrimaryScreen overrides
  void reconfigure(uint32_t activeSides) override;
  uint32_t activeSides() override;
  void warpCursor(int32_t x, int32_t y) override;
  uint32_t registerHotKey(KeyID key, KeyModifierMask mask) override;
  void unregisterHotKey(uint32_t id) override;
  void fakeInputBegin() override;
  void fakeInputEnd() override;
  int32_t getJumpZoneSize() const override;
  bool isAnyMouseButtonDown(uint32_t &buttonID) const override;
  void getCursorCenter(int32_t &x, int32_t &y) const override;

  // ISecondaryScreen overrides
  void fakeMouseButton(ButtonID id, bool press) override;
  void fakeMouseMove(int32_t x, int32_t y) override;
  void fakeMouseRelativeMove(int32_t dx, int32_t dy) const override;
  void fakeMouseWheel(int32_t xDelta, int32_t yDelta) const override;

  // IPlatformScreen overrides
  void enable() override;
  void disable() override;
  void enter() override;
  bool canLeave() override;
  void leave() override;
  bool setClipboard(ClipboardID, const IClipboard *) override;
  void checkClipboards() override;
  void openScreensaver(bool notify) override;
  void closeScreensaver() override;
  void screensaver(bool activate) override;
  void resetOptions() override;
  void setOptions(const OptionsList &options) override;
  void setSequenceNumber(uint32_t) override;
  bool isPrimary() const override;
  std::string getSecureInputApp() const override;

protected:
  // IPlatformScreen overrides
  void handleSystemEvent(const Event &event) override;
  void updateButtons() override;
  IKeyState *getKeyState() const override;

private:
  // event sending
  void sendEvent(EventTypes, void * = nullptr);
  void sendClipboardEvent(EventTypes, ClipboardID);

  // create the transparent cursor
  Cursor createBlankCursor() const;

  // determine the clipboard from the X selection.  returns
  // kClipboardEnd if no such clipboard.
  ClipboardID getClipboardID(Atom selection) const;

  // continue processing a selection request
  void processClipboardRequest(Window window, Time time, Atom property) const;

  // terminate a selection request
  void destroyClipboardRequest(Window window) const;

  // X I/O error handler
  void onError();
  static int ioErrorHandler(Display *);

  class KeyEventFilter
  {
  public:
    int m_event;
    Window m_window;
    Time m_time;
    KeyCode m_keycode;
  };

  Display *openDisplay(const char *displayName);
  void saveShape();
  void setShape(int32_t width, int32_t height);
  Window openWindow() const;
  void openIM();

  bool grabMouseAndKeyboard();
  void onKeyPress(XKeyEvent &);
  void onKeyRelease(XKeyEvent &, bool isRepeat);
  bool onHotKey(const XKeyEvent &, bool isRepeat);
  void onMousePress(const XButtonEvent &);
  void onMouseRelease(const XButtonEvent &);
  void onMouseMove(const XMotionEvent &);

  bool detectXI2();
#ifdef HAVE_XI2
  void selectXIRawMotion();
#endif
  void selectEvents(Window) const;
  void doSelectEvents(Window) const;

  KeyID mapKeyFromX(XKeyEvent *) const;
  ButtonID mapButtonFromX(const XButtonEvent *) const;
  unsigned int mapButtonToX(ButtonID id) const;

  void warpCursorNoFlush(int32_t x, int32_t y);

  void refreshKeyboard(XEvent *);

  static Bool findKeyEvent(Display *, XEvent *xevent, XPointer arg);

private:
  struct HotKeyItem
  {
  public:
    HotKeyItem(int, unsigned int);

    bool operator<(const HotKeyItem &) const;

  private:
    int m_keycode;
    unsigned int m_mask;
  };
  using FilteredKeycodes = std::set<bool>;
  using HotKeyList = std::vector<std::pair<int, unsigned int>>;
  using HotKeyMap = std::map<uint32_t, HotKeyList>;
  using HotKeyIDList = std::vector<uint32_t>;
  using HotKeyToIDMap = std::map<HotKeyItem, uint32_t>;

  // true if screen is being used as a primary screen, false otherwise
  bool m_isPrimary;
  int m_mouseScrollDelta;

  Display *m_display = nullptr;
  Window m_root = None;
  Window m_window = None;

  // true if mouse has entered the screen
  bool m_isOnScreen;

  uint32_t m_activeSides = 0;
  // screen shape stuff
  int32_t m_x = 0;
  int32_t m_y = 0;
  int32_t m_w = 0;
  int32_t m_h = 0;
  int32_t m_xCenter = 0;
  int32_t m_yCenter = 0;

  // last mouse position
  int32_t m_xCursor = 0;
  int32_t m_yCursor = 0;

  // keyboard stuff
  XWindowsKeyState *m_keyState = nullptr;

  // hot key stuff
  HotKeyMap m_hotKeys;
  HotKeyIDList m_oldHotKeyIDs;
  HotKeyToIDMap m_hotKeyToIDMap;

  // input focus stuff
  Window m_lastFocus = None;
  int m_lastFocusRevert = RevertToNone;

  // input method stuff
  XIM m_im = nullptr;
  XIC m_ic = nullptr;
  KeyCode m_lastKeycode = 0;
  FilteredKeycodes m_filtered;

  // clipboards
  XWindowsClipboard *m_clipboard[kClipboardEnd];
  uint32_t m_sequenceNumber = 0;

  // screen saver stuff
  XWindowsScreenSaver *m_screensaver = nullptr;
  bool m_screensaverNotify = false;

  // logical to physical button mapping.  m_buttons[i] gives the
  // physical button for logical button i+1.
  std::vector<unsigned char> m_buttons;

  // true if global auto-repeat was enabled before we turned it off
  bool m_autoRepeat;

  // stuff to workaround xtest being xinerama unaware.  attempting
  // to fake a mouse motion under xinerama may behave strangely,
  // especially if screen 0 is not at 0,0 or if faking a motion on
  // a screen other than screen 0.
  bool m_xtestIsXineramaUnaware = true;
  bool m_xinerama;

  // stuff to work around lost focus issues on certain systems
  // (ie: a MythTV front-end).
  bool m_preserveFocus = false;

  // XKB extension stuff
  bool m_xkb = false;
  int m_xkbEventBase;

  bool m_xi2detected = false;

  // XRandR extension stuff
  bool m_xrandr = false;
  int m_xrandrEventBase;

  IEventQueue *m_events = nullptr;
  deskflow::KeyMap m_keyMap;

  // pointer to (singleton) screen.  this is only needed by
  // ioErrorHandler().
  static XWindowsScreen *s_screen;
  XWindowsPowerManager m_powerManager;
};
