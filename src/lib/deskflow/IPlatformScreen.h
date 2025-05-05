/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IKeyState.h"
#include "deskflow/IPrimaryScreen.h"
#include "deskflow/IScreen.h"
#include "deskflow/ISecondaryScreen.h"
#include "deskflow/OptionTypes.h"

class IClipboard;

//! Screen interface
/*!
This interface defines the methods common to all platform dependent
screen implementations that are used by both primary and secondary
screens.
*/
class IPlatformScreen : public IScreen, public IPrimaryScreen, public ISecondaryScreen, public IKeyState
{
public:
  //! @name manipulators
  //@{

  IPlatformScreen(IEventQueue *events) : IKeyState(events)
  {
  }

  //! Enable screen
  /*!
  Enable the screen, preparing it to report system and user events.
  For a secondary screen it also means preparing to synthesize events
  and hiding the cursor.
  */
  virtual void enable() = 0;

  //! Disable screen
  /*!
  Undoes the operations in enable() and events should no longer
  be reported.
  */
  virtual void disable() = 0;

  //! Enter screen
  /*!
  Called when the user navigates to this screen.
  */
  virtual void enter() = 0;

  //! Leave screen
  /*!
  Called when the user navigates off the screen.  Returns true if
  the leave can proceed.  A typical reason for failure is being
  unable to install the keyboard and mouse snoopers on a primary
  screen.  Secondary screens should not fail.
  */
  virtual bool canLeave() = 0;

  //! Leave screen
  /*!
  Called when the user navigates off the screen.  Should be gated
  by canLeave().
  */
  virtual void leave() = 0;

  //! Set clipboard
  /*!
  Set the contents of the system clipboard indicated by \c id.
  */
  virtual bool setClipboard(ClipboardID id, const IClipboard *) = 0;

  //! Check clipboard owner
  /*!
  Check ownership of all clipboards and post grab events for any that
  have changed.  This is used as a backup in case the system doesn't
  reliably report clipboard ownership changes.
  */
  virtual void checkClipboards() = 0;

  //! Open screen saver
  /*!
  Open the screen saver.  If \c notify is true then this object must
  send events when the screen saver activates or deactivates until
  \c closeScreensaver() is called.  If \c notify is false then the
  screen saver is disabled and restored on \c closeScreensaver().
  */
  virtual void openScreensaver(bool notify) = 0;

  //! Close screen saver
  /*!
  // Close the screen saver.  Stop reporting screen saver activation
  and deactivation and, if the screen saver was disabled by
  openScreensaver(), enable the screen saver.
  */
  virtual void closeScreensaver() = 0;

  //! Activate/deactivate screen saver
  /*!
  Forcibly activate the screen saver if \c activate is true otherwise
  forcibly deactivate it.
  */
  virtual void screensaver(bool activate) = 0;

  //! Notify of options changes
  /*!
  Reset all options to their default values.
  */
  virtual void resetOptions() = 0;

  //! Notify of options changes
  /*!
  Set options to given values.  Ignore unknown options and don't
  modify options that aren't given in \c options.
  */
  virtual void setOptions(const OptionsList &options) = 0;

  //! Set clipboard sequence number
  /*!
  Sets the sequence number to use in subsequent clipboard events.
  */
  virtual void setSequenceNumber(uint32_t) = 0;

  //! Determine the name of the app causing a secure input state
  /*!
  On MacOS check which app causes a secure input state to be enabled. No
  alternative on other platforms
  */
  virtual std::string getSecureInputApp() const = 0;

  //@}
  //! @name accessors
  //@{

  //! Test if is primary screen
  /*!
  Return true iff this screen is a primary screen.
  */
  virtual bool isPrimary() const = 0;

  //@}

  // IScreen overrides
  void *getEventTarget() const override = 0;
  bool getClipboard(ClipboardID id, IClipboard *) const override = 0;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override = 0;
  void getCursorPos(int32_t &x, int32_t &y) const override = 0;

  // IPrimaryScreen overrides
  void reconfigure(uint32_t activeSides) override = 0;
  void warpCursor(int32_t x, int32_t y) override = 0;
  uint32_t registerHotKey(KeyID key, KeyModifierMask mask) override = 0;
  void unregisterHotKey(uint32_t id) override = 0;
  void fakeInputBegin() override = 0;
  void fakeInputEnd() override = 0;
  int32_t getJumpZoneSize() const override = 0;
  bool isAnyMouseButtonDown(uint32_t &buttonID) const override = 0;
  void getCursorCenter(int32_t &x, int32_t &y) const override = 0;

  // ISecondaryScreen overrides
  void fakeMouseButton(ButtonID id, bool press) override = 0;
  void fakeMouseMove(int32_t x, int32_t y) override = 0;
  void fakeMouseRelativeMove(int32_t dx, int32_t dy) const override = 0;
  void fakeMouseWheel(int32_t xDelta, int32_t yDelta) const override = 0;

  // IKeyState overrides
  void updateKeyMap() override = 0;
  void updateKeyState() override = 0;
  void setHalfDuplexMask(KeyModifierMask) override = 0;
  void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang) override = 0;
  bool
  fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override = 0;
  bool fakeKeyUp(KeyButton button) override = 0;
  void fakeAllKeysUp() override = 0;
  bool fakeCtrlAltDel() override = 0;
  bool fakeMediaKey(KeyID id) override;
  bool isKeyDown(KeyButton) const override = 0;
  KeyModifierMask getActiveModifiers() const override = 0;
  KeyModifierMask pollActiveModifiers() const override = 0;
  int32_t pollActiveGroup() const override = 0;
  void pollPressedKeys(KeyButtonSet &pressedKeys) const override = 0;

protected:
  //! Handle system event
  /*!
  A platform screen is expected to install a handler for system
  events in its c'tor like so:
  \code
  m_events->adoptHandler(EventTypes::System,
                        m_events->getSystemTarget(),
                        new TMethodEventJob<CXXXPlatformScreen>(this,
                            &CXXXPlatformScreen::handleSystemEvent));
  \endcode
  It should remove the handler in its d'tor.  Override the
  \c handleSystemEvent() method to process system events.
  It should post the events \c IScreen as appropriate.

  A primary screen has further responsibilities.  It should post
  the events in \c IPrimaryScreen as appropriate.  It should also
  call \c onKey() on its \c KeyState whenever a key is pressed
  or released (but not for key repeats).  And it should call
  \c updateKeyMap() on its \c KeyState if necessary when the keyboard
  mapping changes.

  The target of all events should be the value returned by
  \c getEventTarget().
  */
  virtual void handleSystemEvent(const Event &event, void *) = 0;
};
