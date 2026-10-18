/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerProxyTests.h"

#include "../deskflow/RecordingPlatformComputer.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "client/Client.h"
#include "client/ServerProxy.h"
#include "common/Settings.h"
#include "deskflow/AppUtil.h"
#include "deskflow/Computer.h"
#include "deskflow/ProtocolTypes.h"
#include "io/IStream.h"
#include "net/ISocketFactory.h"

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

class NullSocketFactory : public ISocketFactory
{
public:
  IDataSocket *create(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    return nullptr;
  }
  IListenSocket *createListen(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    return nullptr;
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

  void write(const void *, uint32_t) override
  {
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

private:
  std::deque<std::string> m_chunks;
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
  QVERIFY(m_settings.isValid());
  Settings::setSettingsFile(m_settings.filePath("Deskflow.conf"));
  Settings::setStateFile(m_settings.filePath("Deskflow.state"));
  (void)testAppUtil();
  m_log.setFilter(LogLevel::Level::Debug);
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

void ServerProxyTests::navigationGesture_invalidSlots_areIgnored()
{
  for (int8_t slot : {int8_t(0), int8_t(3), int8_t(-1)}) {
    FakeStream stream;
    stream.push(std::string(1, static_cast<char>(slot)));
    RecordingEventQueue events;
    ServerProxy proxy(undereferenceableClient(), &stream, &events);
    proxy.handleMessage(reinterpret_cast<const uint8_t *>("DNGS"));
    QCOMPARE(stream.getSize(), uint32_t(0));
    QVERIFY(events.addedEvents().empty());
  }
}

void ServerProxyTests::handleHello_negotiatesSupportedVersionsAndRoutesNavigation()
{
  for (const std::string protocol : {"Synergy", "Barrier"}) {
    for (int minor : {6, 7, 8, 9}) {
      RecordingEventQueue events;
      auto *platform = new RecordingPlatformComputer(&events);
      deskflow::Computer computer(platform, &events);
      Client client(&events, "test-client", NetworkAddress(), new NullSocketFactory, &computer);
      auto *stream = new FakeStream;
      std::string hello = protocol;
      hello.append("\0\1\0", 3);
      hello.push_back(static_cast<char>(minor));
      stream->push(hello);
      client.m_stream = stream;
      client.handleHello();
      QVERIFY2(client.isConnected(), (protocol + " 1." + std::to_string(minor)).c_str());
      stream->push(std::string("DSOP") + std::string(4, '\0'));
      client.m_server->handleData();
      QVERIFY(client.m_ready);
      QVERIFY(platform->enabled);
      if (minor == 9) {
        for (int slot : {1, 2}) {
          stream->push(std::string("DNGS") + static_cast<char>(slot));
          client.m_server->handleData();
        }
#ifdef Q_OS_MACOS
        QCOMPARE(platform->keys.size(), size_t(2));
        QCOMPARE(platform->keys[0].key, KeyID('['));
        QCOMPARE(platform->keys[1].key, KeyID(']'));
        QCOMPARE(platform->keys[0].mask, KeyModifierMask(KeyModifierSuper));
        QCOMPARE(platform->releases.size(), size_t(2));
#else
        QCOMPARE(platform->buttons.size(), size_t(4));
        QCOMPARE(platform->buttons[0].first, kButtonExtra0);
        QCOMPARE(platform->buttons[2].first, kButtonExtra1);
#endif
      }
    }
  }
}

QTEST_MAIN(ServerProxyTests)
