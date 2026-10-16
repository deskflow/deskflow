/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LayoutLinks.h"

#include <algorithm>
#include <cstdlib>

namespace deskflow::layout {

namespace {

// QRect::right() and bottom() are inclusive, edges here are exclusive.
int rightEdge(const QRect &r)
{
  return r.x() + r.width();
}

int bottomEdge(const QRect &r)
{
  return r.y() + r.height();
}

double percent(int value, int start, int length)
{
  return (value - start) * 100.0 / length;
}

bool linkOnSide(const QString &a, const QRect &ra, const QString &b, const QRect &rb, Side side, Link &link)
{
  const bool horizontal = side == Side::Right || side == Side::Left;

  bool touching = false;
  switch (side) {
  case Side::Right:
    touching = rightEdge(ra) == rb.x();
    break;
  case Side::Left:
    touching = ra.x() == rightEdge(rb);
    break;
  case Side::Up:
    touching = ra.y() == bottomEdge(rb);
    break;
  case Side::Down:
    touching = bottomEdge(ra) == rb.y();
    break;
  }
  if (!touching)
    return false;

  const int aStart = horizontal ? ra.y() : ra.x();
  const int aLength = horizontal ? ra.height() : ra.width();
  const int bStart = horizontal ? rb.y() : rb.x();
  const int bLength = horizontal ? rb.height() : rb.width();
  const int overlapStart = std::max(aStart, bStart);
  const int overlapEnd = std::min(aStart + aLength, bStart + bLength);
  if (overlapEnd <= overlapStart)
    return false;

  link = {
      a,
      side,
      percent(overlapStart, aStart, aLength),
      percent(overlapEnd, aStart, aLength),
      b,
      percent(overlapStart, bStart, bLength),
      percent(overlapEnd, bStart, bLength)
  };
  return true;
}

} // namespace

QList<Link> computeLinks(const QMap<QString, QRect> &computers)
{
  QList<Link> links;
  for (const auto &[from, fromRect] : computers.asKeyValueRange()) {
    for (const auto side : {Side::Right, Side::Left, Side::Up, Side::Down}) {
      QList<Link> sideLinks;
      for (const auto &[to, toRect] : computers.asKeyValueRange()) {
        Link link;
        if (from != to && linkOnSide(from, fromRect, to, toRect, side, link))
          sideLinks.append(link);
      }
      std::ranges::sort(sideLinks, {}, &Link::fromStart);
      links.append(sideLinks);
    }
  }
  return links;
}

QRect snap(const QRect &moving, const QList<QRect> &others, int threshold)
{
  int bestDistance = threshold + 1;
  QPoint bestOffset;
  const QRect *neighbour = nullptr;
  bool neighbourIsHorizontal = false;

  for (const auto &other : others) {
    const bool facingHorizontally =
        moving.y() < bottomEdge(other) + threshold && other.y() < bottomEdge(moving) + threshold;
    const bool facingVertically =
        moving.x() < rightEdge(other) + threshold && other.x() < rightEdge(moving) + threshold;

    const auto consider = [&](int delta, bool horizontal) {
      if (std::abs(delta) < bestDistance) {
        bestDistance = std::abs(delta);
        bestOffset = horizontal ? QPoint(delta, 0) : QPoint(0, delta);
        neighbour = &other;
        neighbourIsHorizontal = horizontal;
      }
    };
    if (facingHorizontally) {
      consider(rightEdge(other) - moving.x(), true);
      consider(other.x() - rightEdge(moving), true);
    }
    if (facingVertically) {
      consider(bottomEdge(other) - moving.y(), false);
      consider(other.y() - bottomEdge(moving), false);
    }
  }

  if (neighbour == nullptr)
    return moving;

  QRect result = moving.translated(bestOffset);

  const int startDelta = neighbourIsHorizontal ? neighbour->y() - result.y() : neighbour->x() - result.x();
  const int endDelta =
      neighbourIsHorizontal ? bottomEdge(*neighbour) - bottomEdge(result) : rightEdge(*neighbour) - rightEdge(result);
  const int align = std::abs(startDelta) <= std::abs(endDelta) ? startDelta : endDelta;
  if (std::abs(align) <= threshold)
    result.translate(neighbourIsHorizontal ? QPoint(0, align) : QPoint(align, 0));

  return result;
}

QRect snapResize(const QRect &resized, const QList<QRect> &others, int threshold)
{
  int right = rightEdge(resized);
  int bottom = bottomEdge(resized);
  int bestX = threshold + 1;
  int bestY = threshold + 1;
  for (const auto &other : others) {
    for (const int x : {other.x(), rightEdge(other)}) {
      if (x > resized.x() && std::abs(x - rightEdge(resized)) < bestX) {
        bestX = std::abs(x - rightEdge(resized));
        right = x;
      }
    }
    for (const int y : {other.y(), bottomEdge(other)}) {
      if (y > resized.y() && std::abs(y - bottomEdge(resized)) < bestY) {
        bestY = std::abs(y - bottomEdge(resized));
        bottom = y;
      }
    }
  }
  return QRect(resized.topLeft(), QSize(right - resized.x(), bottom - resized.y()));
}

bool overlapsAny(const QRect &rect, const QList<QRect> &others)
{
  return std::ranges::any_of(others, [&rect](const QRect &other) { return rect.intersects(other); });
}

} // namespace deskflow::layout
