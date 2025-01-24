/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_4.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "deskflow/ProtocolUtil.h"
#include "server/Server.h"

#include <cstring>
#include <memory>

//
// ClientProxy1_4
//

ClientProxy1_4::ClientProxy1_4(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_3(name, stream, events),
      m_server(server)
{
  assert(m_server != NULL);
}

ClientProxy1_4::~ClientProxy1_4()
{
}

void ClientProxy1_4::keyDown(KeyID key, KeyModifierMask mask, KeyButton button, const std::string &lang)
{
  ClientProxy1_3::keyDown(key, mask, button, lang);
}

void ClientProxy1_4::keyRepeat(
    KeyID key, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang
)
{
  ClientProxy1_3::keyRepeat(key, mask, count, button, lang);
}

void ClientProxy1_4::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
  ClientProxy1_3::keyUp(key, mask, button);
}

void ClientProxy1_4::keepAlive()
{
  ClientProxy1_3::keepAlive();
}
