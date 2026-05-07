/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/ScreenEdges.h"

#include "base/DirectionTypes.h"

#include <algorithm>

namespace deskflow {

namespace {

struct EdgeSegment
{
  int32_t edge = 0;
  int32_t start = 0;
  int32_t end = 0;
};

int32_t right(const ScreenRect &rect)
{
  return rect.x + rect.w;
}

int32_t bottom(const ScreenRect &rect)
{
  return rect.y + rect.h;
}

bool contains(const ScreenRect &rect, int32_t x, int32_t y)
{
  return x >= rect.x && x < right(rect) && y >= rect.y && y < bottom(rect);
}

bool containsAny(const std::vector<ScreenRect> &screens, int32_t x, int32_t y)
{
  for (const auto &screen : screens) {
    if (contains(screen, x, y)) {
      return true;
    }
  }
  return false;
}

uint32_t sideMask(Direction side)
{
  switch (side) {
    using enum Direction;
  case Left:
    return static_cast<uint32_t>(DirectionMask::LeftMask);
  case Right:
    return static_cast<uint32_t>(DirectionMask::RightMask);
  case Top:
    return static_cast<uint32_t>(DirectionMask::TopMask);
  case Bottom:
    return static_cast<uint32_t>(DirectionMask::BottomMask);
  default:
    return 0;
  }
}

bool isHorizontal(Direction side)
{
  return side == Direction::Top || side == Direction::Bottom;
}

int32_t boundsStart(const ScreenRect &bounds, Direction side)
{
  return isHorizontal(side) ? bounds.x : bounds.y;
}

int32_t boundsSize(const ScreenRect &bounds, Direction side)
{
  return isHorizontal(side) ? bounds.w : bounds.h;
}

int32_t screenStart(const ScreenRect &screen, Direction side)
{
  return isHorizontal(side) ? screen.x : screen.y;
}

int32_t screenEnd(const ScreenRect &screen, Direction side)
{
  return isHorizontal(side) ? right(screen) : bottom(screen);
}

int32_t screenCenter(const ScreenRect &screen, Direction side)
{
  return screenStart(screen, side) + (screenEnd(screen, side) - screenStart(screen, side)) / 2;
}

int32_t edgeCoordinate(const ScreenRect &screen, Direction side)
{
  switch (side) {
    using enum Direction;
  case Left:
    return screen.x;
  case Right:
    return right(screen) - 1;
  case Top:
    return screen.y;
  case Bottom:
    return bottom(screen) - 1;
  default:
    return 0;
  }
}

int32_t outsideCoordinate(const ScreenRect &screen, Direction side)
{
  switch (side) {
    using enum Direction;
  case Left:
    return screen.x - 1;
  case Right:
    return right(screen);
  case Top:
    return screen.y - 1;
  case Bottom:
    return bottom(screen);
  default:
    return 0;
  }
}

int32_t projectedBoundsCoordinate(const ScreenRect &bounds, Direction side)
{
  switch (side) {
    using enum Direction;
  case Left:
    return bounds.x;
  case Right:
    return right(bounds) - 1;
  case Top:
    return bounds.y;
  case Bottom:
    return bottom(bounds) - 1;
  default:
    return 0;
  }
}

bool isAtBoundsEdge(const ScreenRect &bounds, Direction side, int32_t x, int32_t y)
{
  switch (side) {
    using enum Direction;
  case Left:
    return x <= bounds.x && y >= bounds.y && y < bottom(bounds);
  case Right:
    return x >= right(bounds) - 1 && y >= bounds.y && y < bottom(bounds);
  case Top:
    return y <= bounds.y && x >= bounds.x && x < right(bounds);
  case Bottom:
    return y >= bottom(bounds) - 1 && x >= bounds.x && x < right(bounds);
  default:
    return false;
  }
}

void setEdgeCoordinate(Direction side, int32_t edge, int32_t &x, int32_t &y)
{
  if (isHorizontal(side)) {
    y = edge;
  } else {
    x = edge;
  }
}

int32_t getAxisCoordinate(Direction side, int32_t x, int32_t y)
{
  return isHorizontal(side) ? x : y;
}

void setAxisCoordinate(Direction side, int32_t value, int32_t &x, int32_t &y)
{
  if (isHorizontal(side)) {
    x = value;
  } else {
    y = value;
  }
}

bool pointInEdgeBand(const ScreenRect &screen, Direction side, int32_t edgeBandSize, int32_t x, int32_t y)
{
  const int32_t bandSize = std::max<int32_t>(1, edgeBandSize);
  switch (side) {
    using enum Direction;
  case Left:
    return y >= screen.y && y < bottom(screen) && x >= screen.x && x < screen.x + bandSize;
  case Right:
    return y >= screen.y && y < bottom(screen) && x >= right(screen) - bandSize && x < right(screen);
  case Top:
    return x >= screen.x && x < right(screen) && y >= screen.y && y < screen.y + bandSize;
  case Bottom:
    return x >= screen.x && x < right(screen) && y >= bottom(screen) - bandSize && y < bottom(screen);
  default:
    return false;
  }
}

bool isVisibleEdgePoint(const std::vector<ScreenRect> &screens, const ScreenRect &screen, Direction side, int32_t axis)
{
  if (axis < screenStart(screen, side) || axis >= screenEnd(screen, side)) {
    return false;
  }

  if (isHorizontal(side)) {
    return !containsAny(screens, axis, outsideCoordinate(screen, side));
  }

  return !containsAny(screens, outsideCoordinate(screen, side), axis);
}

bool isVisibleEdge(
    const std::vector<ScreenRect> &screens, const ScreenRect &screen, Direction side, int32_t edgeBandSize, int32_t x,
    int32_t y
)
{
  return pointInEdgeBand(screen, side, edgeBandSize, x, y) &&
         isVisibleEdgePoint(screens, screen, side, getAxisCoordinate(side, x, y));
}

void subtractSegment(std::vector<EdgeSegment> &segments, const EdgeSegment &cover)
{
  std::vector<EdgeSegment> next;
  for (const auto &segment : segments) {
    if (cover.end <= segment.start || cover.start >= segment.end) {
      next.push_back(segment);
      continue;
    }
    if (cover.start > segment.start) {
      next.push_back({segment.edge, segment.start, cover.start});
    }
    if (cover.end < segment.end) {
      next.push_back({segment.edge, cover.end, segment.end});
    }
  }
  segments = next;
}

std::vector<EdgeSegment>
visibleEdgeSegments(const std::vector<ScreenRect> &screens, const ScreenRect &screen, Direction side)
{
  std::vector<EdgeSegment> segments{{edgeCoordinate(screen, side), screenStart(screen, side), screenEnd(screen, side)}};

  for (const auto &other : screens) {
    const int32_t outside = outsideCoordinate(screen, side);
    const bool coversOutsideLine = isHorizontal(side) ? (outside >= other.y && outside < bottom(other))
                                                      : (outside >= other.x && outside < right(other));
    if (!coversOutsideLine) {
      continue;
    }

    const EdgeSegment cover{
        edgeCoordinate(screen, side),
        std::max(screenStart(screen, side), screenStart(other, side)),
        std::min(screenEnd(screen, side), screenEnd(other, side)),
    };
    if (cover.start >= cover.end) {
      continue;
    }
    subtractSegment(segments, cover);
  }

  return segments;
}

std::vector<EdgeSegment> visibleEdgeSegments(const std::vector<ScreenRect> &screens, Direction side)
{
  std::vector<EdgeSegment> segments;
  for (const auto &screen : screens) {
    const auto screenSegments = visibleEdgeSegments(screens, screen, side);
    segments.insert(segments.end(), screenSegments.begin(), screenSegments.end());
  }

  std::sort(segments.begin(), segments.end(), [](const auto &lhs, const auto &rhs) {
    if (lhs.start != rhs.start) {
      return lhs.start < rhs.start;
    }
    if (lhs.end != rhs.end) {
      return lhs.end < rhs.end;
    }
    return lhs.edge < rhs.edge;
  });
  return segments;
}

std::optional<EdgeSegment>
preferredSegment(const std::vector<EdgeSegment> &segments, const ScreenRect &screen, Direction side)
{
  if (segments.empty()) {
    return std::nullopt;
  }

  const int32_t center = screenCenter(screen, side);
  auto best = segments.front();
  for (const auto &segment : segments) {
    if (center >= segment.start && center < segment.end) {
      return segment;
    }
    if (segment.end - segment.start > best.end - best.start) {
      best = segment;
    }
  }

  return best;
}

std::optional<EdgeSegment> segmentForFraction(const std::vector<EdgeSegment> &segments, double fraction)
{
  if (segments.empty()) {
    return std::nullopt;
  }

  int32_t totalLength = 0;
  for (const auto &segment : segments) {
    totalLength += segment.end - segment.start;
  }
  if (totalLength <= 0) {
    return std::nullopt;
  }

  const double clampedFraction = std::clamp(fraction, 0.0, 1.0);
  int32_t offset = static_cast<int32_t>(clampedFraction * totalLength);
  if (offset >= totalLength) {
    offset = totalLength - 1;
  }

  for (const auto &segment : segments) {
    const int32_t length = segment.end - segment.start;
    if (offset < length) {
      return {EdgeSegment{segment.edge, segment.start + offset, segment.end}};
    }
    offset -= length;
  }

  return segments.back();
}

bool projectFromVisibleEdge(
    const std::vector<ScreenRect> &screens, Direction side, int32_t edgeInset, int32_t &x, int32_t &y
)
{
  const int32_t inset = std::max<int32_t>(0, edgeInset);
  for (const auto &screen : screens) {
    const int32_t axis = getAxisCoordinate(side, x, y);
    if (!isVisibleEdgePoint(screens, screen, side, axis)) {
      continue;
    }

    switch (side) {
      using enum Direction;
    case Left:
      if (x < screen.x) {
        x = std::min(right(screen) - 1, screen.x + inset);
        return true;
      }
      break;
    case Right:
      if (x >= right(screen)) {
        x = std::max(screen.x, right(screen) - 1 - inset);
        return true;
      }
      break;
    case Top:
      if (y < screen.y) {
        y = std::min(bottom(screen) - 1, screen.y + inset);
        return true;
      }
      break;
    case Bottom:
      if (y >= bottom(screen)) {
        y = std::max(screen.y, bottom(screen) - 1 - inset);
        return true;
      }
      break;
    default:
      break;
    }
  }

  return false;
}

} // namespace

bool projectToVisibleEdge(
    const std::vector<ScreenRect> &screens, uint32_t activeSides, int32_t edgeBandSize, const ScreenRect &bounds,
    int32_t &x, int32_t &y
)
{
  bool projected = false;
  for (Direction side = Direction::FirstDirection; side <= Direction::LastDirection;
       side = static_cast<Direction>(static_cast<int>(side) + 1)) {
    if ((activeSides & sideMask(side)) == 0) {
      continue;
    }

    for (const auto &screen : screens) {
      if (!isVisibleEdge(screens, screen, side, edgeBandSize, x, y)) {
        continue;
      }

      const int32_t edge = projectedBoundsCoordinate(bounds, side);
      const int32_t oldX = x;
      const int32_t oldY = y;
      setEdgeCoordinate(side, edge, x, y);
      projected = projected || oldX != x || oldY != y;
      break;
    }
  }

  return projected;
}

bool projectFromVisibleEdge(const std::vector<ScreenRect> &screens, int32_t edgeInset, int32_t &x, int32_t &y)
{
  if (containsAny(screens, x, y)) {
    return false;
  }

  bool projected = false;
  for (Direction side = Direction::FirstDirection; side <= Direction::LastDirection;
       side = static_cast<Direction>(static_cast<int>(side) + 1)) {
    projected = projectFromVisibleEdge(screens, side, edgeInset, x, y) || projected;
  }

  return projected;
}

bool remapToVisibleEdge(const std::vector<ScreenRect> &screens, const ScreenRect &bounds, int32_t &x, int32_t &y)
{
  if (screens.empty() || bounds.w <= 0 || bounds.h <= 0) {
    return false;
  }

  for (Direction side = Direction::FirstDirection; side <= Direction::LastDirection;
       side = static_cast<Direction>(static_cast<int>(side) + 1)) {
    if (!isAtBoundsEdge(bounds, side, x, y)) {
      continue;
    }

    const auto segments = visibleEdgeSegments(screens, side);
    if (segments.empty()) {
      continue;
    }

    const int32_t axisStart = boundsStart(bounds, side);
    const int32_t size = boundsSize(bounds, side);
    const double fraction = size > 1 ? static_cast<double>(getAxisCoordinate(side, x, y) - axisStart) / size : 0.0;
    const auto segment = segmentForFraction(segments, fraction);
    if (!segment) {
      continue;
    }

    const int32_t oldX = x;
    const int32_t oldY = y;
    setEdgeCoordinate(side, segment->edge, x, y);
    setAxisCoordinate(side, segment->start, x, y);
    return oldX != x || oldY != y;
  }

  return false;
}

std::optional<ScreenEdgeInterval> visibleEdgeInterval(
    const std::vector<ScreenRect> &screens, const ScreenRect &bounds, const ScreenRect &screen, Direction side
)
{
  if (bounds.w <= 0 || bounds.h <= 0) {
    return std::nullopt;
  }

  switch (side) {
    using enum Direction;
  case Left:
    if (screen.x <= bounds.x) {
      return std::nullopt;
    }
    break;
  case Right:
    if (right(screen) >= right(bounds)) {
      return std::nullopt;
    }
    break;
  case Top:
    if (screen.y <= bounds.y) {
      return std::nullopt;
    }
    break;
  case Bottom:
    if (bottom(screen) >= bottom(bounds)) {
      return std::nullopt;
    }
    break;
  default:
    return std::nullopt;
  }

  const auto segment = preferredSegment(visibleEdgeSegments(screens, screen, side), screen, side);
  if (!segment) {
    return std::nullopt;
  }

  const int32_t size = boundsSize(bounds, side);
  ScreenEdgeInterval interval{
      100.0 * (segment->start - boundsStart(bounds, side)) / size,
      100.0 * (segment->end - boundsStart(bounds, side)) / size,
  };

  interval.start = std::clamp(interval.start, 0.0, 100.0);
  interval.end = std::clamp(interval.end, 0.0, 100.0);
  if (interval.start <= 0.0 && interval.end >= 100.0) {
    return std::nullopt;
  }

  return interval;
}

bool projectToVisibleBottomEdge(
    const std::vector<ScreenRect> &screens, uint32_t activeSides, int32_t edgeBandSize, const ScreenRect &bounds,
    int32_t &x, int32_t &y
)
{
  if ((activeSides & static_cast<uint32_t>(DirectionMask::BottomMask)) == 0) {
    return false;
  }

  return projectToVisibleEdge(screens, static_cast<uint32_t>(DirectionMask::BottomMask), edgeBandSize, bounds, x, y);
}

bool projectFromVisibleBottomEdge(const std::vector<ScreenRect> &screens, int32_t edgeInset, int32_t &x, int32_t &y)
{
  if (containsAny(screens, x, y)) {
    return false;
  }

  return projectFromVisibleEdge(screens, Direction::Bottom, edgeInset, x, y);
}

std::optional<ScreenEdgeInterval>
visibleBottomEdgeInterval(const std::vector<ScreenRect> &screens, const ScreenRect &bounds, const ScreenRect &screen)
{
  return visibleEdgeInterval(screens, bounds, screen, Direction::Bottom);
}

} // namespace deskflow
