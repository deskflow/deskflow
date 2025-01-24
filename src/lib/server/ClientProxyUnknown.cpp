/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxyUnknown.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "deskflow/AppUtil.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/XDeskflow.h"
#include "deskflow/protocol_types.h"
#include "io/IStream.h"
#include "io/XIO.h"
#include "server/ClientProxy1_0.h"
#include "server/ClientProxy1_1.h"
#include "server/ClientProxy1_2.h"
#include "server/ClientProxy1_3.h"
#include "server/ClientProxy1_4.h"
#include "server/ClientProxy1_5.h"
#include "server/ClientProxy1_6.h"
#include "server/ClientProxy1_7.h"
#include "server/ClientProxy1_8.h"
#include "server/Server.h"

#include <iterator>
#include <sstream>

//
// ClientProxyUnknown
//

ClientProxyUnknown::ClientProxyUnknown(deskflow::IStream *stream, double timeout, Server *server, IEventQueue *events)
    : m_stream(stream),
      m_proxy(NULL),
      m_ready(false),
      m_server(server),
      m_events(events)
{
  assert(m_server != NULL);

  m_events->adoptHandler(
      Event::kTimer, this, new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleTimeout, NULL)
  );
  m_timer = m_events->newOneShotTimer(timeout, this);
  addStreamHandlers();

  const auto protocol = m_server->protocolString();
  const auto helloMessage = protocol + kMsgHelloArgs;

  LOG_DEBUG("saying hello as %s, protocol v%d.%d", protocol.c_str(), kProtocolMajorVersion, kProtocolMinorVersion);
  ProtocolUtil::writef(m_stream, helloMessage.c_str(), kProtocolMajorVersion, kProtocolMinorVersion);
}

ClientProxyUnknown::~ClientProxyUnknown()
{
  removeHandlers();
  removeTimer();
  delete m_stream;
  delete m_proxy;
}

ClientProxy *ClientProxyUnknown::orphanClientProxy()
{
  if (m_ready) {
    removeHandlers();
    ClientProxy *proxy = m_proxy;
    m_proxy = NULL;
    return proxy;
  } else {
    return NULL;
  }
}

void ClientProxyUnknown::sendSuccess()
{
  m_ready = true;
  removeTimer();
  m_events->addEvent(Event(m_events->forClientProxyUnknown().success(), this));
}

void ClientProxyUnknown::sendFailure()
{
  delete m_proxy;
  m_proxy = NULL;
  m_ready = false;
  removeHandlers();
  removeTimer();
  m_events->addEvent(Event(m_events->forClientProxyUnknown().failure(), this));
}

void ClientProxyUnknown::addStreamHandlers()
{
  assert(m_stream != NULL);

  m_events->adoptHandler(
      m_events->forIStream().inputReady(), m_stream->getEventTarget(),
      new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleData)
  );
  m_events->adoptHandler(
      m_events->forIStream().outputError(), m_stream->getEventTarget(),
      new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleWriteError)
  );
  m_events->adoptHandler(
      m_events->forIStream().inputShutdown(), m_stream->getEventTarget(),
      new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleDisconnect)
  );
  m_events->adoptHandler(
      m_events->forIStream().inputFormatError(), m_stream->getEventTarget(),
      new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleDisconnect)
  );
  m_events->adoptHandler(
      m_events->forIStream().outputShutdown(), m_stream->getEventTarget(),
      new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleWriteError)
  );
}

void ClientProxyUnknown::addProxyHandlers()
{
  assert(m_proxy != NULL);

  m_events->adoptHandler(
      m_events->forClientProxy().ready(), m_proxy,
      new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleReady)
  );
  m_events->adoptHandler(
      m_events->forClientProxy().disconnected(), m_proxy,
      new TMethodEventJob<ClientProxyUnknown>(this, &ClientProxyUnknown::handleDisconnect)
  );
}

