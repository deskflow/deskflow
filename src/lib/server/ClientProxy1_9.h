/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_8.h"

// Experimental input-release result, negotiated separately from legacy 1.8.
class ClientProxy1_9 : public ClientProxy1_8
{
public:
  ClientProxy1_9(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events);
  bool supportsInputRelease() const override
  {
    return true;
  }
  void requestInputRelease(uint64_t generation, uint32_t buttons) override;

protected:
  bool parseMessage(const uint8_t *code) override;

private:
  IEventQueue *m_events;
  uint64_t m_pendingRelease = 0;
};
