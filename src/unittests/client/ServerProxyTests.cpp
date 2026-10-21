/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerProxyTests.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "client/Client.h"
#include "client/ServerProxy.h"
#include "deskflow/AppUtil.h"
#include "deskflow/Clipboard.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ipc/CoreIpcServer.h"
#include "io/IStream.h"

#include <QTest>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace {

class TestAppUtil : public AppUtil
{
public:
  int run() override
  {
    return 0;
  }

  std::vector<std::string> getKeyboardLayoutList() override
  {
    return {"en"};
  }

  std::string getCurrentLanguageCode() override
  {
    return "en";
  }
};

class FakeStream : public deskflow::IStream
{
public:
  void push(const std::string &bytes)
  {
    m_chunks.push_back(bytes);
  }

  void close() override
  {
    m_chunks.clear();
    m_inputShutdown = true;
  }

  uint32_t read(void *buffer, uint32_t size) override
  {
    if (m_inputShutdown || m_chunks.empty() || size == 0) {
      return 0;
    }

    auto &front = m_chunks.front();
    const size_t bytesToRead = std::min(static_cast<size_t>(size), front.size());
    if (buffer != nullptr) {
      std::memcpy(buffer, front.data(), bytesToRead);
    }

    front.erase(0, bytesToRead);
    if (front.empty()) {
      m_chunks.pop_front();
    }
    return static_cast<uint32_t>(bytesToRead);
  }

  void write(const void *buffer, uint32_t size) override
  {
    m_written.emplace_back(static_cast<const char *>(buffer), size);
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
    close();
  }

  void shutdownOutput() override
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<FakeStream *>(this);
  }

  bool isReady() const override
  {
    return !m_inputShutdown && !m_chunks.empty();
  }

  uint32_t getSize() const override
  {
    size_t total = 0;
    for (const auto &chunk : m_chunks) {
      total += chunk.size();
    }
    return static_cast<uint32_t>(std::min<size_t>(total, UINT32_MAX));
  }

  const std::vector<std::string> &written() const
  {
    return m_written;
  }

private:
  std::deque<std::string> m_chunks;
  std::vector<std::string> m_written;
  bool m_inputShutdown = false;
};

class RecordingEventQueue : public IEventQueue
{
public:
  ~RecordingEventQueue() override
  {
    for (const auto &event : m_addedEvents) {
      Event::deleteData(event);
    }
  }

  int loop() override
  {
    return 0;
  }

  void adoptBuffer(IEventQueueBuffer *) override
  {
  }

  bool getEvent(Event &, double = -1.0) override
  {
    return false;
  }

  bool dispatchEvent(const Event &event) override
  {
    const auto handler = m_handlers.find(HandlerKey{event.getType(), event.getTarget()});
    if (handler == m_handlers.end()) {
      return false;
    }

    const auto callback = handler->second;
    callback(event);
    return true;
  }

  void addEvent(Event &&event) override
  {
    m_addedEvents.emplace_back(std::move(event));
  }

  EventQueueTimer *newTimer(double, void *) override
  {
    return timer();
  }

  EventQueueTimer *newOneShotTimer(double, void *) override
  {
    return timer();
  }

  void deleteTimer(EventQueueTimer *) override
  {
  }

  void addHandler(EventTypes type, void *target, const EventHandler &handler) override
  {
    m_handlers[HandlerKey{type, target}] = handler;
  }

  void removeHandler(EventTypes type, void *target) override
  {
    m_handlers.erase(HandlerKey{type, target});
  }

  void removeHandlers(void *target) override
  {
    for (auto it = m_handlers.begin(); it != m_handlers.end();) {
      if (it->first.target == target) {
        it = m_handlers.erase(it);
      } else {
        ++it;
      }
    }
  }

  void waitForReady() const override
  {
  }

  void *getSystemTarget() override
  {
    return this;
  }

  EventQueueTimer *timer()
  {
    return reinterpret_cast<EventQueueTimer *>(&m_timerStorage);
  }

  const std::vector<Event> &addedEvents() const
  {
    return m_addedEvents;
  }

private:
  struct HandlerKey
  {
    EventTypes type;
    void *target;

