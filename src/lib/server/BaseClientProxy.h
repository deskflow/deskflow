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
  ~BaseClientProxy();

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
  virtual void *getEventTarget() const = 0;
  virtual bool getClipboard(ClipboardID id, IClipboard *) const = 0;
  virtual void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const = 0;
  virtual void getCursorPos(int32_t &x, int32_t &y) const = 0;

  // IClient overrides
  virtual void enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool forScreensaver) = 0;
  virtual bool leave() = 0;
  virtual void setClipboard(ClipboardID, const IClipboard *) = 0;
  virtual void grabClipboard(ClipboardID) = 0;
  virtual void setClipboardDirty(ClipboardID, bool) = 0;
  virtual void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) = 0;
  virtual void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &lang) = 0;
  virtual void keyUp(KeyID, KeyModifierMask, KeyButton) = 0;
  virtual void mouseDown(ButtonID) = 0;
  virtual void mouseUp(ButtonID) = 0;
  virtual void mouseMove(int32_t xAbs, int32_t yAbs) = 0;
  virtual void mouseRelativeMove(int32_t xRel, int32_t yRel) = 0;
  virtual void mouseWheel(int32_t xDelta, int32_t yDelta) = 0;
  virtual void screensaver(bool activate) = 0;
  virtual void resetOptions() = 0;
  virtual void setOptions(const OptionsList &options) = 0;
  virtual void sendDragInfo(uint32_t fileCount, const char *info, size_t size) = 0;
  virtual void fileChunkSending(uint8_t mark, char *data, size_t dataSize) = 0;
  virtual std::string getSecureInputApp() const = 0;
  virtual void secureInputNotification(const std::string &app) const = 0;
  virtual std::string getName() const;
  virtual deskflow::IStream *getStream() const = 0;

private:
  std::string m_name;
  int32_t m_x, m_y;
};
