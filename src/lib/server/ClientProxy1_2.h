/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_1.h"

class IEventQueue;

//! Proxy for client implementing protocol version 1.2
class ClientProxy1_2 : public ClientProxy1_1
{
public:
  ClientProxy1_2(const std::string &name, deskflow::IStream *adoptedStream, IEventQueue *events);
  ~ClientProxy1_2();

  // IClient overrides
  virtual void mouseRelativeMove(int32_t xRel, int32_t yRel);
};
