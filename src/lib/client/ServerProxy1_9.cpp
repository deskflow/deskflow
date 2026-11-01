/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ServerProxy1_9.h"

#include "base/Log.h"
#include "client/Client.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"

#include <cstring>

ServerProxy1_9::ServerProxy1_9(Client *client, deskflow::IStream *stream, IEventQueue *events)
    : ServerProxy1_8(client, stream, events)
{
}

ServerProxy::ConnectionResult ServerProxy1_9::parseMessage(const uint8_t *code)
{
  if (memcmp(code, kMsgDTrackpadEvent, 4) == 0) {
    trackpadEvent();
    return ConnectionResult::Okay;
  }
  return ServerProxy1_8::parseMessage(code);
}

void ServerProxy1_9::trackpadEvent()
{
  std::string bytes;
  if (!ProtocolUtil::readf(getStream(), kMsgDTrackpadEvent + 4, &bytes)) {
    return;
  }
  LOG_VERBOSE("recv trackpad event, %zu bytes", bytes.size());
  getClient()->trackpadEvent(bytes);
}
