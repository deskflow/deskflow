/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2006 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_3.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "deskflow/ProtocolUtil.h"

#include <cstring>
#include <memory>

//
// ClientProxy1_3
//

ClientProxy1_3::ClientProxy1_3(const std::string &name, deskflow::IStream *stream, IEventQueue *events)
    : ClientProxy1_2(name, stream, events),
      m_keepAliveRate(kKeepAliveRate),
      m_keepAliveTimer(NULL),
      m_events(events)
{
  setHeartbeatRate(kKeepAliveRate, kKeepAliveRate * kKeepAlivesUntilDeath);
}

ClientProxy1_3::~ClientProxy1_3()
{
  // cannot do this in superclass or our override wouldn't get called
  removeHeartbeatTimer();
}

void ClientProxy1_3::mouseWheel(int32_t xDelta, int32_t yDelta)
{
  LOG((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d,%+d", getName().c_str(), xDelta, yDelta));
  ProtocolUtil::writef(getStream(), kMsgDMouseWheel, xDelta, yDelta);
}

bool ClientProxy1_3::parseMessage(const uint8_t *code)
{
  // process message
  if (memcmp(code, kMsgCKeepAlive, 4) == 0) {
    // reset alarm
    resetHeartbeatTimer();
    return true;
  } else {
    return ClientProxy1_2::parseMessage(code);
  }
}

void ClientProxy1_3::resetHeartbeatRate()
{
  setHeartbeatRate(kKeepAliveRate, kKeepAliveRate * kKeepAlivesUntilDeath);
}

void ClientProxy1_3::setHeartbeatRate(double rate, double)
{
  m_keepAliveRate = rate;
  ClientProxy1_2::setHeartbeatRate(rate, rate * kKeepAlivesUntilDeath);
}

void ClientProxy1_3::resetHeartbeatTimer()
{
  // reset the alarm but not the keep alive timer
  ClientProxy1_2::removeHeartbeatTimer();
  ClientProxy1_2::addHeartbeatTimer();
}

void ClientProxy1_3::addHeartbeatTimer()
{
  // create and install a timer to periodically send keep alives
  if (m_keepAliveRate > 0.0) {
    m_keepAliveTimer = m_events->newTimer(m_keepAliveRate, NULL);
    m_events->adoptHandler(
        Event::kTimer, m_keepAliveTimer,
        new TMethodEventJob<ClientProxy1_3>(this, &ClientProxy1_3::handleKeepAlive, NULL)
    );
  }

  // superclass does the alarm
  ClientProxy1_2::addHeartbeatTimer();
}

void ClientProxy1_3::removeHeartbeatTimer()
{
  // remove the timer that sends keep alives periodically
  if (m_keepAliveTimer != NULL) {
    m_events->removeHandler(Event::kTimer, m_keepAliveTimer);
    m_events->deleteTimer(m_keepAliveTimer);
    m_keepAliveTimer = NULL;
  }

  // superclass does the alarm
  ClientProxy1_2::removeHeartbeatTimer();
}

void ClientProxy1_3::handleKeepAlive(const Event &, void *)
{
  keepAlive();
}

void ClientProxy1_3::keepAlive()
{
  ProtocolUtil::writef(getStream(), kMsgCKeepAlive);
}
