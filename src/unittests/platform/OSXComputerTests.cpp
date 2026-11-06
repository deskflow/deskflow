/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXComputer.h"

#include <QTest>
#include <unistd.h>

class OSXComputerTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void isLocalPointerMovement_eventSource_expectedResult_data();
  void isLocalPointerMovement_eventSource_expectedResult();
  void isLocalPointerMovement_nullEvent_false();
  void handleCGInputEventSecondary_syntheticMovement_unchanged();
};

void OSXComputerTests::isLocalPointerMovement_eventSource_expectedResult_data()
{
  QTest::addColumn<int>("type");
  QTest::addColumn<qint64>("pid");
  QTest::addColumn<bool>("expected");

  const CGEventType movements[] = {
      kCGEventMouseMoved, kCGEventLeftMouseDragged, kCGEventRightMouseDragged, kCGEventOtherMouseDragged
  };
  for (const auto type : movements) {
    QTest::addRow("local-%u", type) << int(type) << qint64(0) << true;
    QTest::addRow("deskflow-%u", type) << int(type) << qint64(getpid()) << false;
    QTest::addRow("other-process-%u", type) << int(type) << qint64(getpid() + 1) << false;
  }
  const CGEventType unrelated[] = {
      kCGEventKeyDown,       kCGEventKeyUp,       kCGEventFlagsChanged,         kCGEventScrollWheel,
      kCGEventLeftMouseDown, kCGEventLeftMouseUp, kCGEventTapDisabledByTimeout, kCGEventTapDisabledByUserInput
  };
  for (const auto type : unrelated) {
    QTest::addRow("unrelated-%u", type) << int(type) << qint64(0) << false;
  }
}

void OSXComputerTests::isLocalPointerMovement_eventSource_expectedResult()
{
  QFETCH(int, type);
  QFETCH(qint64, pid);
  QFETCH(bool, expected);

  CGEventRef event = CGEventCreate(nullptr);
  QVERIFY(event);
  CGEventSetIntegerValueField(event, kCGEventSourceUnixProcessID, pid);
  const bool actual = OSXComputer::isLocalPointerMovement(static_cast<CGEventType>(type), event);
  CFRelease(event);
  QCOMPARE(actual, expected);
}

void OSXComputerTests::isLocalPointerMovement_nullEvent_false()
{
  QVERIFY(!OSXComputer::isLocalPointerMovement(kCGEventMouseMoved, nullptr));
}

void OSXComputerTests::handleCGInputEventSecondary_syntheticMovement_unchanged()
{
  CGEventRef event = CGEventCreate(nullptr);
  QVERIFY(event);
  CGEventSetIntegerValueField(event, kCGEventSourceUnixProcessID, getpid());
  const auto returned = OSXComputer::handleCGInputEventSecondary(nullptr, kCGEventMouseMoved, event, nullptr);
  QCOMPARE(returned, event);
  CFRelease(event);
}

QTEST_GUILESS_MAIN(OSXComputerTests)
#include "OSXComputerTests.moc"
