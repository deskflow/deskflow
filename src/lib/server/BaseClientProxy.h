/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClient.h"

namespace deskflow {
class IStream;
}

//! Generic proxy for client or primary
class BaseClientProxy : public IClient
{
public:
  /*!
  \c name is the name of the client.
  */
  BaseClientProxy(const std::string &name);
  ~BaseClientProxy() override = default;

  //! @name manipulators
  //@{

  //! Save cursor position
  /*!
  Save the position of the cursor when jumping from client.
  */
  void setJumpCursorPos(int32_t x, int32_t y);

  //@}
  //! @name accessors
  //@{

  //! Get cursor position
  /*!
  Get the position of the cursor when last jumping from client.
  */
  void getJumpCursorPos(int32_t &x, int32_t &y) const;

  //! Get cursor position
  /*!
  Return if this proxy is for client or primary.
  */
  virtual bool isPrimary() const
  {
    return false;
  }

  //@}

  // IScreen
  void *getEventTarget() const override = 0;
  bool getClipboard(ClipboardID id, IClipboard *) const override = 0;
  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override = 0;
  void getCursorPos(int32_t &x, int32_t &y) const override = 0;

  // IClient overrides
  void enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool forScreensaver) override = 0;
  bool leave() override = 0;
  void setClipboard(ClipboardID, const IClipboard *) override = 0;
  void grabClipboard(ClipboardID) override = 0;
  void setClipboardDirty(ClipboardID, bool) override = 0;
  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override = 0;
  void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &lang) override = 0;
  void keyUp(KeyID, KeyModifierMask, KeyButton) override = 0;
  void mouseDown(ButtonID) override = 0;
  void mouseUp(ButtonID) override = 0;
  void mouseMove(int32_t xAbs, int32_t yAbs) override = 0;
  void mouseRelativeMove(int32_t xRel, int32_t yRel) override = 0;
  void mouseWheel(int32_t xDelta, int32_t yDelta) override = 0;
  void screensaver(bool activate) override = 0;
  void resetOptions() override = 0;
  void setOptions(const OptionsList &options) override = 0;
  virtual void sendDragInfo(uint32_t fileCount, const char *info, size_t size) = 0;
  virtual void fileChunkSending(uint8_t mark, char *data, size_t dataSize) = 0;
  virtual std::string getSecureInputApp() const = 0;
  virtual void secureInputNotification(const std::string &app) const = 0;
  std::string getName() const override;
  virtual deskflow::IStream *getStream() const = 0;

private:
  std::string m_name;
  int32_t m_x, m_y;
};
