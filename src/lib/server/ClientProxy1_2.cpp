/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_2.h"

#include "base/Log.h"
#include "deskflow/ProtocolUtil.h"

//
// ClientProxy1_1
//

ClientProxy1_2::ClientProxy1_2(const std::string &name, deskflow::IStream *stream, IEventQueue *events)
    : ClientProxy1_1(name, stream, events)
{
  // do nothing
}

ClientProxy1_2::~ClientProxy1_2()
{
  // do nothing
}

void ClientProxy1_2::mouseRelativeMove(int32_t xRel, int32_t yRel)
{
  LOG((CLOG_DEBUG2 "send mouse relative move to \"%s\" %d,%d", getName().c_str(), xRel, yRel));
  ProtocolUtil::writef(getStream(), kMsgDMouseRelMove, xRel, yRel);
}
