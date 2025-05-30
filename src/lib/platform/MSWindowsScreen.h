/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClientArgs.h"
#include "deskflow/PlatformScreen.h"
#include "platform/MSWindowsHook.h"
#include "platform/MSWindowsPowerManager.h"

#include <map>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class EventQueueTimer;
class MSWindowsDesks;
class MSWindowsKeyState;
class MSWindowsScreenSaver;
class Thread;
class MSWindowsDropTarget;

//! Implementation of IPlatformScreen for Microsoft Windows
class MSWindowsScreen : public PlatformScreen
{
public:
  MSWindowsScreen(
      bool isPrimary, bool noHooks, IEventQueue *events, bool enableLangSync = false,
      deskflow::ClientScrollDirection scrollDirection = deskflow::ClientScrollDirection::Normal
  );
  ~MSWindowsScreen() override;

  //! @name manipulators
  //@{

  //! Initialize
  /*!
  Saves the application's HINSTANCE.  This \b must be called by
  WinMain with the HINSTANCE it was passed.
  */
  static void init(HINSTANCE);

  //@}
  //! @name accessors
  //@{

  //! Get instance
  /*!
  Returns the application instance handle passed to init().
  */
  static HINSTANCE getWindowInstance();

  //@}

  // IScreen overrides
  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override;
  void getCursorPos(int32_t &x, int32_t &y) const override;

  /**
   * \brief Get the position of the cursor on the current machine
   * \param pos the object that the function will use to store the position of
   * the cursor \return true if the function was successful
   */
  virtual bool getThisCursorPos(LPPOINT pos);
  /**
   * \brief Sets the cursor position on the current machine
   * \param x The x coordinate of the cursor
   * \param y The Y coordinate of the cursor
   * \return True is successful
   */
  virtual bool setThisCursorPos(int x, int y);

  /**
   * \brief This function will attempt to switch to the current desktop the
   * mouse is located on
   */
  virtual void updateDesktopThread();

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

  // IKeyState overrides
  virtual void updateKeys();
  void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang) override;
  bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override;
  bool fakeKeyUp(KeyButton button) override;
  void fakeAllKeysUp() override;

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

  // simulate a local key to the system directly
  void fakeLocalKey(KeyButton button, bool press) const;

private:
  // initialization and shutdown operations
  HCURSOR createBlankCursor() const;
  void destroyCursor(HCURSOR cursor) const;
  ATOM createWindowClass() const;
  ATOM createDeskWindowClass(bool isPrimary) const;
  void destroyClass(ATOM windowClass) const;
  HWND createWindow(ATOM windowClass, const wchar_t *name) const;
  void destroyWindow(HWND) const;

  // convenience function to send events
public: // HACK
  void sendEvent(EventTypes type, void * = nullptr);

private: // HACK
  void sendClipboardEvent(EventTypes type, ClipboardID id);

  // handle message before it gets dispatched.  returns true iff
  // the message should not be dispatched.
  bool onPreDispatch(HWND, UINT, WPARAM, LPARAM);

  // handle message before it gets dispatched.  returns true iff
  // the message should not be dispatched.
  bool onPreDispatchPrimary(HWND, UINT, WPARAM, LPARAM);

  // handle secondary message before it gets dispatched.  returns true iff
  // the message should not be dispatched.
  bool onPreDispatchSecondary(HWND, UINT, WPARAM, LPARAM);

  // handle message.  returns true iff handled and optionally sets
  // \c *result (which defaults to 0).
  bool onEvent(HWND, UINT, WPARAM, LPARAM, LRESULT *result);

  // message handlers
  bool onMark(uint32_t mark);
  bool onKey(WPARAM, LPARAM);
  bool onHotKey(WPARAM, LPARAM);
  bool onMouseButton(WPARAM, LPARAM);
  bool onMouseMove(int32_t x, int32_t y);
  bool onMouseWheel(int32_t xDelta, int32_t yDelta);
  bool onScreensaver(bool activated);
  bool onDisplayChange();
  void onClipboardChange();

  // warp cursor without discarding queued events
  void warpCursorNoFlush(int32_t x, int32_t y);

  // discard posted messages
  void nextMark();

  // test if event should be ignored
  bool ignore() const;

  // update screen size cache
  void updateScreenShape();

  // fix timer callback
  void handleFixes();

  // fix the clipboard viewer chain
  void fixClipboardViewer();

  // enable/disable special key combinations so we can catch/pass them
  void enableSpecialKeys(bool) const;

  // map a button event to a button ID
  ButtonID mapButtonFromEvent(WPARAM msg, LPARAM button) const;

  // map a button event to a press (true) or release (false)
  bool mapPressFromEvent(WPARAM msg, LPARAM button) const;

  // job to update the key state
  void updateKeysCB(void *);

  // determine whether the mouse is hidden by the system.
  // if true and on secondary screen, enable mouse keys to show the cursor.
  // we were previously restoring the old mouse key settings when not needed, but this was causing
  // issues where the mouse cursor becomes permanently hidden, even if there is a real mouse
  // attached to the system; this could be a windows bug, but losing your mouse is a nightmare
  // so we shouldn't risk doing that.
  void setupMouseKeys();

  // enables the mouse keys accessibility feature to to ensure the
  // mouse cursor can be shown.
  void updateMouseKeys();

  // our window proc
  static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

  // save last position of mouse to compute next delta movement
  void saveMousePosition(int32_t x, int32_t y);

  // check if it is a modifier key repeating message
  bool isModifierRepeat(KeyModifierMask oldState, KeyModifierMask state, WPARAM wParam) const;

