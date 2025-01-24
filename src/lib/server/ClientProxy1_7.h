/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_6.h"

//! Proxy for client implementing protocol version 1.7
class ClientProxy1_7 : public ClientProxy1_6
{
public:
  ClientProxy1_7(const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events);
  ~ClientProxy1_7() override = default;

  void secureInputNotification(const std::string &app) const override;

private:
  IEventQueue *m_events;
};
