/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/String.h"
#include "deskflow/option_types.h"

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
  (i.e. when this object sends a success event).  Returns NULL
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
  void initProxy(const String &name, int major, int minor);
  void removeTimer();
  void handleData(const Event &, void *);
  void handleWriteError(const Event &, void *);
  void handleTimeout(const Event &, void *);
  void handleDisconnect(const Event &, void *);
  void handleReady(const Event &, void *);

private:
  deskflow::IStream *m_stream;
  EventQueueTimer *m_timer;
  ClientProxy *m_proxy;
  bool m_ready;
  Server *m_server;
  IEventQueue *m_events;
};
