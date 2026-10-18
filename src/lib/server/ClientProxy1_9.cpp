/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_9.h"

#include "base/Log.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"

ClientProxy1_9::ClientProxy1_9(
    const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events
)
    : ClientProxy1_8(name, adoptedStream, server, events)
{
}

void ClientProxy1_9::navigationGesture(NavigationActionSlot action)
{
  LOG_VERBOSE("send navigation gesture to \"%s\" action=%d", getName().c_str(), static_cast<int>(action));
  ProtocolUtil::writef(getStream(), kMsgDNavigationGesture, static_cast<int8_t>(action));
}
