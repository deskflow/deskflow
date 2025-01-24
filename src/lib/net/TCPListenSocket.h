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
  virtual ~TCPListenSocket();

  TCPListenSocket &operator=(TCPListenSocket const &) = delete;
  TCPListenSocket &operator=(TCPListenSocket &&) = delete;

  // ISocket overrides
  virtual void bind(const NetworkAddress &);
  virtual void close();
  virtual void *getEventTarget() const;

  // IListenSocket overrides
  virtual IDataSocket *accept();

protected:
  void setListeningJob();

public:
  ISocketMultiplexerJob *serviceListening(ISocketMultiplexerJob *, bool, bool, bool);

protected:
  ArchSocket m_socket;
  Mutex *m_mutex;
  IEventQueue *m_events;
  SocketMultiplexer *m_socketMultiplexer;
};
