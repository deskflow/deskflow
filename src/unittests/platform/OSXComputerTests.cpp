/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXComputerTests.h"

#include "platform/OSXComputer.h"

namespace {
std::optional<ScrollDelta> continuousDelta(CGScrollEventUnit unit, int32_t y, int32_t x)
{
  CGEventRef event = CGEventCreateScrollWheelEvent(nullptr, unit, 2, y, x);
  auto delta = OSXComputer::continuousScrollDelta(event);
  CFRelease(event);
  return delta;
}
} // namespace

void OSXComputerTests::continuousScrollKeepsSubNotchPrecision()
{
  // Magic Mouse / trackpad: a 10 point scroll must not round to a whole 120 notch
  const auto delta = continuousDelta(kCGScrollEventUnitPixel, 10, 0);

  QVERIFY(delta.has_value());
  QCOMPARE(delta->y, 12);
  QCOMPARE(delta->x, 0);
}

void OSXComputerTests::continuousScrollKeepsDirection()
{
  const auto delta = continuousDelta(kCGScrollEventUnitPixel, -5, 20);

  QVERIFY(delta.has_value());
  QCOMPARE(delta->y, -6);
  QCOMPARE(delta->x, 24);
}

void OSXComputerTests::lineScrollUsesNotchPath()
{
  QVERIFY(!continuousDelta(kCGScrollEventUnitLine, 1, 0).has_value());
}

void OSXComputerTests::continuousScrollClampsToWireRange()
{
  // the wire carries int16; an unclamped value would wrap and reverse the scroll
  const auto delta = continuousDelta(kCGScrollEventUnitPixel, 30000, -30000);

  QVERIFY(delta.has_value());
  QCOMPARE(delta->y, INT16_MAX);
  QCOMPARE(delta->x, INT16_MIN);
}

namespace {
// one stroke: began, `steps` changes of (dx, dy) points, then `endPhase`
std::optional<uint16_t> stroke(int steps, int32_t dx, int32_t dy, int64_t endPhase = kCGScrollPhaseEnded)
{
  OSXComputer::SwipeNavigation swipe;
  std::optional<uint16_t> button = swipe.update(kCGScrollPhaseBegan, 0, 0);
  for (int i = 0; i < steps; ++i) {
    if (const auto b = swipe.update(kCGScrollPhaseChanged, dx, dy))
      button = b;
  }
  if (const auto b = swipe.update(endPhase, 0, 0))
    button = b;
  return button;
}
} // namespace

void OSXComputerTests::swipeRightIsBack()
{
  QCOMPARE(stroke(10, 10, 1), std::optional<uint16_t>(4));
}

void OSXComputerTests::swipeLeftIsForward()
{
  QCOMPARE(stroke(10, -10, 1), std::optional<uint16_t>(5));
}

void OSXComputerTests::shortStrokeIsNotSwipe()
{
  QCOMPARE(stroke(2, 10, 0), std::nullopt);
}

void OSXComputerTests::verticalStrokeIsNotSwipe()
{
  QCOMPARE(stroke(10, 10, 8), std::nullopt);
}

void OSXComputerTests::cancelledStrokeIsNotSwipe()
{
  QCOMPARE(stroke(10, 10, 0, kCGScrollPhaseCancelled), std::nullopt);
}

void OSXComputerTests::strokeWithoutBeganIsNotSwipe()
{
  // cursor crossed to this screen mid-stroke: the began phase was never seen here
  OSXComputer::SwipeNavigation swipe;
  for (int i = 0; i < 10; ++i)
    swipe.update(kCGScrollPhaseChanged, 10, 0);

  QCOMPARE(swipe.update(kCGScrollPhaseEnded, 0, 0), std::nullopt);
}

void OSXComputerTests::swipeThresholdIsSixtyPoints()
{
  QCOMPARE(stroke(1, 60, 0), std::optional<uint16_t>(4));
  QCOMPARE(stroke(1, 59, 0), std::nullopt);
}

QTEST_MAIN(OSXComputerTests)
