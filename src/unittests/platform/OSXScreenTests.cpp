/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXScreenTests.h"

#include "platform/OSXScreen.h"

namespace {

// NSEventTypeGesture is unavailable to this pure C++ test target.
constexpr auto kGestureType = static_cast<CGEventType>(29);

} // namespace

void OSXScreenTests::navigationGesturesEnabledFromOptions_enabledOption_returnsTrue()
{
  const OptionsList enabled = {kOptionMacNavigationGestures, 1};

  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(enabled, false));
}

void OSXScreenTests::navigationGesturesEnabledFromOptions_disabledOption_returnsFalse()
{
  const OptionsList disabled = {kOptionMacNavigationGestures, 0};

  QVERIFY(!OSXScreen::navigationGesturesEnabledFromOptions(disabled, true));
}

void OSXScreenTests::navigationGesturesEnabledFromOptions_unrelatedOption_preservesCurrentValue()
{
  const OptionsList unrelated = {kOptionClipboardSharing, 1};

  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(unrelated, true));
}

void OSXScreenTests::navigationGesturesEnabledFromOptions_malformedOptions_preservesCurrentValue()
{
  const OptionsList malformed = {kOptionMacNavigationGestures};

  QVERIFY(OSXScreen::navigationGesturesEnabledFromOptions(malformed, true));
}

void OSXScreenTests::classifyNavigationGestureButton_swipeLeft_returnsExtra0()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, true, 4), kButtonExtra0);
}

void OSXScreenTests::classifyNavigationGestureButton_swipeRight_returnsExtra1()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, true, 8), kButtonExtra1);
}

void OSXScreenTests::classifyNavigationGestureButton_disabled_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, false, 4), kButtonNone);
}

void OSXScreenTests::classifyNavigationGestureButton_localScreen_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, true, true, 4), kButtonNone);
}

void OSXScreenTests::classifyNavigationGestureButton_nonGestureEvent_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kCGEventScrollWheel, false, true, 4), kButtonNone);
}

void OSXScreenTests::classifyNavigationGestureButton_unknownDirection_returnsNone()
{
  QCOMPARE(OSXScreen::classifyNavigationGestureButton(kGestureType, false, true, 12), kButtonNone);
}

QTEST_MAIN(OSXScreenTests)
