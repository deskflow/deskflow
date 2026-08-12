/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class OSXScreenTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void navigationGesturesEnabledFromOptions_enabledOption_returnsTrue();
  void navigationGesturesEnabledFromOptions_disabledOption_returnsFalse();
  void navigationGesturesEnabledFromOptions_unrelatedOption_preservesCurrentValue();
  void navigationGesturesEnabledFromOptions_malformedOptions_preservesCurrentValue();
  void classifyNavigationGestureButton_swipeLeft_returnsExtra0();
  void classifyNavigationGestureButton_swipeRight_returnsExtra1();
  void classifyNavigationGestureButton_disabled_returnsNone();
  void classifyNavigationGestureButton_localScreen_returnsNone();
  void classifyNavigationGestureButton_nonGestureEvent_returnsNone();
  void classifyNavigationGestureButton_unknownDirection_returnsNone();
};
