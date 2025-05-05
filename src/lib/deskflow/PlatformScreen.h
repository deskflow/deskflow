/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClientArgs.h"
#include "deskflow/IPlatformScreen.h"

#include <stdexcept>

//! Base screen implementation
/*!
This screen implementation is the superclass of all other screen
implementations.  It implements a handful of methods and requires
subclasses to implement the rest.
*/
class PlatformScreen : public IPlatformScreen
{
public:
  PlatformScreen(
      IEventQueue *events, deskflow::ClientScrollDirection scrollDirection = deskflow::ClientScrollDirection::SERVER
  );
  ~PlatformScreen() override = default;

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
  void updateKeyMap() override;
  void updateKeyState() override;
  void setHalfDuplexMask(KeyModifierMask) override;
  void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &) override;
  bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override;
  bool fakeKeyUp(KeyButton button) override;
  void fakeAllKeysUp() override;
  bool fakeCtrlAltDel() override;
  bool isKeyDown(KeyButton) const override;
  KeyModifierMask getActiveModifiers() const override;
  KeyModifierMask pollActiveModifiers() const override;
  int32_t pollActiveGroup() const override;
  void pollPressedKeys(KeyButtonSet &pressedKeys) const override;

  // IPlatformScreen overrides
  void enable() override = 0;
  void disable() override = 0;
  void enter() override = 0;
  bool canLeave() override = 0;
  void leave() override = 0;
  bool setClipboard(ClipboardID, const IClipboard *) override = 0;
  void checkClipboards() override = 0;
  void openScreensaver(bool notify) override = 0;
  void closeScreensaver() override = 0;
  void screensaver(bool activate) override = 0;
  void resetOptions() override = 0;
  void setOptions(const OptionsList &options) override = 0;
  void setSequenceNumber(uint32_t) override = 0;
  bool isPrimary() const override = 0;

protected:
  //! Update mouse buttons
  /*!
  Subclasses must implement this method to update their internal mouse
  button mapping and, if desired, state tracking.
  */
  virtual void updateButtons() = 0;

  //! Get the key state
  /*!
  Subclasses must implement this method to return the platform specific
  key state object that each subclass must have.
  */
  virtual IKeyState *getKeyState() const = 0;

  // IPlatformScreen overrides
  void handleSystemEvent(const Event &event, void *) override = 0;

  /*!
   * \brief mapClientScrollDirection
   * Convert scroll according to client scroll directio
   * \return converted value according to the client scroll direction
   */
  virtual int32_t mapClientScrollDirection(int32_t) const;

private:
  /*!
   * \brief m_clientScrollDirection
   * This member contains client scroll direction.
   * This member is used only on client side.
   */
  deskflow::ClientScrollDirection m_clientScrollDirection = deskflow::ClientScrollDirection::SERVER;
};
