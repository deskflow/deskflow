/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_9.h"
#include "base/IEventQueue.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/SharedInputLockEvent.h"

#include <cstring>

ClientProxy1_9::ClientProxy1_9(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_8(name, stream, server, events),
      m_events(events)
{
}

void ClientProxy1_9::requestInputRelease(uint64_t generation, uint32_t buttons)
{
  m_pendingRelease = generation;
  ProtocolUtil::writef(getStream(), kMsgCInputRelease, uint32_t(generation >> 32), uint32_t(generation), buttons);
}

bool ClientProxy1_9::parseMessage(const uint8_t *code)
{
  if (memcmp(code, kMsgDInputReleased, 4) != 0) {
    return ClientProxy1_8::parseMessage(code);
  }
  uint32_t high = 0, low = 0;
  uint8_t success = 0;
  if (!ProtocolUtil::readf(getStream(), kMsgDInputReleased + 4, &high, &low, &success) || success > 1) {
    return false;
  }
  const auto generation = (uint64_t(high) << 32) | low;
  if (generation != 0 && generation == m_pendingRelease) {
    m_pendingRelease = 0;
    m_events->addEvent(Event(
        EventTypes::SharedInputLockRemoteReady, getEventTarget(),
        new deskflow::SharedInputLockEvent(generation, success == 1)
    ));
  }
  return true;
}
