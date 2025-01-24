/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"

namespace deskflow {
class IStream;
}
class IpcMessage;
class IpcLogLineMessage;
class IEventQueue;

class IpcServerProxy
{
  friend class IpcClient;

public:
  IpcServerProxy(deskflow::IStream &stream, IEventQueue *events);
  IpcServerProxy(IpcServerProxy const &) = delete;
  virtual ~IpcServerProxy();

private:
  void send(const IpcMessage &message);

  void handleData(const Event &, void *);
  IpcLogLineMessage *parseLogLine();
  void disconnect();

private:
  deskflow::IStream &m_stream;
  IEventQueue *m_events;
};
