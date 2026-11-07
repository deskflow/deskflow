/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LayoutLinks.h"

#include <QStringList>

#include <algorithm>
#include <cstdlib>
#include <limits>

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

struct Span
{
  int start;
  int end;
};

Span edgeSpan(const QRect &r, Side side)
{
  if (side == Side::Right || side == Side::Left)
    return {r.y(), bottomEdge(r)};
  return {r.x(), rightEdge(r)};
}

double percent(int value, const Span &span)
{
  return (value - span.start) * 100.0 / (span.end - span.start);
}

// Negative when b isn't beyond a's edge on that side.
int gapTo(const QRect &a, const QRect &b, Side side)
{
  switch (side) {
  case Side::Right:
    return b.x() - rightEdge(a);
  case Side::Left:
    return a.x() - rightEdge(b);
  case Side::Up:
    return a.y() - bottomEdge(b);
  case Side::Down:
    return b.y() - bottomEdge(a);
  }
  return -1;
}

QList<Link> linksOnSide(const QString &from, const QMap<QString, QRect> &computers, Side side)
{
  const QRect fromRect = computers.value(from);
  const auto gap = [&](const QString &to) { return gapTo(fromRect, computers.value(to), side); };

  QStringList beyond;
  for (const auto &to : computers.keys()) {
    if (to != from && gap(to) >= 0)
      beyond.append(to);
  }
  std::ranges::stable_sort(beyond, {}, gap);

  const Span edge = edgeSpan(fromRect, side);
  QList<Span> uncovered = {edge};
  QList<Link> links;
  for (const auto &to : beyond) {
    const Span toEdge = edgeSpan(computers.value(to), side);
    QList<Span> remaining;
    for (const auto &free : uncovered) {
      const int start = std::max(free.start, toEdge.start);
      const int end = std::min(free.end, toEdge.end);
      if (end <= start) {
        remaining.append(free);
        continue;
      }
      links.append(
          {from, side, percent(start, edge), percent(end, edge), to, percent(start, toEdge), percent(end, toEdge)}
      );
      if (free.start < start)
        remaining.append({free.start, start});
      if (end < free.end)
        remaining.append({end, free.end});
    }
    uncovered = remaining;
  }
  std::ranges::sort(links, {}, &Link::fromStart);
  return links;
}

} // namespace

QList<Link> computeLinks(const QMap<QString, QRect> &computers)
{
  QList<Link> links;
  for (const auto &from : computers.keys()) {
    for (const auto side : {Side::Right, Side::Left, Side::Up, Side::Down})
      links.append(linksOnSide(from, computers, side));
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

QRect snapResize(const QRect &resized, Qt::Edges edges, const QList<QRect> &others, int threshold)
{
  QList<int> xs;
  QList<int> ys;
  for (const auto &other : others) {
    xs << other.x() << rightEdge(other);
    ys << other.y() << bottomEdge(other);
  }
  const auto nearest = [threshold](int edge, const QList<int> &candidates) {
    int best = edge;
    int bestDistance = threshold + 1;
    for (const int candidate : candidates) {
      if (std::abs(candidate - edge) < bestDistance) {
        bestDistance = std::abs(candidate - edge);
        best = candidate;
      }
    }
    return best;
  };

  const int left = edges & Qt::LeftEdge ? nearest(resized.x(), xs) : resized.x();
  const int top = edges & Qt::TopEdge ? nearest(resized.y(), ys) : resized.y();
  const int right = edges & Qt::RightEdge ? nearest(rightEdge(resized), xs) : rightEdge(resized);
  const int bottom = edges & Qt::BottomEdge ? nearest(bottomEdge(resized), ys) : bottomEdge(resized);
  if (right <= left || bottom <= top)
    return resized;
  return QRect(left, top, right - left, bottom - top);
}

bool overlapsAny(const QRect &rect, const QList<QRect> &others)
{
  return std::ranges::any_of(others, [&rect](const QRect &other) { return rect.intersects(other); });
}

QRect nearestFreeSpot(const QRect &rect, const QList<QRect> &others)
{
  if (!overlapsAny(rect, others))
    return rect;

  // Every free spot can slide until it touches a neighbour or is back in line with rect, so these are enough.
  // Right of all the others is always free, so something is always found.
  QList<int> xs = {rect.x()};
  QList<int> ys = {rect.y()};
  for (const auto &other : others) {
    xs << other.x() - rect.width() << rightEdge(other);
    ys << other.y() - rect.height() << bottomEdge(other);
  }
  QRect best;
  qint64 bestDistance = std::numeric_limits<qint64>::max();
  for (const int x : xs) {
    for (const int y : ys) {
      const qint64 dx = x - rect.x();
      const qint64 dy = y - rect.y();
      const QRect candidate(QPoint(x, y), rect.size());
      if (dx * dx + dy * dy < bestDistance && !overlapsAny(candidate, others)) {
        bestDistance = dx * dx + dy * dy;
        best = candidate;
      }
    }
  }
  return best;
}

} // namespace deskflow::layout
