/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2006 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_2.h"

//! Proxy for client implementing protocol version 1.3
class ClientProxy1_3 : public ClientProxy1_2
{
public:
  ClientProxy1_3(const std::string &name, deskflow::IStream *adoptedStream, IEventQueue *events);
  ClientProxy1_3(ClientProxy1_3 const &) = delete;
  ClientProxy1_3(ClientProxy1_3 &&) = delete;
  ~ClientProxy1_3();

  ClientProxy1_3 &operator=(ClientProxy1_3 const &) = delete;
  ClientProxy1_3 &operator=(ClientProxy1_3 &&) = delete;

  // IClient overrides
  virtual void mouseWheel(int32_t xDelta, int32_t yDelta);

  void handleKeepAlive(const Event &, void *);

protected:
  // ClientProxy overrides
  virtual bool parseMessage(const uint8_t *code);
  virtual void resetHeartbeatRate();
  virtual void setHeartbeatRate(double rate, double alarm);
  virtual void resetHeartbeatTimer();
  virtual void addHeartbeatTimer();
  virtual void removeHeartbeatTimer();
  virtual void keepAlive();

private:
  double m_keepAliveRate;
  EventQueueTimer *m_keepAliveTimer;
  IEventQueue *m_events;
};
