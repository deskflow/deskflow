/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenEdgesTests.h"

#include "base/DirectionTypes.h"
#include "base/ScreenEdges.h"

namespace {

uint32_t bottomSide()
{
  return static_cast<int>(DirectionMask::BottomMask);
}

uint32_t side(DirectionMask side)
{
  return static_cast<int>(side);
}

std::vector<deskflow::ScreenRect> unevenScreens()
{
  return {
      {-1080, -397, 1080, 1920},
      {0, 0, 1920, 1080},
      {1920, -471, 1080, 1920},
  };
}

deskflow::ScreenRect unevenBounds()
{
  return {-1080, -471, 4080, 1994};
}

} // namespace

void ScreenEdgesTests::projectToVisibleBottomEdge_centerBottomWithLowerSideScreens_projectsToVirtualBottom()
{
  const auto screens = unevenScreens();
  const auto bounds = unevenBounds();
  int32_t x = 960;
  int32_t y = 1079;

  const bool projected = deskflow::projectToVisibleBottomEdge(screens, bottomSide(), 1, bounds, x, y);

  QVERIFY(projected);
  QCOMPARE(x, 960);
  QCOMPARE(y, 1522);
}

void ScreenEdgesTests::projectToVisibleBottomEdge_centerBottomWithinEdgeBand_projectsToVirtualBottom()
{
  const auto screens = unevenScreens();
  const auto bounds = unevenBounds();
  int32_t x = 960;
  int32_t y = 1074;

  const bool projected = deskflow::projectToVisibleBottomEdge(screens, bottomSide(), 8, bounds, x, y);

  QVERIFY(projected);
  QCOMPARE(x, 960);
  QCOMPARE(y, 1522);
}

void ScreenEdgesTests::projectToVisibleBottomEdge_internalScreenBoundary_preservesPosition()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {0, 0, 100, 100},
      {0, 100, 100, 100},
  };
  int32_t x = 50;
  int32_t y = 99;

  const bool projected = deskflow::projectToVisibleBottomEdge(screens, bottomSide(), 8, {0, 0, 100, 200}, x, y);

  QVERIFY(!projected);
  QCOMPARE(x, 50);
  QCOMPARE(y, 99);
}

void ScreenEdgesTests::projectToVisibleBottomEdge_inactiveBottomSide_preservesPosition()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {-100, 0, 100, 200},
      {0, 0, 100, 100},
  };
  int32_t x = 50;
  int32_t y = 99;

  const bool projected = deskflow::projectToVisibleBottomEdge(screens, 0, 8, {-100, 0, 200, 200}, x, y);

  QVERIFY(!projected);
  QCOMPARE(x, 50);
  QCOMPARE(y, 99);
}

void ScreenEdgesTests::projectToVisibleBottomEdge_rectangularDesktopBottom_preservesPosition()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {0, 0, 100, 100},
      {100, 0, 100, 100},
  };
  int32_t x = 50;
  int32_t y = 99;

  const bool projected = deskflow::projectToVisibleBottomEdge(screens, bottomSide(), 8, {0, 0, 200, 100}, x, y);

  QVERIFY(!projected);
  QCOMPARE(x, 50);
  QCOMPARE(y, 99);
}

void ScreenEdgesTests::projectToVisibleEdge_centerRightWithFartherTopAndBottomScreens_projectsToVirtualRight()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {0, -100, 150, 100},
      {0, 0, 100, 100},
      {0, 100, 150, 100},
  };
  int32_t x = 99;
  int32_t y = 50;

  const bool projected =
      deskflow::projectToVisibleEdge(screens, side(DirectionMask::RightMask), 8, {0, -100, 150, 300}, x, y);

  QVERIFY(projected);
  QCOMPARE(x, 149);
  QCOMPARE(y, 50);
}

void ScreenEdgesTests::projectToVisibleEdge_centerLeftWithFartherTopAndBottomScreens_projectsToVirtualLeft()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {-50, -100, 150, 100},
      {0, 0, 100, 100},
      {-50, 100, 150, 100},
  };
  int32_t x = 0;
  int32_t y = 50;

  const bool projected =
      deskflow::projectToVisibleEdge(screens, side(DirectionMask::LeftMask), 8, {-50, -100, 150, 300}, x, y);

  QVERIFY(projected);
  QCOMPARE(x, -50);
  QCOMPARE(y, 50);
}

void ScreenEdgesTests::projectToVisibleEdge_centerTopWithHigherSideScreens_projectsToVirtualTop()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {-50, -50, 50, 150},
      {0, 0, 100, 100},
      {100, -50, 50, 150},
  };
  int32_t x = 50;
  int32_t y = 0;

  const bool projected =
      deskflow::projectToVisibleEdge(screens, side(DirectionMask::TopMask), 8, {-50, -50, 200, 150}, x, y);

  QVERIFY(projected);
  QCOMPARE(x, 50);
  QCOMPARE(y, -50);
}

void ScreenEdgesTests::projectFromVisibleBottomEdge_virtualBottomHole_projectsToScreenBottom()
{
  const auto screens = unevenScreens();
  int32_t x = 960;
  int32_t y = 1521;

  const bool projected = deskflow::projectFromVisibleBottomEdge(screens, 8, x, y);

  QVERIFY(projected);
  QCOMPARE(x, 960);
  QCOMPARE(y, 1071);
}

void ScreenEdgesTests::projectFromVisibleEdge_virtualRightHole_projectsToScreenRight()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {0, -100, 150, 100},
      {0, 0, 100, 100},
      {0, 100, 150, 100},
  };
  int32_t x = 149;
  int32_t y = 50;

  const bool projected = deskflow::projectFromVisibleEdge(screens, 8, x, y);

  QVERIFY(projected);
  QCOMPARE(x, 91);
  QCOMPARE(y, 50);
}

void ScreenEdgesTests::projectFromVisibleBottomEdge_positionInsideScreen_preservesPosition()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {0, 0, 1920, 1080},
  };
  int32_t x = 960;
  int32_t y = 1078;

  const bool projected = deskflow::projectFromVisibleBottomEdge(screens, 1, x, y);

  QVERIFY(!projected);
  QCOMPARE(x, 960);
  QCOMPARE(y, 1078);
}

void ScreenEdgesTests::visibleBottomEdgeInterval_centerBottomWithLowerSideScreens_returnsCenterInterval()
{
  const auto screens = unevenScreens();
  const auto bounds = unevenBounds();
  const auto interval = deskflow::visibleBottomEdgeInterval(screens, bounds, screens[1]);

  QVERIFY(interval);
  QCOMPARE(interval->start, 26.470588235294116);
  QCOMPARE(interval->end, 73.529411764705884);
}

void ScreenEdgesTests::visibleEdgeInterval_centerRightWithFartherTopAndBottomScreens_returnsCenterInterval()
{
  const std::vector<deskflow::ScreenRect> screens = {
      {0, -100, 150, 100},
      {0, 0, 100, 100},
      {0, 100, 150, 100},
  };
  const auto interval = deskflow::visibleEdgeInterval(screens, {0, -100, 150, 300}, screens[1], Direction::Right);

  QVERIFY(interval);
  QCOMPARE(interval->start, 33.333333333333336);
  QCOMPARE(interval->end, 66.666666666666671);
}

QTEST_MAIN(ScreenEdgesTests)
