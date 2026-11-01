/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Lucas Magotteaux
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class OSXTrackpadZoomTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void touchFrameCountsTouchingFingers();
  void touchFrameIgnoresOtherEvents();
  void doubleTapToggles();
  void doubleTapAndDragZooms_data();
  void doubleTapAndDragZooms();
  void dragHoldsGesturesUntilLift();
  void dragBeginsAndEndsOnce_data();
  void dragBeginsAndEndsOnce();
  void notADoubleTap_data();
  void notADoubleTap();
  void staggeredLandingAndLift();
  void repeatedSamplesAreIgnored();
  void commandRoundTrip();
  void serializedEventsAreNotCommands();
  void toggleStepsReachTarget_data();
  void toggleStepsReachTarget();
  void dragCursorFollowsDirection();
  void dragCursorStaysWithinZoomLimits();

private:
  Log m_log;
};
