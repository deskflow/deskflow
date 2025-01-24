/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2022 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "AutoArchSocket.h"
#include "arch/IArchNetwork.h"
#include "mt/Mutex.h"
#include "net/IListenSocket.h"

class ISocketMultiplexerJob;
class IEventQueue;
class SocketMultiplexer;

class InverseServerSocket : public IListenSocket
{
public:
  InverseServerSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family);
  InverseServerSocket(InverseServerSocket const &) = delete;
  InverseServerSocket(InverseServerSocket &&) = delete;
  ~InverseServerSocket() override;

  InverseServerSocket &operator=(InverseServerSocket const &) = delete;
  InverseServerSocket &operator=(InverseServerSocket &&) = delete;

  // ISocket overrides
  void bind(const NetworkAddress &) override;
  void close() override;
  void *getEventTarget() const override;

  // IListenSocket overrides
  IDataSocket *accept() override;

protected:
  void setListeningJob(bool read = false);

public:
  ISocketMultiplexerJob *serviceListening(ISocketMultiplexerJob *, bool, bool, bool);

protected:
  AutoArchSocket m_socket;
  Mutex m_mutex;
  IEventQueue *m_events;
  SocketMultiplexer *m_socketMultiplexer;

private:
  NetworkAddress m_address;
};
