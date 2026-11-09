/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "server/ClientProxy1_7.h"

class ClientProxy1_8 : public ClientProxy1_7
{
public:
  ClientProxy1_8(
      const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events,
      bool monitors = false
  );
  ~ClientProxy1_8() override = default;

  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override;
  void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &) override;

  const deskflow::display::Topology *monitorTopology() const override
  {
    return m_topology ? &*m_topology : nullptr;
  }
  bool supportsMonitorTopology() const override
  {
    return m_monitorSupport;
  }
  void updateServerTopology() override;

protected:
  bool parseHandshakeMessage(const uint8_t *) override;
  bool parseMessage(const uint8_t *) override;

private:
  bool recvTopology();
  bool m_monitorSupport = false;
  std::optional<deskflow::display::Topology> m_topology;
  void synchronizeLanguages() const;
};
