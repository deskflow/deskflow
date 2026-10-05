/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EiScrollStateTests.h"
#include "platform/EiScrollState.h"

using deskflow::EiScrollState;

void EiScrollStateTests::smoothAndDiscreteUseSeparateUnits()
{
  EiScrollState state;
  QCOMPARE(state.discrete(0, 60).y, 0);
  // A half-notch of discrete input must not become 60 smooth wheel clicks.
  QCOMPARE(state.smooth(0, 5).y, 0);
  QCOMPARE(state.discrete(0, 60).y, 120);
  QCOMPARE(state.smooth(0, 5).y, 120);
}

void EiScrollStateTests::repeatedMixedEvents()
{
  EiScrollState state;
  // Replay the same-direction event sequence reported in #10166.
  QCOMPARE(state.smooth(0, -15).y, -120);
  QCOMPARE(state.discrete(0, -15).y, 0);
  QCOMPARE(state.smooth(0, -15).y, -240);
  QCOMPARE(state.discrete(0, -15).y, 0);
  QCOMPARE(state.smooth(0, -15).y, -120);
  QCOMPARE(state.discrete(0, -15).y, 0);
}

void EiScrollStateTests::reverseMixedEvents_data()
{
  QTest::addColumn<int>("direction");
  QTest::newRow("up-to-down") << 1;
  QTest::newRow("down-to-up") << -1;
}

void EiScrollStateTests::reverseMixedEvents()
{
  QFETCH(int, direction);
  EiScrollState state;
  // Hyprland 0.56.2 sends a pixel delta followed by a discrete delta for
  // the same wheel movement. Replaying that sequence used to send the
  // first reversed movement in the previous direction.
  QCOMPARE(state.smooth(0, 15 * direction).y, 120 * direction);
  QCOMPARE(state.discrete(0, 15 * direction).y, 0);
  QCOMPARE(state.smooth(0, -15 * direction).y, -120 * direction);
  QCOMPARE(state.discrete(0, -15 * direction).y, 0);
}

void EiScrollStateTests::reverseSmoothDiscardsOldRemainder()
{
  EiScrollState state;
  QCOMPARE(state.smooth(0, 9).y, 0);
  QCOMPARE(state.smooth(0, -10).y, -120);
  QCOMPARE(state.smooth(0, -9).y, 0);
  QCOMPARE(state.smooth(0, 10).y, 120);
}

void EiScrollStateTests::reverseDiscreteDiscardsOldRemainder()
{
  EiScrollState state;
  QCOMPARE(state.discrete(0, 90).y, 0);
  QCOMPARE(state.discrete(0, -120).y, -120);
  QCOMPARE(state.discrete(0, -90).y, 0);
  QCOMPARE(state.discrete(0, 120).y, 120);
}

void EiScrollStateTests::accumulatePartialNotches()
{
  EiScrollState state;
  for (int i = 0; i < 3; ++i) {
    QCOMPARE(state.smooth(0, -2.5).y, 0);
    QCOMPARE(state.discrete(0, 30).y, 0);
  }
  QCOMPARE(state.smooth(0, -2.5).y, -120);
  QCOMPARE(state.discrete(0, 30).y, 120);
  QCOMPARE(state.smooth(0, 20).y, 240);
  QCOMPARE(state.discrete(0, -240).y, -240);
}

void EiScrollStateTests::axesRemainIndependent()
{
  EiScrollState state;
  state.smooth(5, 9);
  const auto smooth = state.smooth(5, -10);
  QCOMPARE(smooth.x, 120);
  QCOMPARE(smooth.y, -120);
  state.discrete(60, -90);
  const auto discrete = state.discrete(60, 120);
  QCOMPARE(discrete.x, 120);
  QCOMPARE(discrete.y, 120);
}

void EiScrollStateTests::stopResetsOnlySpecifiedAxis()
{
  EiScrollState state;
  state.smooth(5, 5);
  state.discrete(60, 60);
  state.reset(false, true);
  const auto smooth = state.smooth(5, 5);
  QCOMPARE(smooth.x, 120);
  QCOMPARE(smooth.y, 0);
  const auto discrete = state.discrete(60, 60);
  QCOMPARE(discrete.x, 120);
  QCOMPARE(discrete.y, 0);
}

void EiScrollStateTests::captureEndClearsBothEventTypes()
{
  EiScrollState state;
  state.smooth(5, -5);
  state.discrete(-60, 60);
  state.reset();
  const auto smooth = state.smooth(5, -5);
  QCOMPARE(smooth.x, 0);
  QCOMPARE(smooth.y, 0);
  const auto discrete = state.discrete(-60, 60);
  QCOMPARE(discrete.x, 0);
  QCOMPARE(discrete.y, 0);
}

QTEST_GUILESS_MAIN(EiScrollStateTests)
