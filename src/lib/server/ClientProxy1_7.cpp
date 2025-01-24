/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_7.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "deskflow/AppUtil.h"
#include "deskflow/ProtocolUtil.h"
#include "server/Server.h"

//
// ClientProxy1_7
//

ClientProxy1_7::ClientProxy1_7(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_6(name, stream, server, events),
      m_events(events)
{
}

void ClientProxy1_7::secureInputNotification(const std::string &app) const
{
  LOG((CLOG_DEBUG2 "send secure input notification to \"%s\" %s", getName().c_str(), app.c_str()));
  ProtocolUtil::writef(getStream(), kMsgDSecureInputNotification, &app);
}