private:
  struct HotKeyItem
  {
  public:
    HotKeyItem(UINT vk, UINT modifiers);

    UINT getVirtualKey() const;

    bool operator<(const HotKeyItem &) const;

  private:
    UINT m_keycode;
    UINT m_mask;
  };
  using HotKeyMap = std::map<uint32_t, HotKeyItem>;
  using HotKeyIDList = std::vector<uint32_t>;
  using HotKeyToIDMap = std::map<HotKeyItem, uint32_t>;
  using PrimaryKeyDownList = std::vector<KeyButton>;

  static HINSTANCE s_windowInstance;

  // true if screen is being used as a primary screen, false otherwise
  bool m_isPrimary;

  // true if hooks are not to be installed (useful for debugging)
  bool m_noHooks;

  // true if mouse has entered the screen
  bool m_isOnScreen;

  // true if the screen is enabled
  bool m_isEnabled = false;

  // our resources
  ATOM m_class = 0;

  // screen shape stuff
  int32_t m_x = 0;
  int32_t m_y = 0;
  int32_t m_w = 0;
  int32_t m_h = 0;
  int32_t m_xCenter = 0;
  int32_t m_yCenter = 0;

  // true if system appears to have multiple monitors
  bool m_multimon = false;

  // last mouse position
  int32_t m_xCursor = 0;
  int32_t m_yCursor = 0;

  // last clipboard
  uint32_t m_sequenceNumber = 0;

  // used to discard queued messages that are no longer needed
  uint32_t m_mark = 0;
  uint32_t m_markReceived = 0;

  // the main loop's thread id
  DWORD m_threadID;

  // timer for periodically checking stuff that requires polling
  EventQueueTimer *m_fixTimer = nullptr;

  // the keyboard layout to use when off primary screen
  HKL m_keyLayout = nullptr;

  // screen saver stuff
  MSWindowsScreenSaver *m_screensaver = nullptr;
  bool m_screensaverNotify = false;
  bool m_screensaverActive = false;

  // clipboard stuff.  our window is used mainly as a clipboard
  // owner and as a link in the clipboard viewer chain.
  HWND m_window = nullptr;
  DWORD m_clipboardSequenceNumber = 0;
  bool m_ownClipboard = false;

  // one desk per desktop and a cond var to communicate with it
  MSWindowsDesks *m_desks = nullptr;

  // keyboard stuff
  MSWindowsKeyState *m_keyState = nullptr;

  // hot key stuff
  HotKeyMap m_hotKeys;
  HotKeyIDList m_oldHotKeyIDs;
  HotKeyToIDMap m_hotKeyToIDMap;

  // map of button state
  bool m_buttons[1 + kButtonExtra0 + 1];

  // m_hasMouse is true if there's a mouse attached to the system or
  // mouse keys is simulating one.  we track this so we can force the
  // cursor to be displayed when the user has entered this screen.
  bool m_hasMouse;

  bool m_gotMouseKeys = false;
  MOUSEKEYS m_mouseKeys;

  MSWindowsHook m_hook;

  static MSWindowsScreen *s_screen;

  IEventQueue *m_events;

  std::string m_desktopPath;

  PrimaryKeyDownList m_primaryKeyDownList;
  MSWindowsPowerManager m_powerManager;
};
