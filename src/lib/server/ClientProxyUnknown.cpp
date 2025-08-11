/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxyUnknown.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "deskflow/AppUtil.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IOException.h"
#include "io/IStream.h"
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
      m_server(server),
      m_events(events)
{
  assert(m_server != nullptr);

  m_events->addHandler(EventTypes::Timer, this, [this](const auto &) { handleTimeout(); });
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
    m_proxy = nullptr;
    return proxy;
  } else {
    return nullptr;
  }
}

void ClientProxyUnknown::sendSuccess()
{
  m_ready = true;
  removeTimer();
  m_events->addEvent(Event(EventTypes::ClientProxyUnknownSuccess, this));
}

void ClientProxyUnknown::sendFailure()
{
  delete m_proxy;
  m_proxy = nullptr;
  m_ready = false;
  removeHandlers();
  removeTimer();
  m_events->addEvent(Event(EventTypes::ClientProxyUnknownFailure, this));
}

void ClientProxyUnknown::addStreamHandlers()
{
  assert(m_stream != nullptr);
  m_events->addHandler(EventTypes::StreamInputReady, m_stream->getEventTarget(), [this](const auto &) {
    handleData();
  });
  m_events->addHandler(EventTypes::StreamOutputError, m_stream->getEventTarget(), [this](const auto &) {
    handleWriteError();
  });
  m_events->addHandler(EventTypes::StreamInputShutdown, m_stream->getEventTarget(), [this](const auto &) {
    handleDisconnect();
  });
  m_events->addHandler(EventTypes::StreamInputFormatError, m_stream->getEventTarget(), [this](const auto &) {
    handleDisconnect();
  });
  m_events->addHandler(EventTypes::StreamOutputShutdown, m_stream->getEventTarget(), [this](const auto &) {
    handleWriteError();
  });
}

void ClientProxyUnknown::addProxyHandlers()
{
  assert(m_proxy != nullptr);

  m_events->addHandler(EventTypes::ClientProxyReady, m_proxy, [this](const auto &) { sendSuccess(); });
  m_events->addHandler(EventTypes::ClientProxyDisconnected, m_proxy, [this](const auto &) { handleDisconnect(); });
}

void ClientProxyUnknown::removeHandlers()
{
  using enum EventTypes;
  if (m_stream != nullptr) {
    m_events->removeHandler(StreamInputReady, m_stream->getEventTarget());
    m_events->removeHandler(StreamOutputError, m_stream->getEventTarget());
    m_events->removeHandler(StreamInputShutdown, m_stream->getEventTarget());
    m_events->removeHandler(StreamInputFormatError, m_stream->getEventTarget());
    m_events->removeHandler(StreamOutputShutdown, m_stream->getEventTarget());
  }
  if (m_proxy != nullptr) {
    m_events->removeHandler(ClientProxyReady, m_proxy);
    m_events->removeHandler(ClientProxyDisconnected, m_proxy);
  }
}

void ClientProxyUnknown::removeTimer()
{
  if (m_timer != nullptr) {
    m_events->deleteTimer(m_timer);
    m_events->removeHandler(EventTypes::Timer, this);
    m_timer = nullptr;
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

    default:
      break;
    }
  }

  // hangup (with error) if version isn't supported
  if (m_proxy == nullptr) {
    throw IncompatibleClientException(major, minor);
  }
}

void ClientProxyUnknown::handleData()
{
  LOG_DEBUG1("parsing hello reply");

  std::string name("<unknown>");

  try {
    // limit the maximum length of the hello
    if (uint32_t n = m_stream->getSize(); n > kMaxHelloLength) {
      LOG_DEBUG1("hello reply too long");
      throw BadClientException();
    }

    // parse the reply to hello
    int16_t major;
    int16_t minor;
    if (std::string protocolName; !ProtocolUtil::readf(m_stream, kMsgHelloBack, &protocolName, &major, &minor, &name)) {
      throw BadClientException();
    }

    // disallow invalid version numbers
    if (major <= 0 || minor < 0) {
      throw IncompatibleClientException(major, minor);
    }

    // remove stream event handlers.  the proxy we're about to create
    // may install its own handlers and we don't want to accidentally
    // remove those later.
    removeHandlers();

    // create client proxy for highest version supported by the client
    initProxy(name, major, minor);

    // the proxy is created and now proxy now owns the stream
    LOG_DEBUG1("created proxy for client \"%s\" version %d.%d", name.c_str(), major, minor);
    m_stream = nullptr;

    // wait until the proxy signals that it's ready or has disconnected
    addProxyHandlers();
    return;
  } catch (IncompatibleClientException &e) {
    // client is incompatible
    LOG_WARN("client \"%s\" has incompatible version %d.%d)", name.c_str(), e.getMajor(), e.getMinor());
    ProtocolUtil::writef(m_stream, kMsgEIncompatible, kProtocolMajorVersion, kProtocolMinorVersion);
  } catch (BadClientException &) {
    // client not behaving
    LOG_WARN("protocol error from client \"%s\"", name.c_str());
    ProtocolUtil::writef(m_stream, kMsgEBad);
  } catch (BaseException &e) {
    // misc error
    LOG_WARN("error communicating with client \"%s\": %s", name.c_str(), e.what());
  }
  sendFailure();
}

void ClientProxyUnknown::handleWriteError()
{
  LOG_NOTE("error communicating with new client");
  sendFailure();
}

void ClientProxyUnknown::handleTimeout()
{
  LOG_NOTE("new client is unresponsive");
  sendFailure();
}

void ClientProxyUnknown::handleDisconnect()
{
  LOG_NOTE("new client disconnected");
  sendFailure();
}
