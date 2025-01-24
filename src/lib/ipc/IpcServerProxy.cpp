/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ipc/IpcServerProxy.h"

#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "common/ipc.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"
#include "ipc/IpcMessage.h"

//
// IpcServerProxy
//

IpcServerProxy::IpcServerProxy(deskflow::IStream &stream, IEventQueue *events) : m_stream(stream), m_events(events)
{
  m_events->adoptHandler(
      m_events->forIStream().inputReady(), stream.getEventTarget(),
      new TMethodEventJob<IpcServerProxy>(this, &IpcServerProxy::handleData)
  );
}

IpcServerProxy::~IpcServerProxy()
{
  m_events->removeHandler(m_events->forIStream().inputReady(), m_stream.getEventTarget());
}

void IpcServerProxy::handleData(const Event &, void *)
{
  LOG((CLOG_DEBUG "start ipc handle data"));

  uint8_t code[4];
  uint32_t n = m_stream.read(code, 4);
  while (n != 0) {

    LOG((CLOG_DEBUG "ipc read: %c%c%c%c", code[0], code[1], code[2], code[3]));

    IpcMessage *m = nullptr;
    if (memcmp(code, kIpcMsgLogLine, 4) == 0) {
      m = parseLogLine();
    } else if (memcmp(code, kIpcMsgShutdown, 4) == 0) {
      m = new IpcShutdownMessage();
    } else {
      LOG((CLOG_ERR "invalid ipc message"));
      disconnect();
    }

    // don't delete with this event; the data is passed to a new event.
    Event e(m_events->forIpcServerProxy().messageReceived(), this, NULL, Event::kDontFreeData);
    e.setDataObject(m);
    m_events->addEvent(e);

    n = m_stream.read(code, 4);
  }

  LOG((CLOG_DEBUG "finished ipc handle data"));
}

void IpcServerProxy::send(const IpcMessage &message)
{
  LOG((CLOG_DEBUG4 "ipc write: %d", message.type()));

  switch (message.type()) {
  case IpcMessageType::Hello: {
    const IpcHelloMessage &hm = static_cast<const IpcHelloMessage &>(message);
    ProtocolUtil::writef(&m_stream, kIpcMsgHello, hm.clientType());
    break;
  }

  case IpcMessageType::Command: {
    const IpcCommandMessage &cm = static_cast<const IpcCommandMessage &>(message);
    const std::string command = cm.command();
    ProtocolUtil::writef(&m_stream, kIpcMsgCommand, &command);
    break;
  }

  default:
    LOG((CLOG_ERR "ipc message not supported: %d", message.type()));
    break;
  }
}

IpcLogLineMessage *IpcServerProxy::parseLogLine()
{
  std::string logLine;
  ProtocolUtil::readf(&m_stream, kIpcMsgLogLine + 4, &logLine);

  // must be deleted by event handler.
  return new IpcLogLineMessage(logLine);
}

void IpcServerProxy::disconnect()
{
  LOG((CLOG_DEBUG "ipc disconnect, closing stream"));
  m_stream.close();
}
