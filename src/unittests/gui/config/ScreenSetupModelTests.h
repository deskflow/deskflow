/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Mikhail Slyusarev <slyusarevmikhail@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class ScreenSetupModelTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void moveKeepsLayoutConsistent();
  void invalidSpanDropRejected();
  void spanNudgesIntoOwnFootprint();
  void singleCellsSwap();
  void crossingSpansRejected();
};
