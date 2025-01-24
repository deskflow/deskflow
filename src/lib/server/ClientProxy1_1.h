/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_0.h"

//! Proxy for client implementing protocol version 1.1
class ClientProxy1_1 : public ClientProxy1_0
{
public:
  ClientProxy1_1(const std::string &name, deskflow::IStream *adoptedStream, IEventQueue *events);
  ~ClientProxy1_1();

  // IClient overrides
  virtual void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &);
  virtual void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &);
  virtual void keyUp(KeyID, KeyModifierMask, KeyButton);
};