    bool operator<(const HandlerKey &other) const
    {
      if (type != other.type) {
        return static_cast<uint32_t>(type) < static_cast<uint32_t>(other.type);
      }
      return std::less<void *>{}(target, other.target);
    }
  };

  int m_timerStorage = 0;
  std::map<HandlerKey, EventHandler> m_handlers;
  std::vector<Event> m_addedEvents;
};

class TestServerProxy : public ServerProxy
{
public:
  using ServerProxy::ServerProxy;

  bool parseHandshakeMessageReturnsDisconnect(const uint8_t *code)
  {
    return parseHandshakeMessage(code) == ConnectionResult::Disconnect;
  }
};

Client *undereferenceableClient()
{
  // These paths must queue cleanup without calling through to Client.
  return reinterpret_cast<Client *>(0x1);
}

void fillWithText(Clipboard &clipboard, size_t size)
{
  clipboard.open(0);
  clipboard.empty();
  clipboard.add(IClipboard::Format::Text, std::string(size, 'a'));
  clipboard.close();
}

uint8_t clipboardChunkType(const std::string &message)
{
  // the chunk type follows the 4-byte message code, 1-byte clipboard id and 4-byte sequence number
  return static_cast<uint8_t>(message.at(9));
}

ClipboardID clipboardChunkId(const std::string &message)
{
  return static_cast<ClipboardID>(message.at(4));
}

TestAppUtil &testAppUtil()
{
  static TestAppUtil util;
  return util;
}

const Client::DisconnectRequest *disconnectRequest(const RecordingEventQueue &events)
{
  if (events.addedEvents().size() != 1) {
    return nullptr;
  }
  return static_cast<const Client::DisconnectRequest *>(events.addedEvents().front().getDataObject());
}

} // namespace

void ServerProxyTests::initTestCase()
{
  (void)testAppUtil();
  m_log.setFilter(LogLevel::Level::Debug);

  // clipboard sends report to the GUI over IPC, which needs a server instance; it never listens
  new deskflow::core::ipc::CoreIpcServer(this);
}

void ServerProxyTests::handleKeepAliveAlarm_timeout_queuesDisconnectRequest()
{
  RecordingEventQueue events;
  FakeStream stream;
  ServerProxy proxy(undereferenceableClient(), &stream, &events);

  QVERIFY(events.dispatchEvent(Event(EventTypes::Timer, events.timer())));

  QCOMPARE(events.addedEvents().size(), static_cast<size_t>(1));
  const auto &event = events.addedEvents().front();
  QVERIFY(event.getType() == EventTypes::ClientDisconnectRequested);
  QCOMPARE(event.getTarget(), stream.getEventTarget());

  const auto *request = disconnectRequest(events);
  QVERIFY(request != nullptr);
  QVERIFY(request->kind() == Client::DisconnectRequest::Kind::Disconnect);
  QCOMPARE(QString::fromUtf8(request->message()), QStringLiteral("server is not responding"));
}

void ServerProxyTests::handleData_incompleteMessage_queuesDisconnectRequest()
{
  RecordingEventQueue events;
  FakeStream stream;
  stream.push("DF");
  ServerProxy proxy(undereferenceableClient(), &stream, &events);

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamInputReady, stream.getEventTarget())));

  QCOMPARE(events.addedEvents().size(), static_cast<size_t>(1));
  const auto &event = events.addedEvents().front();
  QVERIFY(event.getType() == EventTypes::ClientDisconnectRequested);
  QCOMPARE(event.getTarget(), stream.getEventTarget());

  const auto *request = disconnectRequest(events);
  QVERIFY(request != nullptr);
  QVERIFY(request->kind() == Client::DisconnectRequest::Kind::Disconnect);
  QCOMPARE(QString::fromUtf8(request->message()), QStringLiteral("incomplete message from server"));
}

void ServerProxyTests::parseHandshakeMessage_protocolError_queuesRefusalRequest()
{
  RecordingEventQueue events;
  FakeStream stream;
  TestServerProxy proxy(undereferenceableClient(), &stream, &events);

  QVERIFY(proxy.parseHandshakeMessageReturnsDisconnect(reinterpret_cast<const uint8_t *>(kMsgEBad)));
  QCOMPARE(events.addedEvents().size(), static_cast<size_t>(1));
  const auto *request = disconnectRequest(events);
  QVERIFY(request != nullptr);
  QVERIFY(request->kind() == Client::DisconnectRequest::Kind::Refuse);
  QVERIFY(request->refusalReason() == deskflow::core::ConnectionRefusal::ProtocolError);
  QCOMPARE(QString::fromUtf8(request->message()), QStringLiteral("server reported a protocol error"));
}

