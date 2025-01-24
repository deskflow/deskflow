/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "common/stdmap.h"
#include "common/stdvector.h"
#include "deskflow/DragInformation.h"
#include "deskflow/PlatformScreen.h"
#include "platform/OSXClipboard.h"
#include "platform/OSXPowerManager.h"

#include <Carbon/Carbon.h>
#include <IOKit/IOMessage.h>
#include <bitset>
#include <mach/mach_init.h>
#include <mach/mach_interface.h>
#include <mach/mach_port.h>
#include <memory>

extern "C"
{
  using CGSConnectionID = int;
  CGError CGSSetConnectionProperty(CGSConnectionID cid, CGSConnectionID targetCID, CFStringRef key, CFTypeRef value);
  int _CGSDefaultConnection();
}

template <class T> class CondVar;
class EventQueueTimer;
class Mutex;
class Thread;
class OSXKeyState;
class OSXScreenSaver;
class IEventQueue;
class Mutex;

//! Implementation of IPlatformScreen for OS X
class OSXScreen : public PlatformScreen
{
public:
  OSXScreen(
      IEventQueue *events, bool isPrimary, bool enableLangSync = false,
      deskflow::ClientScrollDirection scrollDirection = deskflow::ClientScrollDirection::SERVER
  );

  virtual ~OSXScreen();

  IEventQueue *getEvents() const
  {
    return m_events;
  }

  // IScreen overrides
  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override;
  void getCursorPos(int32_t &x, int32_t &y) const override;

  // IPrimaryScreen overrides
  void reconfigure(uint32_t activeSides) override;
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
  void fakeDraggingFiles(DragFileList fileList) override;
  std::string &getDraggingFilename() override;
  std::string getSecureInputApp() const override;

  const std::string &getDropTarget() const override
  {
    return m_dropTarget;
  }
  void waitForCarbonLoop() const;

protected:
  // IPlatformScreen overrides
  void handleSystemEvent(const Event &, void *) override;
  void updateButtons() override;
  IKeyState *getKeyState() const override;

private:
  bool updateScreenShape();
  bool updateScreenShape(const CGDirectDisplayID, const CGDisplayChangeSummaryFlags);
  void postMouseEvent(CGPoint &) const;

  // convenience function to send events
  void sendEvent(Event::Type type, void * = NULL) const;
  void sendClipboardEvent(Event::Type type, ClipboardID id) const;

  // message handlers
  bool onMouseMove(CGFloat mx, CGFloat my);
  // mouse button handler.  pressed is true if this is a mousedown
  // event, false if it is a mouseup event.  macButton is the index
  // of the button pressed using the mac button mapping.
  bool onMouseButton(bool pressed, uint16_t macButton);
  bool onMouseWheel(int32_t xDelta, int32_t yDelta) const;

  void constructMouseButtonEventMap();

  bool onKey(CGEventRef event);

  void onMediaKey(CGEventRef event);

  bool onHotKey(EventRef event) const;

  // Added here to allow the carbon cursor hack to be called.
  void showCursor();
  void hideCursor();

  // map deskflow mouse button to mac buttons
  ButtonID mapDeskflowButtonToMac(uint16_t) const;

  // map mac mouse button to deskflow buttons
  ButtonID mapMacButtonToDeskflow(uint16_t) const;

  // map mac scroll wheel value to a deskflow scroll wheel value
  int32_t mapScrollWheelToDeskflow(int32_t) const;

  // map deskflow scroll wheel value to a mac scroll wheel value
  int32_t mapScrollWheelFromDeskflow(int32_t) const;

  // get the current scroll wheel speed
  double getScrollSpeed() const;

  // enable/disable drag handling for buttons 3 and up
  void enableDragTimer(bool enable);

  // drag timer handler
  void handleDrag(const Event &, void *);

  // clipboard check timer handler
  void handleClipboardCheck(const Event &, void *);

  // Resolution switch callback
  static void displayReconfigurationCallback(CGDirectDisplayID, CGDisplayChangeSummaryFlags, void *);

  // fast user switch callback
  static pascal OSStatus userSwitchCallback(EventHandlerCallRef nextHandler, EventRef theEvent, void *inUserData);

  // sleep / wakeup support
  void watchSystemPowerThread(void *);
  static void testCanceled(CFRunLoopTimerRef timer, void *info);
  static void powerChangeCallback(void *refcon, io_service_t service, natural_t messageType, void *messageArgument);
  void handlePowerChangeRequest(natural_t messageType, void *messageArgument);

  void handleConfirmSleep(const Event &event, void *);

  // global hotkey operating mode
  static bool isGlobalHotKeyOperatingModeAvailable();
  static void setGlobalHotKeysEnabled(bool enabled);
  static bool getGlobalHotKeysEnabled();

