/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LayoutLinksTests.h"

#include "gui/config/LayoutLinks.h"

using namespace deskflow::gui::layout;

namespace {
QStringList format(const QList<Link> &links)
{
  QStringList out;
  for (const auto &link : links)
    out.append(QStringLiteral("%1: %2").arg(link.from, formatLink(link)));
  return out;
}
} // namespace

void LayoutLinksTests::fullEdge()
{
  const auto links = computeLinks({{"a", QRect(0, 0, 1920, 1080)}, {"b", QRect(1920, 0, 1920, 1080)}});
  QCOMPARE(format(links), QStringList({"a: right = b", "b: left = a"}));
}

void LayoutLinksTests::halfEdge()
{
  const auto links = computeLinks({{"a", QRect(0, 0, 1920, 1080)}, {"b", QRect(1920, 540, 1920, 1080)}});
  QCOMPARE(format(links), QStringList({"a: right(50,100) = b(0,50)", "b: left(0,50) = a(50,100)"}));
}

void LayoutLinksTests::differentHeights()
{
  const auto links = computeLinks({{"a", QRect(0, 0, 1000, 1000)}, {"b", QRect(1000, -500, 1000, 2000)}});
  QCOMPARE(format(links), QStringList({"a: right = b(25,75)", "b: left(25,75) = a"}));
}

void LayoutLinksTests::twoOnOneSide()
{
  const auto links = computeLinks(
      {{"a", QRect(0, 0, 100, 100)}, {"low", QRect(100, 50, 100, 50)}, {"high", QRect(100, 0, 100, 50)}}
  );
  QCOMPARE(
      format(links), QStringList({"a: right(0,50) = high", "a: right(50,100) = low", "low: left = a(50,100)",
                                  "low: up = high", "high: left = a(0,50)", "high: down = low"})
  );
}

void LayoutLinksTests::upDown()
{
  const auto links = computeLinks({{"a", QRect(0, 100, 100, 100)}, {"b", QRect(0, 0, 100, 100)}});
  QCOMPARE(format(links), QStringList({"a: up = b", "b: down = a"}));
}

void LayoutLinksTests::gap()
{
  QVERIFY(computeLinks({{"a", QRect(0, 0, 100, 100)}, {"b", QRect(101, 0, 100, 100)}}).isEmpty());
}

void LayoutLinksTests::cornerOnly()
{
  QVERIFY(computeLinks({{"a", QRect(0, 0, 100, 100)}, {"b", QRect(100, 100, 100, 100)}}).isEmpty());
}

void LayoutLinksTests::snapFlush()
{
  const QRect other(0, 0, 100, 100);
  QCOMPARE(snap(QRect(105, 3, 100, 100), {other}, 10), QRect(100, 0, 100, 100));
  QCOMPARE(snap(QRect(-7, -104, 100, 100), {other}, 10), QRect(0, -100, 100, 100));
}

void LayoutLinksTests::snapNoneWhenFar()
{
  QCOMPARE(snap(QRect(150, 30, 100, 100), {QRect(0, 0, 100, 100)}, 10), QRect(150, 30, 100, 100));
}

void LayoutLinksTests::overlap()
{
  QVERIFY(overlapsAny(QRect(50, 50, 100, 100), {QRect(0, 0, 100, 100)}));
  QVERIFY(!overlapsAny(QRect(100, 0, 100, 100), {QRect(0, 0, 100, 100)}));
}

QTEST_MAIN(LayoutLinksTests)
