/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy.h"

#include "base/Log.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"

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

void ClientProxy::sendMouserData(const std::string &line)
{
  LOG_VERBOSE("send mouser data to \"%s\": %.96s", getName().c_str(), line.c_str());
  ProtocolUtil::writef(getStream(), kMsgDMouserData, &line);
}

void ClientProxy::sendHidFrame(uint16_t deviceId, const std::string &bytes)
{
  LOG_VERBOSE(
      "send hid frame to \"%s\" (device %u, %d bytes)", getName().c_str(), deviceId, static_cast<int>(bytes.size())
  );
  ProtocolUtil::writef(getStream(), kMsgDHidReport, deviceId, &bytes);
}

void *ClientProxy::getEventTarget() const
{
  return static_cast<IScreen *>(const_cast<ClientProxy *>(this));
}
