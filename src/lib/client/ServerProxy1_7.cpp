/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/ServerProxy1_7.h"

#include "base/Log.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"

#include <cstring>
#include <string>

ServerProxy1_7::ServerProxy1_7(Client *client, deskflow::IStream *stream, IEventQueue *events)
    : ServerProxy(client, stream, events)
{
}

ServerProxy::ConnectionResult ServerProxy1_7::parseMessage(const uint8_t *code)
{
  auto result = ConnectionResult::Okay;
  if (memcmp(code, kMsgDSecureInputNotification, 4) == 0) {
    secureInputNotification();
  } else {
    result = ServerProxy::parseMessage(code);
  }
  return result;
}

void ServerProxy1_7::secureInputNotification() const
{
  std::string app;
  ProtocolUtil::readf(getStream(), kMsgDSecureInputNotification + 4, &app);
  LOG_INFO("application \"%s\" is blocking the keyboard", app.c_str());
}
