/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerAppTests.h"

#include "base/EventQueue.h"
#include "deskflow/ServerApp.h"

namespace {
// Fails to start a set number of times, like a bind before the network is back after wake (#10314)
class FlakyStartServerApp : public ServerApp
{
public:
  FlakyStartServerApp(IEventQueue *events, int failures) : ServerApp(events), m_failures(failures)
  {
  }

  bool startServer() override
  {
    ++attempts;
    if (attempts <= m_failures) {
      return false;
    }
    getEvents()->addEvent(Event(EventTypes::Quit));
    return true;
  }

  int attempts = 0;

private:
  int m_failures;
};

void quitAfter(IEventQueue &events, double seconds)
{
  EventQueueTimer *timer = events.newOneShotTimer(seconds, nullptr);
  events.addHandler(EventTypes::Timer, timer, [&events](const auto &) { events.addEvent(Event(EventTypes::Quit)); });
}
} // namespace

void ServerAppTests::initTestCase()
{
  m_arch.init();
}

void ServerAppTests::handleResume_startFails_retriesUntilStarted()
{
  EventQueue events;
  FlakyStartServerApp app(&events, 2);
  quitAfter(events, 10.0);

  app.handleSuspend();
  app.handleResume();
  events.loop();

  QCOMPARE(app.attempts, 3);
}

void ServerAppTests::handleSuspend_whileRetrying_stopsRetrying()
{
  EventQueue events;
  FlakyStartServerApp app(&events, 1000);

  app.handleSuspend();
  app.handleResume();
  app.handleSuspend();
  quitAfter(events, 2.5);
  events.loop();

  QCOMPARE(app.attempts, 1);
}

QTEST_GUILESS_MAIN(ServerAppTests)
