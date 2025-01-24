/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include "net/ISocketFactory.h"

class IEventQueue;
class SocketMultiplexer;

class InverseSocketFactory : public ISocketFactory
{
public:
  InverseSocketFactory(IEventQueue *events, SocketMultiplexer *socketMultiplexer);

  // ISocketFactory overrides
  IDataSocket *create(bool secure, IArchNetwork::EAddressFamily family = IArchNetwork::kINET) const override;
  IListenSocket *createListen(bool secure, IArchNetwork::EAddressFamily family = IArchNetwork::kINET) const override;

private:
  IEventQueue *m_events = nullptr;
  SocketMultiplexer *m_socketMultiplexer = nullptr;
};
