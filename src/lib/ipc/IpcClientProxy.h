/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include "common/ipc.h"

namespace deskflow {
class IStream;
}
class IpcMessage;
class IpcCommandMessage;
class IpcSettingMessage;
class IpcHelloMessage;
class IEventQueue;

class IpcClientProxy
{
  friend class IpcServer;

public:
  IpcClientProxy(deskflow::IStream &stream, IEventQueue *events);
  IpcClientProxy(IpcClientProxy const &) = delete;
  IpcClientProxy(IpcClientProxy &&) = delete;
  virtual ~IpcClientProxy();

  IpcClientProxy &operator=(IpcClientProxy const &) = delete;
  IpcClientProxy &operator=(IpcClientProxy &&) = delete;

private:
  void send(const IpcMessage &message);
  void handleData(const Event &, void *);
  void handleDisconnect(const Event &, void *);
  void handleWriteError(const Event &, void *);
  IpcHelloMessage *parseHello();
  IpcCommandMessage *parseCommand();
  IpcSettingMessage *parseSetting() const;
  void disconnect();

private:
  deskflow::IStream &m_stream;
  IEventQueue *m_events;
  IpcClientType m_clientType = IpcClientType::Unknown;
  bool m_disconnecting = false;
  ArchMutex m_readMutex = ARCH->newMutex();
  ArchMutex m_writeMutex = ARCH->newMutex();
};
