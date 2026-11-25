/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class OSXComputerTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void continuousScrollKeepsSubNotchPrecision();
  void continuousScrollKeepsDirection();
  void lineScrollUsesNotchPath();
  void continuousScrollClampsToWireRange();
  void swipeRightIsBack();
  void swipeLeftIsForward();
  void shortStrokeIsNotSwipe();
  void verticalStrokeIsNotSwipe();
  void cancelledStrokeIsNotSwipe();
  void strokeWithoutBeganIsNotSwipe();
  void swipeThresholdIsSixtyPoints();
};
