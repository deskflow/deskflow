/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy.h"

#include "base/EventQueue.h"
#include "base/Log.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"

//
// ClientProxy
//

ClientProxy::ClientProxy(const std::string &name, deskflow::IStream *stream) : BaseClientProxy(name), m_stream(stream)
{
}

ClientProxy::~ClientProxy()
{
  delete m_stream;
}

void ClientProxy::close(const char *msg)
{
  LOG((CLOG_DEBUG1 "send close \"%s\" to \"%s\"", msg, getName().c_str()));
  ProtocolUtil::writef(getStream(), msg);

  // force the close to be sent before we return
  getStream()->flush();
}

deskflow::IStream *ClientProxy::getStream() const
{
  return m_stream;
}

void *ClientProxy::getEventTarget() const
{
  return static_cast<IScreen *>(const_cast<ClientProxy *>(this));
}
