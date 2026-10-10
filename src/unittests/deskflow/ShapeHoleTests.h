/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Con Hennessy <con.hennessy@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class ShapeHoleTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void findShapeHoles_singleRegionFillingBounds_noHoles();
  void findShapeHoles_bottomRightCornerMissing_oneHoleFound();
  void findShapeHoles_topLeftCornerMissing_oneHoleFound();
  void findShapeHoles_bottomLeftCornerMissing_oneHoleFound();
  void findShapeHoles_topRightCornerMissing_oneHoleFound();
  void findShapeHoles_unequalMonitorSizes_holeFoundAtCorrectRect();
  void findShapeHoles_tShape_twoHolesFoundBesideStem();
  void findShapeHoles_notchInMiddleOfEdge_oneHoleFoundNotTouchingCorner();
  void findShapeHoles_interiorGap_noHoles();
  void findShapeHoles_nonRectangularConnectedGap_noHoles();
  void activeDirections_matchesEachLocation();
  void parseHoleLocation_roundTripsWithToString();
  void parseHoleLocation_unknownName_returnsNullopt();
};
