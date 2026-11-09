/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "StatusBarTests.h"

#include "common/Constants.h"
#include "gui/widgets/StatusBar.h"

#include <QLabel>
#include <QTimer>

#include <limits>

namespace {

QLabel *statusLabel(const StatusBar &bar)
{
  const auto labels = bar.findChildren<QLabel *>();
  for (auto *label : labels) {
    if (label->text().startsWith(kAppName))
      return label;
  }
  return nullptr;
}

QString waitingText(int seconds)
{
  return StatusBar::tr("%1 is waiting %n second(s) before the next retry", nullptr, seconds).arg(kAppName);
}

void inactiveStates()
{
  QTest::addColumn<ConnectionState>("connection");
  QTest::addColumn<ProcessState>("process");
  QTest::addColumn<bool>("server");

  QTest::newRow("stopped") << ConnectionState::Connecting << ProcessState::Stopped << false;
  QTest::newRow("stopping") << ConnectionState::Connecting << ProcessState::Stopping << false;
  QTest::newRow("starting") << ConnectionState::Connecting << ProcessState::Starting << false;
  QTest::newRow("process-retry") << ConnectionState::Connecting << ProcessState::RetryPending << false;
  QTest::newRow("listening") << ConnectionState::Listening << ProcessState::Started << true;
  QTest::newRow("server-connected") << ConnectionState::Connected << ProcessState::Started << true;
  QTest::newRow("server-connecting") << ConnectionState::Connecting << ProcessState::Started << true;
}

} // namespace

void StatusBarTests::setStatus_connectingWithoutRetry_keepsTimerStopped()
{
  StatusBar bar;
  auto *timer = bar.findChild<QTimer *>();
  QVERIFY(timer);

  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);

  QVERIFY(!timer->isActive());
}

void StatusBarTests::setConnectionInterval_connecting_updatesTextImmediately()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  auto *label = statusLabel(bar);
  QVERIFY(label);

  bar.setConnectionInterval(30);

  QCOMPARE(label->text(), waitingText(30));
}

void StatusBarTests::setConnectionInterval_newRetry_replacesWait()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  bar.setConnectionInterval(300);
  auto *label = statusLabel(bar);
  QVERIFY(label);

  bar.setConnectionInterval(30);

  QCOMPARE(label->text(), waitingText(30));
}

void StatusBarTests::setConnectionInterval_disconnected_showsScheduledRetry()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Disconnected, ProcessState::Started, false);
  auto *label = statusLabel(bar);
  QVERIFY(label);

  bar.setConnectionInterval(30);

  QCOMPARE(label->text(), waitingText(30));
}

void StatusBarTests::setStatus_repeatedConnecting_doesNotConsumeWait()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  bar.setConnectionInterval(300);
  auto *label = statusLabel(bar);
  QVERIFY(label);
  const auto expected = label->text();

  for (int i = 0; i < 5; ++i)
    bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);

  QCOMPARE(label->text(), expected);
}

void StatusBarTests::setStatus_repeatedDisconnected_preservesWait()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Disconnected, ProcessState::Started, false);
  bar.setConnectionInterval(30);
  auto *label = statusLabel(bar);
  QVERIFY(label);

  bar.setStatus(ConnectionState::Disconnected, ProcessState::Started, false);

  QCOMPARE(label->text(), waitingText(30));
}

void StatusBarTests::setStatus_leavingConnecting_clearsWait_data()
{
  inactiveStates();
}

void StatusBarTests::setStatus_leavingConnecting_clearsWait()
{
  QFETCH(ConnectionState, connection);
  QFETCH(ProcessState, process);
  QFETCH(bool, server);
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  bar.setConnectionInterval(30);
  auto *timer = bar.findChild<QTimer *>();
  QVERIFY(timer);

  bar.setStatus(connection, process, server);
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);

  QVERIFY(!timer->isActive());
}

void StatusBarTests::setConnectionInterval_inactive_ignoresLateRetry_data()
{
  inactiveStates();
}

void StatusBarTests::setConnectionInterval_inactive_ignoresLateRetry()
{
  QFETCH(ConnectionState, connection);
  QFETCH(ProcessState, process);
  QFETCH(bool, server);
  StatusBar bar;
  bar.setStatus(connection, process, server);
  auto *label = statusLabel(bar);
  QVERIFY(label);
  const auto expected = label->text();

  bar.setConnectionInterval(30);

  QCOMPARE(label->text(), expected);
}

void StatusBarTests::setConnectionInterval_nonPositive_clearsWait_data()
{
  QTest::addColumn<int>("seconds");
  QTest::newRow("zero") << 0;
  QTest::newRow("negative") << -1;
  QTest::newRow("minimum") << std::numeric_limits<int>::min();
}

void StatusBarTests::setConnectionInterval_nonPositive_clearsWait()
{
  QFETCH(int, seconds);
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  bar.setConnectionInterval(30);
  auto *timer = bar.findChild<QTimer *>();
  QVERIFY(timer);

  bar.setConnectionInterval(seconds);

  QVERIFY(!timer->isActive());
}

void StatusBarTests::setConnectionInterval_oneSecond_showsPendingWait()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  auto *label = statusLabel(bar);
  QVERIFY(label);

  bar.setConnectionInterval(1);

  QCOMPARE(label->text(), waitingText(1));
}

void StatusBarTests::setConnectionInterval_largeValue_doesNotOverflow()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  auto *label = statusLabel(bar);
  QVERIFY(label);

  bar.setConnectionInterval(std::numeric_limits<int>::max());

  QCOMPARE(label->text(), waitingText(std::numeric_limits<int>::max()));
}

void StatusBarTests::setStatus_afterEventLoopDelay_usesElapsedTime()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  bar.setConnectionInterval(2);
  auto *label = statusLabel(bar);
  QVERIFY(label);

  QTest::qSleep(2100);
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);

  QCOMPARE(label->text(), StatusBar::tr("%1 is connecting...").arg(kAppName));
}

void StatusBarTests::retryTimer_afterDeadline_stops()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  auto *timer = bar.findChild<QTimer *>();
  QVERIFY(timer);

  bar.setConnectionInterval(1);
  QVERIFY(timer->isActive());

  QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 3000);
}

void StatusBarTests::retryTimer_lateTimeout_keepsStoppedStatus()
{
  StatusBar bar;
  bar.setStatus(ConnectionState::Connecting, ProcessState::Started, false);
  bar.setConnectionInterval(30);
  bar.setStatus(ConnectionState::Disconnected, ProcessState::Stopped, false);
  auto *timer = bar.findChild<QTimer *>();
  auto *label = statusLabel(bar);
  QVERIFY(timer);
  QVERIFY(label);
  const auto expected = label->text();

  QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));

  QCOMPARE(label->text(), expected);
}

QTEST_MAIN(StatusBarTests)
