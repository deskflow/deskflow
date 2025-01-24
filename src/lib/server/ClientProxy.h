/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"
#include "server/BaseClientProxy.h"

namespace deskflow {
class IStream;
}

//! Generic proxy for client
class ClientProxy : public BaseClientProxy
{
public:
  /*!
  \c name is the name of the client.
  */
  ClientProxy(const std::string &name, deskflow::IStream *adoptedStream);
  ClientProxy(ClientProxy const &) = delete;
  ClientProxy(ClientProxy &&) = delete;
  ~ClientProxy();

  ClientProxy &operator=(ClientProxy const &) = delete;
  ClientProxy &operator=(ClientProxy &&) = delete;

  //! @name manipulators
  //@{

  //! Disconnect
  /*!
  Ask the client to disconnect, using \p msg as the reason.
  */
  void close(const char *msg);

  //@}
  //! @name accessors
  //@{

  //! Get stream
  /*!
  Returns the original stream passed to the c'tor.
  */
  deskflow::IStream *getStream() const override;

  //@}

  // IScreen
  void *getEventTarget() const override;
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
  void sendDragInfo(uint32_t fileCount, const char *info, size_t size) override = 0;
  void fileChunkSending(uint8_t mark, char *data, size_t dataSize) override = 0;
  void secureInputNotification(const std::string &app) const override = 0;

private:
  deskflow::IStream *m_stream;
};
