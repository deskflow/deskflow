/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXTrackpadGestureTests.h"

#include "platform/OSXTrackpadGesture.h"

using namespace deskflow::osx::gesture;

namespace {

CGEventRef scroll(bool continuous, int64_t phase, int64_t momentum)
{
  CGEventRef event = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, -12, 30);
  CGEventSetIntegerValueField(event, kCGScrollWheelEventIsContinuous, continuous ? 1 : 0);
  CGEventSetIntegerValueField(event, kCGScrollWheelEventScrollPhase, phase);
  CGEventSetIntegerValueField(event, kCGScrollWheelEventMomentumPhase, momentum);
  return event;
}

CGEventRef parse(const std::string &bytes)
{
  CFDataRef data =
      CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(bytes.data()), static_cast<CFIndex>(bytes.size()));
  CGEventRef event = CGEventCreateFromData(kCFAllocatorDefault, data);
  CFRelease(data);
  return event;
}

} // namespace

void OSXTrackpadGestureTests::trackpadScrollsAreForwarded_data()
{
  QTest::addColumn<bool>("continuous");
  QTest::addColumn<int>("phase");
  QTest::addColumn<int>("momentum");
  QTest::addColumn<bool>("forwarded");

  QTest::newRow("trackpad scroll began") << true << int(kCGScrollPhaseBegan) << 0 << true;
  QTest::newRow("trackpad may begin") << true << int(kCGScrollPhaseMayBegin) << 0 << true;
  QTest::newRow("momentum") << true << 0 << int(kCGMomentumScrollPhaseContinue) << true;
  QTest::newRow("smooth mouse wheel, no phases") << true << 0 << 0 << false;
  QTest::newRow("mouse wheel notch") << false << 0 << 0 << false;
}

void OSXTrackpadGestureTests::trackpadScrollsAreForwarded()
{
  QFETCH(bool, continuous);
  QFETCH(int, phase);
  QFETCH(int, momentum);
  QFETCH(bool, forwarded);

  CGEventRef event = scroll(continuous, phase, momentum);
  QCOMPARE(takeScroll(event).has_value(), forwarded);
  CFRelease(event);
}

void OSXTrackpadGestureTests::forwardedScrollKeepsItsPhases()
{
  CGEventRef event = scroll(true, kCGScrollPhaseChanged, 0);
  const auto bytes = takeScroll(event);
  CFRelease(event);
  QVERIFY(bytes.has_value());

  CGEventRef received = parse(*bytes);
  QVERIFY(received != nullptr);
  QCOMPARE(CGEventGetType(received), kCGEventScrollWheel);
  QCOMPARE(CGEventGetIntegerValueField(received, kCGScrollWheelEventScrollPhase), int64_t(kCGScrollPhaseChanged));
  QCOMPARE(CGEventGetIntegerValueField(received, kCGScrollWheelEventIsContinuous), int64_t(1));
  QCOMPARE(CGEventGetIntegerValueField(received, kCGScrollWheelEventPointDeltaAxis1), int64_t(-12));
  QCOMPARE(CGEventGetIntegerValueField(received, kCGScrollWheelEventPointDeltaAxis2), int64_t(30));
  CFRelease(received);
}

void OSXTrackpadGestureTests::scaleScrollInvertsAndScales()
{
  CGEventRef event = scroll(true, kCGScrollPhaseChanged, 0);
  const auto y = CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1);
  const auto x = CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis2);
  const auto fixedY = CGEventGetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis1);
  const auto lineY = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);

  scaleScroll(event, 1, 1);
  QCOMPARE(CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1), y);

  // inverted vertical, doubled horizontal
  scaleScroll(event, 2, -1);
  QCOMPARE(CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1), -y);
  QCOMPARE(CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis2), 2 * x);
  QCOMPARE(CGEventGetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis1), -fixedY);
  QCOMPARE(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1), -lineY);
  // the phase is what makes it a trackpad scroll; it stays
  QCOMPARE(CGEventGetIntegerValueField(event, kCGScrollWheelEventScrollPhase), int64_t(kCGScrollPhaseChanged));
  CFRelease(event);
}

QTEST_MAIN(OSXTrackpadGestureTests)
