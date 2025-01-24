/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include "InverseServerSocket.h"

class SecureServerSocket : public InverseServerSocket
{
public:
  SecureServerSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family);

  // IListenSocket overrides
  IDataSocket *accept() override;

private:
  std::string getCertificateFileName() const;
};
