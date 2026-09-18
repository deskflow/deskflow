/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "client/ServerProxy.h"

//! Proxy for server implementing protocol version 1.7
class ServerProxy1_7 : public ServerProxy
{
public:
  ServerProxy1_7(Client *client, deskflow::IStream *stream, IEventQueue *events);
  ~ServerProxy1_7() override = default;

protected:
  ConnectionResult parseMessage(const uint8_t *code) override;

private:
  void secureInputNotification() const;
};
