// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "../../deskflow/MockEventQueue.h"
#include "base/Event.h"
#include "base/EventQueueTimer.h"
#include "base/Log.h"
#include "client/Client.h"
#include "common/PeerAuthentication.h"
#include "deskflow/ClientApp.h"
#include "deskflow/ipc/CoreIpcServer.h"
#include <QCoreApplication>
#include <QTest>
#include <map>
class RecordingQueue : public MockEventQueue
{
public:
  int created = 0, deleted = 0, quit = 0;
  std::map<void *, EventHandler> handlers;
  std::vector<EventQueueTimer *> timers;
  ~RecordingQueue() override
  {
    for (auto *p : timers)
      delete p;
  }
  EventQueueTimer *newOneShotTimer(double, void *) override
  {
    ++created;
    auto *p = new EventQueueTimer;
    timers.push_back(p);
    return p;
  }
  void deleteTimer(EventQueueTimer *) override
  {
    ++deleted;
  }
  void addHandler(EventTypes, void *target, const EventHandler &h) override
  {
    handlers[target] = h;
  }
  void removeHandler(EventTypes, void *target) override
  {
    handlers.erase(target);
  }
  void addEvent(Event &&e) override
  {
    if (e.getType() == EventTypes::Quit)
      ++quit;
  }
};
class SafeClient : public ClientApp
{
public:
  using ClientApp::ClientApp;
  int attempts = 0;
  QString simulatedAddress = "127.0.0.2", lastAddress;
  bool startClient() override
  {
    ++attempts;
    lastAddress = simulatedAddress;
    return true;
  } // No Computer/input/socket.
};
class ClientAppRecoveryTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void duplicateLossEventsProduceOneRetry()
  {
    RecordingQueue q;
    SafeClient app(&q);
    app.handleClientDisconnected();
    app.handleClientDisconnected();
    QCOMPARE(q.created, 1);
    app.stopClient();
    QCOMPARE(q.deleted, 1);
  }
  void stoppedClientRejectsPreviouslyQueuedTimer()
  {
    RecordingQueue q;
    SafeClient app(&q);
    app.scheduleClientRestart(1);
    auto callback = q.handlers.begin()->second;
    app.stopClient();
    callback(Event(EventTypes::Timer));
    QCOMPARE(app.attempts, 0);
    app.handleClientDisconnected();
    QCOMPARE(q.created, 1);
  }
  void recoverableFailureRetriesAndUsesFreshAddress()
  {
    RecordingQueue q;
    SafeClient app(&q);
    auto *failure = new Client::FailInfo("synthetic network unavailable");
    failure->m_retry = true;
    app.handleClientRefused(
        Event(EventTypes::ClientConnectionRefused, nullptr, failure, Event::EventFlags::DontFreeData)
    );
    QCOMPARE(q.created, 1);
    auto callback = q.handlers.begin()->second;
    app.simulatedAddress = "127.0.0.3";
    callback(Event(EventTypes::Timer));
    QCOMPARE(app.attempts, 1);
    QCOMPARE(app.lastAddress, QString("127.0.0.3"));
    callback(Event(EventTypes::Timer));
    QCOMPARE(app.attempts, 1);
    app.stopClient();
  }
  void nonretryableFailureStopsPendingRecovery()
  {
    RecordingQueue q;
    SafeClient app(&q);
    app.scheduleClientRestart(1);
    auto callback = q.handlers.begin()->second;
    auto *failure = new Client::FailInfo("synthetic configuration refusal");
    failure->m_retry = false;
    app.handleClientRefused(
        Event(EventTypes::ClientConnectionRefused, nullptr, failure, Event::EventFlags::DontFreeData)
    );
    QCOMPARE(q.quit, 1);
    callback(Event(EventTypes::Timer));
    QCOMPARE(app.attempts, 0);
    app.scheduleClientRestart(1);
    QCOMPARE(q.created, 1);
    app.stopClient();
  }
  void successfulConnectionCancelsObsoleteRetry()
  {
    RecordingQueue q;
    SafeClient app(&q);
    app.scheduleClientRestart(1);
    auto callback = q.handlers.begin()->second;
    app.handleClientConnected();
    callback(Event(EventTypes::Timer));
    QCOMPARE(app.attempts, 0);
    app.stopClient();
  }
  void rejectedAuthenticationCancelsRecoveryAcrossAddressChanges()
  {
    RecordingQueue q;
    SafeClient app(&q);
    app.scheduleClientRestart(1);
    auto callback = q.handlers.begin()->second;
    QVERIFY(deskflow::auth::Runtime::activate({QString(32, '1'), QString(64, 'a'), 1}));
    deskflow::auth::Runtime::reject();
    app.simulatedAddress = "127.0.0.9";
    callback(Event(EventTypes::Timer));
    QCOMPARE(app.attempts, 0);
    app.handleClientDisconnected();
    QCOMPARE(q.created, 1);
    app.stopClient();
  }
};
int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  Log log;
  deskflow::core::ipc::CoreIpcServer ipc(nullptr); // Constructed only; listen() is never called.
  ClientAppRecoveryTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "ClientAppRecoveryTests.moc"
