/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientListenerTests.h"

#include "base/EventQueue.h"
#include "net/IDataSocket.h"
#include "net/IListenSocket.h"
#include "net/ISocketFactory.h"
#include "net/NetworkAddress.h"
#include "server/ClientListener.h"

#include <QTest>

#include <memory>
#include <set>

namespace {

class HandlerTrackingEventQueue : public EventQueue
{
public:
  void addHandler(EventTypes type, void *target, const EventHandler &handler) override
  {
    m_targets.insert(target);
    EventQueue::addHandler(type, target, handler);
  }

  void removeHandlers(void *target) override
  {
    m_targets.erase(target);
    EventQueue::removeHandlers(target);
  }

  bool hasHandlers(void *target) const
  {
    return m_targets.contains(target);
  }

private:
  std::set<void *> m_targets;
};

class FakeDataSocket : public IDataSocket
{
public:
  explicit FakeDataSocket(void *eventTarget) : IDataSocket(nullptr), m_eventTarget(eventTarget)
  {
    // do nothing
  }

  void bind(const NetworkAddress &) override
  {
    // do nothing
  }

  void close() override
  {
    // do nothing
  }

  void *getEventTarget() const override
  {
    return m_eventTarget;
  }

  void connect(const NetworkAddress &) override
  {
    // do nothing
  }

  bool isFatal() const override
  {
    return false;
  }

  uint32_t read(void *, uint32_t) override
  {
    return 0;
  }

  void write(const void *, uint32_t) override
  {
    // do nothing
  }

  void flush() override
  {
    // do nothing
  }

  void shutdownInput() override
  {
    // do nothing
  }

  void shutdownOutput() override
  {
    // do nothing
  }

  bool isReady() const override
  {
    return false;
  }

  uint32_t getSize() const override
  {
    return 0;
  }

private:
  void *m_eventTarget;
};

class FakeListenSocket : public IListenSocket
{
public:
  explicit FakeListenSocket(void *socketEventTarget) : m_socketEventTarget(socketEventTarget)
  {
    // do nothing
  }

  void bind(const NetworkAddress &) override
  {
    // do nothing
  }

  void close() override
  {
    // do nothing
  }

  void *getEventTarget() const override
  {
    return const_cast<void *>(static_cast<const void *>(this));
  }

  std::unique_ptr<IDataSocket> accept() override
  {
    return std::make_unique<FakeDataSocket>(m_socketEventTarget);
  }

private:
  void *m_socketEventTarget;
};

class FakeSocketFactory : public ISocketFactory
{
public:
  FakeSocketFactory(void *socketEventTarget, IListenSocket **listenSocket)
      : m_socketEventTarget(socketEventTarget),
        m_listenSocket(listenSocket)
  {
    // do nothing
  }

  IDataSocket *create(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    return nullptr;
  }

  IListenSocket *createListen(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    *m_listenSocket = new FakeListenSocket(m_socketEventTarget);
    return *m_listenSocket;
  }

private:
  void *m_socketEventTarget;
  IListenSocket **m_listenSocket;
};

} // namespace

void ClientListenerTests::initTestCase()
{
  m_arch.init();
}

void ClientListenerTests::destructor_pendingSocket_removesSocketHandlers()
{
  HandlerTrackingEventQueue events;
  int socketEventTarget = 0;
  IListenSocket *listenSocket = nullptr;
  auto socketFactory = std::make_unique<FakeSocketFactory>(&socketEventTarget, &listenSocket);
  auto listener = std::make_unique<ClientListener>(
      NetworkAddress(24800), std::move(socketFactory), &events, SecurityLevel::Encrypted
  );
  events.dispatchEvent(Event(EventTypes::ListenSocketConnecting, listenSocket->getEventTarget()));

  listener.reset();

  QVERIFY(!events.hasHandlers(&socketEventTarget));
}

QTEST_MAIN(ClientListenerTests)
