/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ipc/IpcClientProxy.h"

#include "base/Log.h"
#include "base/TMethodEventJob.h"
#include "common/ipc.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcSettingMessage.h"

//
// IpcClientProxy
//

IpcClientProxy::IpcClientProxy(deskflow::IStream &stream, IEventQueue *events) : m_stream(stream), m_events(events)
{
  m_events->adoptHandler(
      m_events->forIStream().inputReady(), stream.getEventTarget(),
      new TMethodEventJob<IpcClientProxy>(this, &IpcClientProxy::handleData)
  );

  m_events->adoptHandler(
      m_events->forIStream().outputError(), stream.getEventTarget(),
      new TMethodEventJob<IpcClientProxy>(this, &IpcClientProxy::handleWriteError)
  );

  m_events->adoptHandler(
      m_events->forIStream().inputShutdown(), stream.getEventTarget(),
      new TMethodEventJob<IpcClientProxy>(this, &IpcClientProxy::handleDisconnect)
  );

  m_events->adoptHandler(
      m_events->forIStream().outputShutdown(), stream.getEventTarget(),
      new TMethodEventJob<IpcClientProxy>(this, &IpcClientProxy::handleWriteError)
  );
}

IpcClientProxy::~IpcClientProxy()
{
  m_events->removeHandler(m_events->forIStream().inputReady(), m_stream.getEventTarget());
  m_events->removeHandler(m_events->forIStream().outputError(), m_stream.getEventTarget());
  m_events->removeHandler(m_events->forIStream().inputShutdown(), m_stream.getEventTarget());
  m_events->removeHandler(m_events->forIStream().outputShutdown(), m_stream.getEventTarget());

  // don't delete the stream while it's being used.
  ARCH->lockMutex(m_readMutex);
  ARCH->lockMutex(m_writeMutex);
  delete &m_stream;
  ARCH->unlockMutex(m_readMutex);
  ARCH->unlockMutex(m_writeMutex);

  ARCH->closeMutex(m_readMutex);
  ARCH->closeMutex(m_writeMutex);
}

void IpcClientProxy::handleDisconnect(const Event &, void *)
{
  disconnect();
  LOG((CLOG_DEBUG "ipc client disconnected"));
}

void IpcClientProxy::handleWriteError(const Event &, void *)
{
  disconnect();
  LOG((CLOG_DEBUG "ipc client write error"));
}

void IpcClientProxy::handleData(const Event &, void *)
{
  // don't allow the dtor to destroy the stream while we're using it.
  ArchMutexLock lock(m_readMutex);

  LOG((CLOG_DEBUG "start ipc handle data"));

  uint8_t code[4];
  uint32_t n = m_stream.read(code, 4);
  while (n != 0) {

    LOG((CLOG_DEBUG "ipc read: %c%c%c%c", code[0], code[1], code[2], code[3]));

    IpcMessage *m = nullptr;
    if (memcmp(code, kIpcMsgHello, 4) == 0) {
      m = parseHello();
    } else if (memcmp(code, kIpcMsgCommand, 4) == 0) {
      m = parseCommand();
    } else if (memcmp(code, kIpcMsgSetting, 4) == 0) {
      m = parseSetting();
    } else {
      LOG((CLOG_ERR "invalid ipc message"));
      disconnect();
    }

    // don't delete with this event; the data is passed to a new event.
    Event e(m_events->forIpcClientProxy().messageReceived(), this, NULL, Event::kDontFreeData);
    e.setDataObject(m);
    m_events->addEvent(e);

    n = m_stream.read(code, 4);
  }

  LOG((CLOG_DEBUG "finished ipc handle data"));
}

void IpcClientProxy::send(const IpcMessage &message)
{
  // don't allow other threads to write until we've finished the entire
  // message. stream write is locked, but only for that single write.
  // also, don't allow the dtor to destroy the stream while we're using it.
  ArchMutexLock lock(m_writeMutex);

  LOG((CLOG_DEBUG4 "ipc write: %d", message.type()));

  switch (message.type()) {
  case IpcMessageType::LogLine: {
    const IpcLogLineMessage &llm = static_cast<const IpcLogLineMessage &>(message);
    const std::string logLine = llm.logLine();
    ProtocolUtil::writef(&m_stream, kIpcMsgLogLine, &logLine);
    break;
  }

  case IpcMessageType::Shutdown:
    ProtocolUtil::writef(&m_stream, kIpcMsgShutdown);
    break;

  case IpcMessageType::HelloBack:
    ProtocolUtil::writef(&m_stream, kIpcMsgHelloBack);
    break;

  default:
    LOG((CLOG_ERR "ipc message not supported: %d", message.type()));
    break;
  }
}

IpcHelloMessage *IpcClientProxy::parseHello()
{
  uint8_t type;
  ProtocolUtil::readf(&m_stream, kIpcMsgHello + 4, &type);

  m_clientType = static_cast<IpcClientType>(type);

  // must be deleted by event handler.
  return new IpcHelloMessage(m_clientType);
}

IpcCommandMessage *IpcClientProxy::parseCommand()
{
  std::string command;
  uint8_t elevate;
  ProtocolUtil::readf(&m_stream, kIpcMsgCommand + 4, &command, &elevate);

  // must be deleted by event handler.
  return new IpcCommandMessage(command, elevate != 0);
}

IpcSettingMessage *IpcClientProxy::parseSetting() const
{
  std::string name;
  std::string value;

  ProtocolUtil::readf(&m_stream, kIpcMsgSetting + 4, &name, &value);

  // must be deleted by event handler.
  return new IpcSettingMessage(name, value);
}

void IpcClientProxy::disconnect()
{
  LOG((CLOG_DEBUG "ipc disconnect, closing stream"));
  m_disconnecting = true;
  m_stream.close();
  m_events->addEvent(Event(m_events->forIpcClientProxy().disconnected(), this));
}
