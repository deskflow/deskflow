/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Deskflow Developers
 * Copyright (C) 2012-2022 Symless Ltd.
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
#include "net/ISocketFactory.h"

class IEventQueue;
class SocketMultiplexer;

class InverseSocketFactory : public ISocketFactory
{
public:
  InverseSocketFactory(IEventQueue *events, SocketMultiplexer *socketMultiplexer);

  // ISocketFactory overrides
  IDataSocket *create(
      IArchNetwork::EAddressFamily family = IArchNetwork::kINET, SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const override;
  IListenSocket *createListen(
      IArchNetwork::EAddressFamily family = IArchNetwork::kINET, SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const override;

private:
  IEventQueue *m_events = nullptr;
  SocketMultiplexer *m_socketMultiplexer = nullptr;
};
