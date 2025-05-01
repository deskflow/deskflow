/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_3.h"

class Server;

//! Proxy for client implementing protocol version 1.4
class ClientProxy1_4 : public ClientProxy1_3
{
public:
  ClientProxy1_4(const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events);
  ~ClientProxy1_4() override = default;

  //! @name accessors
  //@{

  //! get server pointer
  Server *getServer()
  {
    return m_server;
  }

  //@}

  // IClient overrides
  void keyDown(KeyID key, KeyModifierMask mask, KeyButton button, const std::string &) override;
  void keyRepeat(KeyID key, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override;
  void keyUp(KeyID key, KeyModifierMask mask, KeyButton button) override;
  void keepAlive() override;

  Server *m_server;
};