void ServerProxyTests::onClipboardChanged_largeClipboard_sendsOneChunkPerFlush()
{
  RecordingEventQueue events;
  FakeStream stream;
  ServerProxy proxy(undereferenceableClient(), &stream, &events);
  Clipboard clipboard;
  fillWithText(clipboard, 150 * 1024);

  proxy.onClipboardChanged(kClipboardClipboard, &clipboard);

  QCOMPARE(stream.written().size(), static_cast<size_t>(2));
  QCOMPARE(clipboardChunkType(stream.written().at(0)), ChunkType::DataStart);
  QCOMPARE(clipboardChunkType(stream.written().at(1)), ChunkType::DataChunk);

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QCOMPARE(stream.written().size(), static_cast<size_t>(3));
  QCOMPARE(clipboardChunkType(stream.written().at(2)), ChunkType::DataChunk);

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QCOMPARE(stream.written().size(), static_cast<size_t>(4));
  QCOMPARE(clipboardChunkType(stream.written().at(3)), ChunkType::DataChunk);

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QCOMPARE(stream.written().size(), static_cast<size_t>(5));
  QCOMPARE(clipboardChunkType(stream.written().at(4)), ChunkType::DataEnd);

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QCOMPARE(stream.written().size(), static_cast<size_t>(5));
}

void ServerProxyTests::onClipboardChanged_otherClipboardInFlight_waitsForTransferToEnd()
{
  RecordingEventQueue events;
  FakeStream stream;
  ServerProxy proxy(undereferenceableClient(), &stream, &events);
  Clipboard clipboard;
  fillWithText(clipboard, 100 * 1024);
  Clipboard selection;
  fillWithText(selection, 10);

  proxy.onClipboardChanged(kClipboardClipboard, &clipboard);
  proxy.onClipboardChanged(kClipboardSelection, &selection);

  QCOMPARE(stream.written().size(), static_cast<size_t>(2));

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QCOMPARE(stream.written().size(), static_cast<size_t>(6));
  QCOMPARE(clipboardChunkType(stream.written().at(3)), ChunkType::DataEnd);
  QCOMPARE(clipboardChunkId(stream.written().at(3)), kClipboardClipboard);
  QCOMPARE(clipboardChunkType(stream.written().at(4)), ChunkType::DataStart);
  QCOMPARE(clipboardChunkId(stream.written().at(4)), kClipboardSelection);
  QCOMPARE(clipboardChunkType(stream.written().at(5)), ChunkType::DataChunk);
  QCOMPARE(clipboardChunkId(stream.written().at(5)), kClipboardSelection);
}

void ServerProxyTests::onClipboardChanged_sameClipboardInFlight_restartsTransfer()
{
  RecordingEventQueue events;
  FakeStream stream;
  ServerProxy proxy(undereferenceableClient(), &stream, &events);
  Clipboard older;
  fillWithText(older, 150 * 1024);
  Clipboard newer;
  fillWithText(newer, 10);

  proxy.onClipboardChanged(kClipboardClipboard, &older);
  proxy.onClipboardChanged(kClipboardClipboard, &newer);

  QCOMPARE(stream.written().size(), static_cast<size_t>(4));
  QCOMPARE(clipboardChunkType(stream.written().at(2)), ChunkType::DataStart);
  QCOMPARE(clipboardChunkType(stream.written().at(3)), ChunkType::DataChunk);

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QCOMPARE(stream.written().size(), static_cast<size_t>(5));
  QCOMPARE(clipboardChunkType(stream.written().at(4)), ChunkType::DataEnd);

  QVERIFY(events.dispatchEvent(Event(EventTypes::StreamOutputFlushed, stream.getEventTarget())));
  QCOMPARE(stream.written().size(), static_cast<size_t>(5));
}

QTEST_MAIN(ServerProxyTests)
