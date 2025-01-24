/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/protocol_types.h"
#include "server/BaseClientProxy.h"

namespace deskflow {
class Screen;
}

//! Primary screen as pseudo-client
/*!
The primary screen does not have a client associated with it.  This
class provides a pseudo-client to allow the primary screen to be
treated as if it was a client.
*/
class PrimaryClient : public BaseClientProxy
{
public:
  /*!
  \c name is the name of the server and \p screen is primary screen.
  */
  PrimaryClient(const std::string &name, deskflow::Screen *screen);
  ~PrimaryClient();

#ifdef TEST_ENV
  PrimaryClient() : BaseClientProxy("")
  {
  }
#endif

  //! @name manipulators
  //@{

  //! Update configuration
  /*!
  Handles reconfiguration of jump zones.
  */
  virtual void reconfigure(uint32_t activeSides);

  //! Register a system hotkey
  /*!
  Registers a system-wide hotkey for key \p key with modifiers \p mask.
  Returns an id used to unregister the hotkey.
  */
  virtual uint32_t registerHotKey(KeyID key, KeyModifierMask mask);

  //! Unregister a system hotkey
  /*!
  Unregisters a previously registered hot key.
  */
  virtual void unregisterHotKey(uint32_t id);

  //! Prepare to synthesize input on primary screen
  /*!
  Prepares the primary screen to receive synthesized input.  We do not
  want to receive this synthesized input as user input so this method
  ensures that we ignore it.  Calls to \c fakeInputBegin() and
  \c fakeInputEnd() may be nested;  only the outermost have an effect.
  */
  void fakeInputBegin();

  //! Done synthesizing input on primary screen
  /*!
  Undoes whatever \c fakeInputBegin() did.
  */
  void fakeInputEnd();

  //@}
  //! @name accessors
  //@{

  //! Get jump zone size
  /*!
  Return the jump zone size, the size of the regions on the edges of
  the screen that cause the cursor to jump to another screen.
  */
  int32_t getJumpZoneSize() const;

  //! Get cursor center position
  /*!
  Return the cursor center position which is where we park the
  cursor to compute cursor motion deltas and should be far from
  the edges of the screen, typically the center.
  */
  void getCursorCenter(int32_t &x, int32_t &y) const;

  //! Get toggle key state
  /*!
  Returns the primary screen's current toggle modifier key state.
  */
  virtual KeyModifierMask getToggleMask() const;

  //! Get screen lock state
  /*!
  Returns true if the user is locked to the screen.
  */
  bool isLockedToScreen() const;

  //@}

  // FIXME -- these probably belong on IScreen
  virtual void enable();
  virtual void disable();

  // IScreen overrides
  void *getEventTarget() const override;
  bool getClipboard(ClipboardID id, IClipboard *) const override;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override;
  void getCursorPos(int32_t &x, int32_t &y) const override;

  // IClient overrides
  void enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool forScreensaver) override;
  bool leave() override;
  void setClipboard(ClipboardID, const IClipboard *) override;
  void grabClipboard(ClipboardID) override;
  void setClipboardDirty(ClipboardID, bool) override;
  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override;
  void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &) override;
  void keyUp(KeyID, KeyModifierMask, KeyButton) override;
  void mouseDown(ButtonID) override;
  void mouseUp(ButtonID) override;
  void mouseMove(int32_t xAbs, int32_t yAbs) override;
  void mouseRelativeMove(int32_t xRel, int32_t yRel) override;
  void mouseWheel(int32_t xDelta, int32_t yDelta) override;
  void screensaver(bool activate) override;
  void resetOptions() override;
  void setOptions(const OptionsList &options) override;
  void sendDragInfo(uint32_t fileCount, const char *info, size_t size) override;
  void fileChunkSending(uint8_t mark, char *data, size_t dataSize) override;
  std::string getSecureInputApp() const override;
  void secureInputNotification(const std::string &app) const override;

  deskflow::IStream *getStream() const override
  {
    return nullptr;
  }
  bool isPrimary() const override
  {
    return true;
  }

private:
  deskflow::Screen *m_screen;
  bool m_clipboardDirty[kClipboardEnd];
  int32_t m_fakeInputCount;
};
