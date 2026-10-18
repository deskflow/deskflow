/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXComputerTests.h"

#include "common/NavigationTypes.h"
#include "platform/OSXComputer.h"

constexpr auto kGestureType = static_cast<CGEventType>(29);

void OSXComputerTests::navigationGesturesEnabledFromOptions_enabledOption_returnsTrue()
{
  const OptionsList enabled = {kOptionMacNavigationGestures, 1};

  QVERIFY(OSXComputer::navigationGesturesEnabledFromOptions(enabled, false));
}

void OSXComputerTests::navigationGesturesEnabledFromOptions_disabledOption_returnsFalse()
{
  const OptionsList disabled = {kOptionMacNavigationGestures, 0};

  QVERIFY(!OSXComputer::navigationGesturesEnabledFromOptions(disabled, true));
}

void OSXComputerTests::navigationGesturesEnabledFromOptions_unrelatedOption_preservesCurrentValue()
{
  const OptionsList unrelated = {kOptionClipboardSharing, 1};

  QVERIFY(OSXComputer::navigationGesturesEnabledFromOptions(unrelated, true));
}

void OSXComputerTests::navigationGesturesEnabledFromOptions_malformedOptions_preservesCurrentValue()
{
  const OptionsList malformed = {kOptionMacNavigationGestures};

  QVERIFY(OSXComputer::navigationGesturesEnabledFromOptions(malformed, true));
}

void OSXComputerTests::classifyNavigationGesture_swipeLeft_returnsLeft()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kGestureType, false, true, 1.0, 0.0), NavigationGestureDirection::Left
  );
}

void OSXComputerTests::classifyNavigationGesture_swipeRight_returnsRight()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kGestureType, false, true, -1.0, 0.0), NavigationGestureDirection::Right
  );
}

void OSXComputerTests::classifyNavigationGesture_swipeUp_returnsUp()
{
  QCOMPARE(OSXComputer::classifyNavigationGesture(kGestureType, false, true, 0.0, 1.0), NavigationGestureDirection::Up);
}

void OSXComputerTests::classifyNavigationGesture_swipeDown_returnsDown()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kGestureType, false, true, 0.0, -1.0), NavigationGestureDirection::Down
  );
}

void OSXComputerTests::classifyNavigationGesture_ambiguousDelta_returnsNone()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kGestureType, false, true, 1.0, 1.0), NavigationGestureDirection::None
  );
}

void OSXComputerTests::classifyNavigationGesture_disabled_returnsNone()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kGestureType, false, false, 1.0, 0.0), NavigationGestureDirection::None
  );
}

void OSXComputerTests::classifyNavigationGesture_localScreen_returnsNone()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kGestureType, true, true, 1.0, 0.0), NavigationGestureDirection::None
  );
}

void OSXComputerTests::classifyNavigationGesture_nonGestureEvent_returnsNone()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kCGEventScrollWheel, false, true, 1.0, 0.0),
      NavigationGestureDirection::None
  );
}

void OSXComputerTests::classifyNavigationGesture_zeroDelta_returnsNone()
{
  QCOMPARE(
      OSXComputer::classifyNavigationGesture(kGestureType, false, true, 0.0, 0.0), NavigationGestureDirection::None
  );
}

void OSXComputerTests::navigationActionSlotForDirection_configuredDirections_returnsSlots()
{
  QCOMPARE(
      OSXComputer::navigationActionSlotForDirection(
          NavigationGestureDirection::Up, NavigationGestureDirection::Up, NavigationGestureDirection::Down
      ),
      NavigationActionSlot::Action1
  );
  QCOMPARE(
      OSXComputer::navigationActionSlotForDirection(
          NavigationGestureDirection::Down, NavigationGestureDirection::Up, NavigationGestureDirection::Down
      ),
      NavigationActionSlot::Action2
  );
  QCOMPARE(
      OSXComputer::navigationActionSlotForDirection(
          NavigationGestureDirection::Left, NavigationGestureDirection::Up, NavigationGestureDirection::Down
      ),
      NavigationActionSlot::None
  );
}

void OSXComputerTests::navigationDirectionsFromOptions_configuredOptions_returnsDirections()
{
  const OptionsList options = {
      kOptionMacNavigationGestureAction1, static_cast<uint32_t>(NavigationGestureDirection::Up),
      kOptionMacNavigationGestureAction2, static_cast<uint32_t>(NavigationGestureDirection::Down)
  };

  auto action1 = NavigationGestureDirection::Left;
  auto action2 = NavigationGestureDirection::Right;
  OSXComputer::navigationDirectionsFromOptions(options, action1, action2);

  QCOMPARE(action1, NavigationGestureDirection::Up);
  QCOMPARE(action2, NavigationGestureDirection::Down);
}

QTEST_MAIN(OSXComputerTests)
