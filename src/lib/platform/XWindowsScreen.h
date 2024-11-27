/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "arch/Arch.h"
#include "common/stdset.h"
#include "common/stdvector.h"
#include "deskflow/ClientArgs.h"
#include "deskflow/KeyMap.h"
#include "deskflow/PlatformScreen.h"
#include "platform/XWindowsPowerManager.h"

#if X_DISPLAY_MISSING
#error X11 is required to build deskflow
#else
#include <X11/Xlib.h>
#endif

class XWindowsClipboard;
class XWindowsKeyState;
class XWindowsScreenSaver;

//! Implementation of IPlatformScreen for X11
class XWindowsScreen : public PlatformScreen
{
public:
  XWindowsScreen(
      const char *displayName, bool isPrimary, bool disableXInitThreads, int mouseScrollDelta, IEventQueue *events,
      deskflow::ClientScrollDirection m_clientScrollDirection = deskflow::ClientScrollDirection::SERVER
  );
  virtual ~XWindowsScreen() override;

  //! @name manipulators
  //@{

  //@}

  // IScreen overrides
  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(SInt32 &x, SInt32 &y, SInt32 &width, SInt32 &height) const override;
  void getCursorPos(SInt32 &x, SInt32 &y) const override;

  // IPrimaryScreen overrides
  void reconfigure(UInt32 activeSides) override;
  void warpCursor(SInt32 x, SInt32 y) override;
  UInt32 registerHotKey(KeyID key, KeyModifierMask mask) override;
  void unregisterHotKey(UInt32 id) override;
  void fakeInputBegin() override;
  void fakeInputEnd() override;
  SInt32 getJumpZoneSize() const override;
  bool isAnyMouseButtonDown(UInt32 &buttonID) const override;
  void getCursorCenter(SInt32 &x, SInt32 &y) const override;

  // ISecondaryScreen overrides
  void fakeMouseButton(ButtonID id, bool press) override;
  void fakeMouseMove(SInt32 x, SInt32 y) override;
  void fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const override;
  void fakeMouseWheel(SInt32 xDelta, SInt32 yDelta) const override;

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
  void setSequenceNumber(UInt32) override;
  bool isPrimary() const override;
  String getSecureInputApp() const override;

protected:
  // IPlatformScreen overrides
  void handleSystemEvent(const Event &, void *) override;
  void updateButtons() override;
  IKeyState *getKeyState() const override;

private:
  // event sending
  void sendEvent(Event::Type, void * = NULL);
  void sendClipboardEvent(Event::Type, ClipboardID);

  // create the transparent cursor
  Cursor createBlankCursor() const;

  // determine the clipboard from the X selection.  returns
  // kClipboardEnd if no such clipboard.
  ClipboardID getClipboardID(Atom selection) const;

  // continue processing a selection request
  void processClipboardRequest(Window window, Time time, Atom property);

  // terminate a selection request
  void destroyClipboardRequest(Window window);

  // X I/O error handler
  void onError();
  static int ioErrorHandler(Display *);

private:
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
  void setShape(SInt32 width, SInt32 height);
  Window openWindow() const;
  void openIM();

  bool grabMouseAndKeyboard();
  void onKeyPress(XKeyEvent &);
  void onKeyRelease(XKeyEvent &, bool isRepeat);
  bool onHotKey(XKeyEvent &, bool isRepeat);
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

  void warpCursorNoFlush(SInt32 x, SInt32 y);

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
  using HotKeyMap = std::map<UInt32, HotKeyList>;
  using HotKeyIDList = std::vector<UInt32>;
  using HotKeyToIDMap = std::map<HotKeyItem, UInt32>;

  // true if screen is being used as a primary screen, false otherwise
  bool m_isPrimary;
  int m_mouseScrollDelta;

  Display *m_display;
  Window m_root;
  Window m_window;

  // true if mouse has entered the screen
  bool m_isOnScreen;

  // screen shape stuff
  SInt32 m_x, m_y;
  SInt32 m_w, m_h;
  SInt32 m_xCenter, m_yCenter;

  // last mouse position
  SInt32 m_xCursor, m_yCursor;

  // keyboard stuff
  XWindowsKeyState *m_keyState;

  // hot key stuff
  HotKeyMap m_hotKeys;
  HotKeyIDList m_oldHotKeyIDs;
  HotKeyToIDMap m_hotKeyToIDMap;

  // input focus stuff
  Window m_lastFocus;
  int m_lastFocusRevert;

  // input method stuff
  XIM m_im;
  XIC m_ic;
  KeyCode m_lastKeycode;
  FilteredKeycodes m_filtered;

  // clipboards
  XWindowsClipboard *m_clipboard[kClipboardEnd];
  UInt32 m_sequenceNumber;

  // screen saver stuff
  XWindowsScreenSaver *m_screensaver;
  bool m_screensaverNotify;

  // logical to physical button mapping.  m_buttons[i] gives the
  // physical button for logical button i+1.
  std::vector<unsigned char> m_buttons;

  // true if global auto-repeat was enabled before we turned it off
  bool m_autoRepeat;

  // stuff to workaround xtest being xinerama unaware.  attempting
  // to fake a mouse motion under xinerama may behave strangely,
  // especially if screen 0 is not at 0,0 or if faking a motion on
  // a screen other than screen 0.
  bool m_xtestIsXineramaUnaware;
  bool m_xinerama;

  // stuff to work around lost focus issues on certain systems
  // (ie: a MythTV front-end).
  bool m_preserveFocus;

  // XKB extension stuff
  bool m_xkb;
  int m_xkbEventBase;

  bool m_xi2detected;

  // XRandR extension stuff
  bool m_xrandr;
  int m_xrandrEventBase;

  IEventQueue *m_events;
  deskflow::KeyMap m_keyMap;

  // pointer to (singleton) screen.  this is only needed by
  // ioErrorHandler().
  static XWindowsScreen *s_screen;
  XWindowsPowerManager m_powerManager;
};
