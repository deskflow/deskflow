/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "net/IListenSocket.h"

#include <mutex>

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
  TCPListenSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::AddressFamily family);
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

  ISocketMultiplexerJob *serviceListening(ISocketMultiplexerJob *, bool, bool, bool);

protected:
  void setListeningJob();

  IEventQueue *m_events;
  ArchSocket m_socket;
  SocketMultiplexer *m_socketMultiplexer;

private:
  std::mutex m_mutex;
};
