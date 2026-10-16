/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class LayoutLinksTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void computeLinks_sideBySideSameHeight_fullEdgeLinks();
  void computeLinks_halfOverlap_rangedLinks();
  void computeLinks_differentHeights_rangeOnTallerComputer();
  void computeLinks_twoNeighboursOnOneEdge_edgeSplit();
  void computeLinks_stacked_upDownLinks();
  void computeLinks_gapBetween_noLinks();
  void computeLinks_cornerTouch_noLinks();
  void snap_nearRightEdge_flushAndAligned();
  void snap_nearTopEdge_flushAndAligned();
  void snap_farFromOthers_unchanged();
  void snapResize_nearNeighbourEdges_edgesMatched();
  void snapResize_farFromOthers_unchanged();
  void overlapsAny_intersecting_true();
  void overlapsAny_sharedEdge_false();
};