  // Quartz event tap support
  static CGEventRef handleCGInputEvent(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
  static CGEventRef
  handleCGInputEventSecondary(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

  // convert CFString to char*
  static char *CFStringRefToUTF8String(CFStringRef aString);

  void getDropTargetThread(void *);

private:
  struct HotKeyItem
  {
  public:
    HotKeyItem(uint32_t, uint32_t);
    HotKeyItem(EventHotKeyRef, uint32_t, uint32_t);

    EventHotKeyRef getRef() const;

    bool operator<(const HotKeyItem &) const;

  private:
    EventHotKeyRef m_ref;
    uint32_t m_keycode;
    uint32_t m_mask;
  };

  enum EMouseButtonState
  {
    kMouseButtonUp = 0,
    kMouseButtonDragged,
    kMouseButtonDown,
    kMouseButtonStateMax
  };

  class MouseButtonState
  {
  public:
    void set(uint32_t button, EMouseButtonState state);
    bool any();
    void reset();
    void overwrite(uint32_t buttons);

    bool test(uint32_t button) const;
    int8_t getFirstButtonDown() const;

  private:
    std::bitset<NumButtonIDs> m_buttons;
  };

  using HotKeyMap = std::map<uint32_t, HotKeyItem>;
  using HotKeyIDList = std::vector<uint32_t>;
  using ModifierHotKeyMap = std::map<KeyModifierMask, uint32_t>;
  using HotKeyToIDMap = std::map<HotKeyItem, uint32_t>;

  // true if screen is being used as a primary screen, false otherwise
  bool m_isPrimary;

  // true if mouse has entered the screen
  bool m_isOnScreen;

  // the display
  CGDirectDisplayID m_displayID;

  // screen shape stuff
  int32_t m_x, m_y;
  int32_t m_w, m_h;
  int32_t m_xCenter, m_yCenter;

  // mouse state
  mutable int32_t m_xCursor, m_yCursor;
  mutable bool m_cursorPosValid;

  /* FIXME: this data structure is explicitly marked mutable due
     to a need to track the state of buttons since the remote
     side only lets us know of change events, and because the
     fakeMouseButton button method is marked 'const'. This is
     Evil, and this should be moved to a place where it need not
     be mutable as soon as possible. */
  mutable MouseButtonState m_buttonState;
  using MouseButtonEventMapType = std::map<uint16_t, CGEventType>;
  std::vector<MouseButtonEventMapType> MouseButtonEventMap;

  bool m_cursorHidden;
  int32_t m_dragNumButtonsDown;
  Point m_dragLastPoint;
  EventQueueTimer *m_dragTimer;

  // keyboard stuff
  OSXKeyState *m_keyState;

  // clipboards
  OSXClipboard m_pasteboard;
  uint32_t m_sequenceNumber;

  // screen saver stuff
  OSXScreenSaver *m_screensaver;
  bool m_screensaverNotify;

  // clipboard stuff
  bool m_ownClipboard;
  EventQueueTimer *m_clipboardTimer;

  // window object that gets user input events when the server
  // has focus.
  WindowRef m_hiddenWindow;
  // window object that gets user input events when the server
  // does not have focus.
  WindowRef m_userInputWindow;

  // fast user switching
  EventHandlerRef m_switchEventHandlerRef;

  // sleep / wakeup
  Mutex *m_pmMutex;
  Thread *m_pmWatchThread;
  CondVar<bool> *m_pmThreadReady;
  CFRunLoopRef m_pmRunloop;
  io_connect_t m_pmRootPort;

  // hot key stuff
  HotKeyMap m_hotKeys;
  HotKeyIDList m_oldHotKeyIDs;
  ModifierHotKeyMap m_modifierHotKeys;
  uint32_t m_activeModifierHotKey;
  KeyModifierMask m_activeModifierHotKeyMask;
  HotKeyToIDMap m_hotKeyToIDMap;

  // global hotkey operating mode
  static bool s_testedForGHOM;
  static bool s_hasGHOM;

  // Quartz input event support
  CFMachPortRef m_eventTapPort;
  CFRunLoopSourceRef m_eventTapRLSR;

  // for double click coalescing.
  double m_lastClickTime;
  int m_clickState;
  int32_t m_lastSingleClickXCursor;
  int32_t m_lastSingleClickYCursor;

  IEventQueue *m_events;

  std::unique_ptr<Thread> m_getDropTargetThread;
  std::string m_dropTarget;

#if defined(MAC_OS_X_VERSION_10_7)
  Mutex *m_carbonLoopMutex;
  CondVar<bool> *m_carbonLoopReady;
#endif

  OSXPowerManager m_powerManager;

  class OSXScreenImpl *m_impl;
};
