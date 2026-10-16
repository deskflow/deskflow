/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LayoutLinksTests.h"

#include "common/LayoutLinks.h"

using namespace deskflow::layout;

void LayoutLinksTests::computeLinks_sideBySideSameHeight_fullEdgeLinks()
{
  const QMap<QString, QRect> placed = {{"a", QRect(0, 0, 1920, 1080)}, {"b", QRect(1920, 0, 1920, 1080)}};

  const auto links = computeLinks(placed);

  QCOMPARE(links, QList<Link>({{"a", Side::Right, 0, 100, "b", 0, 100}, {"b", Side::Left, 0, 100, "a", 0, 100}}));
}

void LayoutLinksTests::computeLinks_halfOverlap_rangedLinks()
{
  const QMap<QString, QRect> placed = {{"a", QRect(0, 0, 1920, 1080)}, {"b", QRect(1920, 540, 1920, 1080)}};

  const auto links = computeLinks(placed);

  QCOMPARE(links, QList<Link>({{"a", Side::Right, 50, 100, "b", 0, 50}, {"b", Side::Left, 0, 50, "a", 50, 100}}));
}

void LayoutLinksTests::computeLinks_differentHeights_rangeOnTallerComputer()
{
  const QMap<QString, QRect> placed = {{"a", QRect(0, 0, 1000, 1000)}, {"b", QRect(1000, -500, 1000, 2000)}};

  const auto links = computeLinks(placed);

  QCOMPARE(links, QList<Link>({{"a", Side::Right, 0, 100, "b", 25, 75}, {"b", Side::Left, 25, 75, "a", 0, 100}}));
}

void LayoutLinksTests::computeLinks_twoNeighboursOnOneEdge_edgeSplit()
{
  const QMap<QString, QRect> placed = {
      {"a", QRect(0, 0, 100, 100)}, {"low", QRect(100, 50, 100, 50)}, {"high", QRect(100, 0, 100, 50)}
  };

  const auto links = computeLinks(placed);

  QCOMPARE(
      links, QList<Link>(
                 {{"a", Side::Right, 0, 50, "high", 0, 100},
                  {"a", Side::Right, 50, 100, "low", 0, 100},
                  {"high", Side::Left, 0, 100, "a", 0, 50},
                  {"high", Side::Down, 0, 100, "low", 0, 100},
                  {"low", Side::Left, 0, 100, "a", 50, 100},
                  {"low", Side::Up, 0, 100, "high", 0, 100}}
             )
  );
}

void LayoutLinksTests::computeLinks_stacked_upDownLinks()
{
  const QMap<QString, QRect> placed = {{"a", QRect(0, 100, 100, 100)}, {"b", QRect(0, 0, 100, 100)}};

  const auto links = computeLinks(placed);

  QCOMPARE(links, QList<Link>({{"a", Side::Up, 0, 100, "b", 0, 100}, {"b", Side::Down, 0, 100, "a", 0, 100}}));
}

void LayoutLinksTests::computeLinks_gapBetween_noLinks()
{
  const QMap<QString, QRect> placed = {{"a", QRect(0, 0, 100, 100)}, {"b", QRect(101, 0, 100, 100)}};

  const auto links = computeLinks(placed);

  QVERIFY(links.isEmpty());
}

void LayoutLinksTests::computeLinks_cornerTouch_noLinks()
{
  const QMap<QString, QRect> placed = {{"a", QRect(0, 0, 100, 100)}, {"b", QRect(100, 100, 100, 100)}};

  const auto links = computeLinks(placed);

  QVERIFY(links.isEmpty());
}

void LayoutLinksTests::snap_nearRightEdge_flushAndAligned()
{
  const QRect other(0, 0, 100, 100);

  const auto snapped = snap(QRect(105, 3, 100, 100), {other}, 10);

  QCOMPARE(snapped, QRect(100, 0, 100, 100));
}

void LayoutLinksTests::snap_nearTopEdge_flushAndAligned()
{
  const QRect other(0, 0, 100, 100);

  const auto snapped = snap(QRect(-7, -104, 100, 100), {other}, 10);

  QCOMPARE(snapped, QRect(0, -100, 100, 100));
}

void LayoutLinksTests::snap_farFromOthers_unchanged()
{
  const QRect moving(150, 30, 100, 100);

  const auto snapped = snap(moving, {QRect(0, 0, 100, 100)}, 10);

  QCOMPARE(snapped, moving);
}

void LayoutLinksTests::snapResize_nearNeighbourEdges_edgesMatched()
{
  const QRect other(200, 0, 100, 100);

  const auto snapped = snapResize(QRect(0, 0, 195, 104), {other}, 10);

  QCOMPARE(snapped, QRect(0, 0, 200, 100));
}

void LayoutLinksTests::snapResize_farFromOthers_unchanged()
{
  const QRect other(500, 500, 100, 100);

  const auto snapped = snapResize(QRect(0, 0, 195, 104), {other}, 10);

  QCOMPARE(snapped, QRect(0, 0, 195, 104));
}

void LayoutLinksTests::overlapsAny_intersecting_true()
{
  const QRect other(0, 0, 100, 100);

  const bool overlaps = overlapsAny(QRect(50, 50, 100, 100), {other});

  QVERIFY(overlaps);
}

void LayoutLinksTests::overlapsAny_sharedEdge_false()
{
  const QRect other(0, 0, 100, 100);

  const bool overlaps = overlapsAny(QRect(100, 0, 100, 100), {other});

  QVERIFY(!overlaps);
}

QTEST_MAIN(LayoutLinksTests)
