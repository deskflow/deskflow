/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy.h"

#include "base/Log.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"
#include "io/StreamFilter.h"
#include "net/SecureSocket.h"

//
// ClientProxy
//

ClientProxy::ClientProxy(const std::string &name, deskflow::IStream *stream) : BaseClientProxy(name), m_stream(stream)
{
  // do nothing
}

ClientProxy::~ClientProxy()
{
  delete m_stream;
}

void ClientProxy::close(const char *msg) const
{
  LOG_VERBOSE("send close \"%s\" to \"%s\"", msg, getName().c_str());
  ProtocolUtil::writef(getStream(), msg);

  // force the close to be sent before we return
  getStream()->flush();
}

deskflow::IStream *ClientProxy::getStream() const
{
  return m_stream;
}

QString ClientProxy::peerIdentityReference() const
{
  auto *stream = m_stream;
  while (auto *filter = dynamic_cast<StreamFilter *>(stream))
    stream = filter->getStream();
  const auto *secure = dynamic_cast<SecureSocket *>(stream);
  return secure ? secure->peerIdentityReference() : QString();
}

void *ClientProxy::getEventTarget() const
{
  return static_cast<IComputer *>(const_cast<ClientProxy *>(this));
}
