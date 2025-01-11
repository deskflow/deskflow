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

#include "deskflow/Clipboard.h"
#include "deskflow/protocol_types.h"
#include "server/ClientProxy.h"

class Event;
class EventQueueTimer;
class IEventQueue;

//! Proxy for client implementing protocol version 1.0
class ClientProxy1_0 : public ClientProxy
{
public:
  ClientProxy1_0(const std::string &name, deskflow::IStream *adoptedStream, IEventQueue *events);
  ClientProxy1_0(ClientProxy1_0 const &) = delete;
  ClientProxy1_0(ClientProxy1_0 &&) = delete;
  ~ClientProxy1_0();

  ClientProxy1_0 &operator=(ClientProxy1_0 const &) = delete;
  ClientProxy1_0 &operator=(ClientProxy1_0 &&) = delete;

  // IScreen
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

protected:
  virtual bool parseHandshakeMessage(const uint8_t *code);
  virtual bool parseMessage(const uint8_t *code);

  virtual void resetHeartbeatRate();
  virtual void setHeartbeatRate(double rate, double alarm);
  virtual void resetHeartbeatTimer();
  virtual void addHeartbeatTimer();
  virtual void removeHeartbeatTimer();
  virtual bool recvClipboard();

private:
  void disconnect();
  void removeHandlers();

  void handleData(const Event &, void *);
  void handleDisconnect(const Event &, void *);
  void handleWriteError(const Event &, void *);
  void handleFlatline(const Event &, void *);

  bool recvInfo();
  bool recvGrabClipboard();

protected:
  struct ClientClipboard
  {
  public:
    ClientClipboard();

  public:
    Clipboard m_clipboard;
    uint32_t m_sequenceNumber;
    bool m_dirty;
  };

  ClientClipboard m_clipboard[kClipboardEnd];

private:
  typedef bool (ClientProxy1_0::*MessageParser)(const uint8_t *);

  ClientInfo m_info;
  double m_heartbeatAlarm;
  EventQueueTimer *m_heartbeatTimer;
  MessageParser m_parser;
  IEventQueue *m_events;
};