void ClientProxyUnknown::removeHandlers()
{
  if (m_stream != NULL) {
    m_events->removeHandler(m_events->forIStream().inputReady(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIStream().outputError(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIStream().inputShutdown(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIStream().inputFormatError(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIStream().outputShutdown(), m_stream->getEventTarget());
  }
  if (m_proxy != NULL) {
    m_events->removeHandler(m_events->forClientProxy().ready(), m_proxy);
    m_events->removeHandler(m_events->forClientProxy().disconnected(), m_proxy);
  }
}

void ClientProxyUnknown::removeTimer()
{
  if (m_timer != NULL) {
    m_events->deleteTimer(m_timer);
    m_events->removeHandler(Event::kTimer, this);
    m_timer = NULL;
  }
}

void ClientProxyUnknown::initProxy(const std::string &name, int major, int minor)
{
  if (major == 1) {
    switch (minor) {
    case 0:
      m_proxy = new ClientProxy1_0(name, m_stream, m_events);
      break;

    case 1:
      m_proxy = new ClientProxy1_1(name, m_stream, m_events);
      break;

    case 2:
      m_proxy = new ClientProxy1_2(name, m_stream, m_events);
      break;

    case 3:
      m_proxy = new ClientProxy1_3(name, m_stream, m_events);
      break;

    case 4:
      m_proxy = new ClientProxy1_4(name, m_stream, m_server, m_events);
      break;

    case 5:
      m_proxy = new ClientProxy1_5(name, m_stream, m_server, m_events);
      break;

    case 6:
      m_proxy = new ClientProxy1_6(name, m_stream, m_server, m_events);
      break;

    case 7:
      m_proxy = new ClientProxy1_7(name, m_stream, m_server, m_events);
      break;

    case 8:
      m_proxy = new ClientProxy1_8(name, m_stream, m_server, m_events);
      break;
    }
  }

  // hangup (with error) if version isn't supported
  if (m_proxy == NULL) {
    throw XIncompatibleClient(major, minor);
  }
}

void ClientProxyUnknown::handleData(const Event &, void *)
{
  LOG((CLOG_DEBUG1 "parsing hello reply"));

  std::string name("<unknown>");

  try {
    // limit the maximum length of the hello
    uint32_t n = m_stream->getSize();
    if (n > kMaxHelloLength) {
      LOG((CLOG_DEBUG1 "hello reply too long"));
      throw XBadClient();
    }

    // parse the reply to hello
    int16_t major, minor;
    std::string protocolName;
    if (!ProtocolUtil::readf(m_stream, kMsgHelloBack, &protocolName, &major, &minor, &name)) {
      throw XBadClient();
    }

    // disallow invalid version numbers
    if (major <= 0 || minor < 0) {
      throw XIncompatibleClient(major, minor);
    }

    // remove stream event handlers.  the proxy we're about to create
    // may install its own handlers and we don't want to accidentally
    // remove those later.
    removeHandlers();

    // create client proxy for highest version supported by the client
    initProxy(name, major, minor);

    // the proxy is created and now proxy now owns the stream
    LOG((CLOG_DEBUG1 "created proxy for client \"%s\" version %d.%d", name.c_str(), major, minor));
    m_stream = NULL;

    // wait until the proxy signals that it's ready or has disconnected
    addProxyHandlers();
    return;
  } catch (XIncompatibleClient &e) {
    // client is incompatible
    LOG((CLOG_WARN "client \"%s\" has incompatible version %d.%d)", name.c_str(), e.getMajor(), e.getMinor()));
    ProtocolUtil::writef(m_stream, kMsgEIncompatible, kProtocolMajorVersion, kProtocolMinorVersion);
  } catch (XBadClient &) {
    // client not behaving
    LOG((CLOG_WARN "protocol error from client \"%s\"", name.c_str()));
    ProtocolUtil::writef(m_stream, kMsgEBad);
  } catch (XBase &e) {
    // misc error
    LOG((CLOG_WARN "error communicating with client \"%s\": %s", name.c_str(), e.what()));
  }
  sendFailure();
}

void ClientProxyUnknown::handleWriteError(const Event &, void *)
{
  LOG((CLOG_NOTE "error communicating with new client"));
  sendFailure();
}

void ClientProxyUnknown::handleTimeout(const Event &, void *)
{
  LOG((CLOG_NOTE "new client is unresponsive"));
  sendFailure();
}

void ClientProxyUnknown::handleDisconnect(const Event &, void *)
{
  LOG((CLOG_NOTE "new client disconnected"));
  sendFailure();
}

void ClientProxyUnknown::handleReady(const Event &, void *)
{
  sendSuccess();
}
