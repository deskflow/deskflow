/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class EiScrollStateTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void smoothAndDiscreteUseSeparateUnits();
  void repeatedMixedEvents();
  void reverseMixedEvents_data();
  void reverseMixedEvents();
  void reverseSmoothDiscardsOldRemainder();
  void reverseDiscreteDiscardsOldRemainder();
  void accumulatePartialNotches();
  void axesRemainIndependent();
  void stopResetsOnlySpecifiedAxis();
  void captureEndClearsBothEventTypes();
};
