/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/SecurityLevel.h"
#include "net/TCPListenSocket.h"

#include <set>

class IEventQueue;
class SocketMultiplexer;
class IDataSocket;

class SecureListenSocket : public TCPListenSocket
{
public:
  SecureListenSocket(
      IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family,
      SecurityLevel securityLevel = SecurityLevel::PlainText
  );

  // IListenSocket overrides
  std::unique_ptr<IDataSocket> accept() override;

private:
  const SecurityLevel m_securityLevel;
};
