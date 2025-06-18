/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"
#include "deskflow/OptionTypes.h"

#include <string>

class ClientProxy;
class EventQueueTimer;
namespace deskflow {
class IStream;
}
class Server;
class IEventQueue;

class ClientProxyUnknown
{
public:
  ClientProxyUnknown(deskflow::IStream *stream, double timeout, Server *server, IEventQueue *events);
  ClientProxyUnknown(ClientProxyUnknown const &) = delete;
  ClientProxyUnknown(ClientProxyUnknown &&) = delete;
  ~ClientProxyUnknown();

  ClientProxyUnknown &operator=(ClientProxyUnknown const &) = delete;
  ClientProxyUnknown &operator=(ClientProxyUnknown &&) = delete;

  //! @name manipulators
  //@{

  //! Get the client proxy
  /*!
  Returns the client proxy created after a successful handshake
  (i.e. when this object sends a success event).  Returns nullptr
  if the handshake is unsuccessful or incomplete.
  */
  ClientProxy *orphanClientProxy();

  //! Get the stream
  deskflow::IStream *getStream()
  {
    return m_stream;
  }

  //@}

private:
  void sendSuccess();
  void sendFailure();
  void addStreamHandlers();
  void addProxyHandlers();
  void removeHandlers();
  void initProxy(const std::string &name, int major, int minor);
  void removeTimer();
  void handleData();
  void handleWriteError();
  void handleTimeout();
  void handleDisconnect();
  void handleReady();

private:
  deskflow::IStream *m_stream = nullptr;
  EventQueueTimer *m_timer = nullptr;
  ClientProxy *m_proxy = nullptr;
  bool m_ready = false;
  Server *m_server = nullptr;
  IEventQueue *m_events = nullptr;
};
