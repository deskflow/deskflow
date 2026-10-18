/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXInputCompatibility.h"

#include <QTest>

using namespace deskflow::platform::osx;

class OSXInputCompatibilityTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void optionsSourceMatchesExecutableOnly();
  void middleClickRequiresRemoteOptionsEvent();
  void copyPreservesMouseEdgesAndPreventsLoops();
  void shiftSyncRequiresConsumedPhysicalRelease();
};

void OSXInputCompatibilityTests::optionsSourceMatchesExecutableOnly()
{
  QVERIFY(isOptionsAgentPath("/Applications/logioptionsplus.app/Contents/MacOS/logioptionsplus_agent"));
  QVERIFY(isOptionsAgentPath("logioptionsplus_agent"));
  QVERIFY(!isOptionsAgentPath("/Applications/other_agent"));
  QVERIFY(!isOptionsAgentPath("/Applications/logioptionsplus_agent_fake"));
  QVERIFY(!isOptionsAgentPath("/Applications/logioptionsplus_agent/other_agent"));
  QVERIFY(!isOptionsAgentPath(""));
  QVERIFY(!isOptionsAgentEvent(nullptr));
}

void OSXInputCompatibilityTests::middleClickRequiresRemoteOptionsEvent()
{
  auto event = CGEventCreateMouseEvent(nullptr, kCGEventOtherMouseDown, {10, 20}, kCGMouseButtonCenter);
  QVERIFY(event);
  QVERIFY(shouldRelayMiddleClick(kCGEventOtherMouseDown, event, false, true, true));
  QVERIFY(!shouldRelayMiddleClick(kCGEventOtherMouseDown, event, true, true, true));
  QVERIFY(!shouldRelayMiddleClick(kCGEventOtherMouseDown, event, false, false, true));
  QVERIFY(!shouldRelayMiddleClick(kCGEventOtherMouseDown, event, false, true, false));
  QVERIFY(!shouldRelayMiddleClick(kCGEventScrollWheel, event, false, true, true));
  QVERIFY(!shouldRelayMiddleClick(kCGEventOtherMouseDown, nullptr, false, true, true));
  CGEventSetIntegerValueField(event, kCGMouseEventButtonNumber, 3);
  QVERIFY(!shouldRelayMiddleClick(kCGEventOtherMouseDown, event, false, true, true));
  CFRelease(event);
}

void OSXInputCompatibilityTests::copyPreservesMouseEdgesAndPreventsLoops()
{
  for (auto type : {kCGEventOtherMouseDown, kCGEventOtherMouseUp}) {
    auto event = CGEventCreateMouseEvent(nullptr, type, {11, 22}, kCGMouseButtonCenter);
    QVERIFY(event);
    CGEventSetFlags(event, kCGEventFlagMaskControl);
    CGEventSetIntegerValueField(event, kCGMouseEventClickState, 2);
    auto copy = copyCompatibilityEvent(event);
    QVERIFY(copy);
    QCOMPARE(CGEventGetType(copy), type);
    QCOMPARE(CGEventGetIntegerValueField(copy, kCGMouseEventButtonNumber), int64_t(kCGMouseButtonCenter));
    QCOMPARE(CGEventGetIntegerValueField(copy, kCGMouseEventClickState), int64_t(2));
    QCOMPARE(CGEventGetLocation(copy).x, 11.0);
    QCOMPARE(CGEventGetLocation(copy).y, 22.0);
    QCOMPARE(CGEventGetFlags(copy), CGEventFlags(kCGEventFlagMaskControl));
    QCOMPARE(CGEventGetIntegerValueField(copy, kCGEventSourceUserData), kCompatibilityEventTag);
    QVERIFY(CGEventGetIntegerValueField(event, kCGEventSourceUserData) != kCompatibilityEventTag);
    QVERIFY(!shouldRelayMiddleClick(type, copy, false, true, true));
    CFRelease(copy);
    CFRelease(event);
  }
  QVERIFY(!copyCompatibilityEvent(nullptr));
}

void OSXInputCompatibilityTests::shiftSyncRequiresConsumedPhysicalRelease()
{
  for (auto key : {56, 60}) {
    auto event = CGEventCreateKeyboardEvent(nullptr, key, false);
    QVERIFY(event);
    CGEventSetType(event, kCGEventFlagsChanged);
    CGEventSetFlags(event, kCGEventFlagMaskControl);
    CGEventSetIntegerValueField(event, kCGEventSourceUnixProcessID, 0);
    QVERIFY(shouldSyncShiftRelease(kCGEventFlagsChanged, event, false, true));
    // A release on the host already reaches SESSION, so never duplicate it.
    QVERIFY(!shouldSyncShiftRelease(kCGEventFlagsChanged, event, true, true));
    QVERIFY(!shouldSyncShiftRelease(kCGEventFlagsChanged, event, false, false));
    QVERIFY(!shouldSyncShiftRelease(kCGEventKeyUp, event, false, true));
    auto copy = copyCompatibilityEvent(event);
    QVERIFY(copy);
    QVERIFY(!shouldSyncShiftRelease(kCGEventFlagsChanged, copy, false, true));
    QCOMPARE(CGEventGetFlags(copy), CGEventFlags(kCGEventFlagMaskControl));
    CFRelease(copy);
    CGEventSetIntegerValueField(event, kCGEventSourceUnixProcessID, 123);
    QVERIFY(!shouldSyncShiftRelease(kCGEventFlagsChanged, event, false, true));
    CGEventSetIntegerValueField(event, kCGEventSourceUnixProcessID, 0);
    CGEventSetFlags(event, kCGEventFlagMaskShift);
    QVERIFY(!shouldSyncShiftRelease(kCGEventFlagsChanged, event, false, true));
    CGEventSetFlags(event, 0);
    CGEventSetIntegerValueField(event, kCGKeyboardEventKeycode, 55);
    QVERIFY(!shouldSyncShiftRelease(kCGEventFlagsChanged, event, false, true));
    CFRelease(event);
  }
  QVERIFY(!shouldSyncShiftRelease(kCGEventFlagsChanged, nullptr, false, true));
}

QTEST_GUILESS_MAIN(OSXInputCompatibilityTests)
#include "OSXInputCompatibilityTests.moc"
