/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXMouseEventTests.h"

#include "platform/OSXMouseEvent.h"

#include <QTest>

#include <array>
#include <memory>
#include <type_traits>

namespace {

using Event = std::unique_ptr<std::remove_pointer_t<CGEventRef>, decltype(&CFRelease)>;

constexpr std::array<CGEventType, 4> movementTypes = {
    kCGEventMouseMoved, kCGEventLeftMouseDragged, kCGEventRightMouseDragged, kCGEventOtherMouseDragged
};

CGMouseButton buttonForEvent(CGEventType type)
{
  if (type == kCGEventRightMouseDragged) {
    return kCGMouseButtonRight;
  }
  if (type == kCGEventOtherMouseDragged) {
    return kCGMouseButtonCenter;
  }
  return kCGMouseButtonLeft;
}

} // namespace

void OSXMouseEventTests::markedMovement_copyKeepsMarkerAndCannotRestore_data()
{
  QTest::addColumn<quint32>("eventType");
  for (const auto type : movementTypes) {
    QTest::newRow(qPrintable(QString::number(type))) << static_cast<quint32>(type);
  }
}

void OSXMouseEventTests::markedMovement_copyKeepsMarkerAndCannotRestore()
{
  QFETCH(quint32, eventType);
  const auto type = static_cast<CGEventType>(eventType);
  const auto position = CGPointMake(640, 400);
  Event event(CGEventCreateMouseEvent(nullptr, type, position, buttonForEvent(type)), CFRelease);
  QVERIFY(event);

  deskflow::osx::markMouseEvent(event.get());
  QCOMPARE(CGEventGetIntegerValueField(event.get(), kCGEventSourceUserData), deskflow::osx::kMouseEventMarker);
  QVERIFY(!deskflow::osx::shouldRestoreCursor(CGEventGetType(event.get()), event.get()));

  Event copy(CGEventCreateCopy(event.get()), CFRelease);
  QVERIFY(copy);
  QCOMPARE(CGEventGetIntegerValueField(copy.get(), kCGEventSourceUserData), deskflow::osx::kMouseEventMarker);
  QCOMPARE(CGEventGetType(copy.get()), type);
  QCOMPARE(CGEventGetLocation(copy.get()).x, position.x);
  QCOMPARE(CGEventGetLocation(copy.get()).y, position.y);
  QVERIFY(!deskflow::osx::shouldRestoreCursor(CGEventGetType(copy.get()), copy.get()));
}

void OSXMouseEventTests::unmarkedMovement_restoresAtAnyPosition_data()
{
  QTest::addColumn<quint32>("eventType");
  QTest::addColumn<double>("x");
  QTest::addColumn<double>("y");
  for (const auto type : movementTypes) {
    const auto prefix = QString::number(type);
    // These are sample display coordinates; this test never queries or moves the real cursor.
    QTest::newRow(qPrintable(prefix + "-center")) << static_cast<quint32>(type) << 640.0 << 400.0;
    QTest::newRow(qPrintable(prefix + "-edge")) << static_cast<quint32>(type) << 0.0 << 0.0;
    QTest::newRow(qPrintable(prefix + "-other-display")) << static_cast<quint32>(type) << -1920.0 << 200.0;
  }
}

void OSXMouseEventTests::unmarkedMovement_restoresAtAnyPosition()
{
  QFETCH(quint32, eventType);
  QFETCH(double, x);
  QFETCH(double, y);
  const auto type = static_cast<CGEventType>(eventType);
  Event event(CGEventCreateMouseEvent(nullptr, type, CGPointMake(x, y), buttonForEvent(type)), CFRelease);
  QVERIFY(event);

  QVERIFY(deskflow::osx::shouldRestoreCursor(CGEventGetType(event.get()), event.get()));
}

void OSXMouseEventTests::unrelatedEvents_doNotRestore_data()
{
  QTest::addColumn<quint32>("eventType");
  constexpr std::array<CGEventType, 11> types = {kCGEventNull,           kCGEventLeftMouseDown, kCGEventLeftMouseUp,
                                                 kCGEventRightMouseDown, kCGEventRightMouseUp,  kCGEventOtherMouseDown,
                                                 kCGEventOtherMouseUp,   kCGEventKeyDown,       kCGEventKeyUp,
                                                 kCGEventFlagsChanged,   kCGEventScrollWheel};
  for (const auto type : types) {
    QTest::newRow(qPrintable(QString::number(type))) << static_cast<quint32>(type);
  }
}

void OSXMouseEventTests::unrelatedEvents_doNotRestore()
{
  QFETCH(quint32, eventType);
  Event event(CGEventCreate(nullptr), CFRelease);
  QVERIFY(event);
  CGEventSetType(event.get(), static_cast<CGEventType>(eventType));

  QVERIFY(!deskflow::osx::shouldRestoreCursor(CGEventGetType(event.get()), event.get()));
  deskflow::osx::markMouseEvent(event.get());
  QVERIFY(!deskflow::osx::shouldRestoreCursor(CGEventGetType(event.get()), event.get()));
}

void OSXMouseEventTests::anotherApplicationsUserData_stillRestores()
{
  Event event(CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, CGPointMake(40, 50), kCGMouseButtonLeft), CFRelease);
  QVERIFY(event);
  CGEventSetIntegerValueField(event.get(), kCGEventSourceUserData, 42);

  // The marker excludes Deskflow injection, not all synthetic input from other applications.
  QVERIFY(deskflow::osx::shouldRestoreCursor(CGEventGetType(event.get()), event.get()));
}

void OSXMouseEventTests::nullEvents_neverRestore()
{
  for (const auto type : movementTypes) {
    QVERIFY(!deskflow::osx::shouldRestoreCursor(type, nullptr));
  }
  QVERIFY(!deskflow::osx::shouldRestoreCursor(kCGEventTapDisabledByTimeout, nullptr));
  QVERIFY(!deskflow::osx::shouldRestoreCursor(kCGEventTapDisabledByUserInput, nullptr));
}

QTEST_APPLESS_MAIN(OSXMouseEventTests)
