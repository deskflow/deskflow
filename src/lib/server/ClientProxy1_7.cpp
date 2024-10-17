/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

ClientProxy1_7::ClientProxy1_7(const String &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_6(name, stream, server, events),
      m_events(events)
{
}

void ClientProxy1_7::secureInputNotification(const String &app) const
{
  LOG((CLOG_DEBUG2 "send secure input notification to \"%s\" %s", getName().c_str(), app.c_str()));
  ProtocolUtil::writef(getStream(), kMsgDSecureInputNotification, &app);
}
