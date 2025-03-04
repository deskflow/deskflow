/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "net/ISocketFactory.h"

class IEventQueue;
class SocketMultiplexer;

//! Socket factory for TCP sockets
class TCPSocketFactory : public ISocketFactory
{
public:
  TCPSocketFactory(IEventQueue *events, SocketMultiplexer *socketMultiplexer);
  virtual ~TCPSocketFactory();

  // ISocketFactory overrides
  virtual IDataSocket *create(
      IArchNetwork::EAddressFamily family = IArchNetwork::kINET, SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const;
  virtual IListenSocket *createListen(
      IArchNetwork::EAddressFamily family = IArchNetwork::kINET, SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const;

private:
  IEventQueue *m_events;
  SocketMultiplexer *m_socketMultiplexer;
};
