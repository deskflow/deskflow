/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_4.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
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
  assert(m_server != nullptr);
}
