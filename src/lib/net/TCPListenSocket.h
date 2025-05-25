/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "net/IListenSocket.h"

class Mutex;
class ISocketMultiplexerJob;
class IEventQueue;
class SocketMultiplexer;

//! TCP listen socket
/*!
A listen socket using TCP.
*/
class TCPListenSocket : public IListenSocket
{
public:
  TCPListenSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family);
  TCPListenSocket(TCPListenSocket const &) = delete;
  TCPListenSocket(TCPListenSocket &&) = delete;
  ~TCPListenSocket() override;

  TCPListenSocket &operator=(TCPListenSocket const &) = delete;
  TCPListenSocket &operator=(TCPListenSocket &&) = delete;

  // ISocket overrides
  void bind(const NetworkAddress &) override;
  void close() override;
  void *getEventTarget() const override;

  // IListenSocket overrides
  std::unique_ptr<IDataSocket> accept() override;

protected:
  void setListeningJob();

public:
  ISocketMultiplexerJob *serviceListening(ISocketMultiplexerJob *, bool, bool, bool);

protected:
  ArchSocket m_socket;
  Mutex *m_mutex = nullptr;
  IEventQueue *m_events;
  SocketMultiplexer *m_socketMultiplexer;
};
