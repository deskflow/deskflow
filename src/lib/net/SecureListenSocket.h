/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/stdset.h"
#include "net/TCPListenSocket.h"

class IEventQueue;
class SocketMultiplexer;
class IDataSocket;

class SecureListenSocket : public TCPListenSocket
{
public:
  SecureListenSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family);

  // IListenSocket overrides
  virtual IDataSocket *accept();
};
