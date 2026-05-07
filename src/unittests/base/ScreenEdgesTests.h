/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class ScreenEdgesTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void projectToVisibleBottomEdge_centerBottomWithLowerSideScreens_projectsToVirtualBottom();
  void projectToVisibleBottomEdge_centerBottomWithinEdgeBand_projectsToVirtualBottom();
  void projectToVisibleBottomEdge_internalScreenBoundary_preservesPosition();
  void projectToVisibleBottomEdge_inactiveBottomSide_preservesPosition();
  void projectToVisibleBottomEdge_rectangularDesktopBottom_preservesPosition();
  void projectToVisibleEdge_centerRightWithFartherTopAndBottomScreens_projectsToVirtualRight();
  void projectToVisibleEdge_centerLeftWithFartherTopAndBottomScreens_projectsToVirtualLeft();
  void projectToVisibleEdge_centerTopWithHigherSideScreens_projectsToVirtualTop();
  void projectFromVisibleBottomEdge_virtualBottomHole_projectsToScreenBottom();
  void projectFromVisibleEdge_virtualRightHole_projectsToScreenRight();
  void projectFromVisibleBottomEdge_positionInsideScreen_preservesPosition();
  void remapToVisibleEdge_leftBoundingEdge_remapsToVisibleMonitorEdge();
  void visibleBottomEdgeInterval_centerBottomWithLowerSideScreens_returnsCenterInterval();
  void visibleEdgeInterval_centerRightWithFartherTopAndBottomScreens_returnsCenterInterval();
};
