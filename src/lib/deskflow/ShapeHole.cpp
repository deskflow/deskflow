/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Con Hennessy <con.hennessy@gmail.com> 
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ShapeHole.h"

#include <algorithm>
#include <array>
#include <queue>
#include <utility>

namespace deskflow {

namespace {

bool isCovered(const std::vector<Rect> &regions, std::int64_t x, std::int64_t y)
{
  return std::ranges::any_of(regions, [x, y](const Rect &r) {
    return r.x <= x && x < r.x + r.w && r.y <= y && y < r.y + r.h;
  });
}

} // namespace

std::optional<HoleLocation> parseHoleLocation(const std::string &name)
{
  static const std::array<const char *, 8> kNames{
      "top", "bottom", "left", "right", "top-left", "top-right", "bottom-left", "bottom-right"
  };
  if (std::ranges::find(kNames, name) == kNames.end()) {
    return std::nullopt;
  }
  return name;
}

std::vector<Direction> activeDirections(const HoleLocation &location)
{
  // for each side of the hole that does NOT coincide with an outer wall of
  // the bounding box, there's real screen immediately beyond it -- crossing
  // that side is a valid switch, in the direction you'd be leaving via if
  // that real screen were its own computer. e.g. a "bottom-right" hole
  // doesn't touch the left or top walls, so its left and top sides each
  // border real screen: approaching from the left and moving right into the
  // hole is a Right-direction switch; approaching from above and moving
  // down is a Bottom-direction switch. (A hole's own left/top/right/bottom
  // side maps to the opposite direction name, since "the hole's left side"
  // is where you'd be moving right to reach it.)
  using enum Direction;
  if (location == "top") {
    return {Left, Right, Top};
  }
  if (location == "bottom") {
    return {Left, Right, Bottom};
  }
  if (location == "left") {
    return {Top, Bottom, Left};
  }
  if (location == "right") {
    return {Top, Bottom, Right};
  }
  if (location == "top-left") {
    return {Left, Top};
  }
  if (location == "top-right") {
    return {Right, Top};
  }
  if (location == "bottom-left") {
    return {Left, Bottom};
  }
  if (location == "bottom-right") {
    return {Right, Bottom};
  }
  return {};
}

std::vector<ShapeHole> findShapeHoles(const std::vector<Rect> &regions, const Rect &bounds)
{
  std::vector<std::int64_t> xs{bounds.x, bounds.x + bounds.w};
  std::vector<std::int64_t> ys{bounds.y, bounds.y + bounds.h};
  for (const auto &r : regions) {
    xs.push_back(r.x);
    xs.push_back(r.x + r.w);
    ys.push_back(r.y);
    ys.push_back(r.y + r.h);
  }
  std::ranges::sort(xs);
  xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
  std::ranges::sort(ys);
  ys.erase(std::unique(ys.begin(), ys.end()), ys.end());

  if (xs.size() < 2 || ys.size() < 2) {
    return {};
  }

  const auto numCols = xs.size() - 1;
  const auto numRows = ys.size() - 1;

  // precompute which grid cells (identified by their top-left corner, a
  // representative point since no region edge falls strictly inside a cell)
  // are covered by a region
  std::vector<std::vector<bool>> covered(numCols, std::vector<bool>(numRows));
  for (std::size_t i = 0; i < numCols; ++i) {
    for (std::size_t j = 0; j < numRows; ++j) {
      covered[i][j] = isCovered(regions, xs[i], ys[j]);
    }
  }

  std::vector<std::vector<bool>> visited(numCols, std::vector<bool>(numRows, false));
  std::vector<ShapeHole> holes;
  static constexpr std::array<std::pair<std::int64_t, std::int64_t>, 4> kNeighborOffsets{
      {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}
  };

  for (std::size_t startI = 0; startI < numCols; ++startI) {
    for (std::size_t startJ = 0; startJ < numRows; ++startJ) {
      if (covered[startI][startJ] || visited[startI][startJ]) {
        continue;
      }

      // flood fill this connected component of uncovered cells
      auto minI = startI;
      auto maxI = startI;
      auto minJ = startJ;
      auto maxJ = startJ;
      std::size_t cellCount = 0;
      std::queue<std::pair<std::size_t, std::size_t>> pending;
      pending.push({startI, startJ});
      visited[startI][startJ] = true;
      while (!pending.empty()) {
        const auto [i, j] = pending.front();
        pending.pop();
        ++cellCount;
        minI = std::min(minI, i);
        maxI = std::max(maxI, i);
        minJ = std::min(minJ, j);
        maxJ = std::max(maxJ, j);

        for (const auto &[di, dj] : kNeighborOffsets) {
          const auto ni = static_cast<std::int64_t>(i) + di;
          const auto nj = static_cast<std::int64_t>(j) + dj;
          if (ni < 0 || nj < 0 || static_cast<std::size_t>(ni) >= numCols ||
              static_cast<std::size_t>(nj) >= numRows) {
            continue;
          }
          const auto ui = static_cast<std::size_t>(ni);
          const auto uj = static_cast<std::size_t>(nj);
          if (covered[ui][uj] || visited[ui][uj]) {
            continue;
          }
          visited[ui][uj] = true;
          pending.push({ui, uj});
        }
      }

      // this component must be a single clean rectangle: every cell inside
      // its own index bounding box must be part of it (i.e. also uncovered)
      const auto expectedCount = (maxI - minI + 1) * (maxJ - minJ + 1);
      if (cellCount != expectedCount) {
        continue;
      }

      const Rect rect{xs[minI], ys[minJ], xs[maxI + 1] - xs[minI], ys[maxJ + 1] - ys[minJ]};

      const bool touchesLeft = rect.x == bounds.x;
      const bool touchesRight = rect.x + rect.w == bounds.x + bounds.w;
      const bool touchesTop = rect.y == bounds.y;
      const bool touchesBottom = rect.y + rect.h == bounds.y + bounds.h;
      const int wallCount =
          (touchesLeft ? 1 : 0) + (touchesRight ? 1 : 0) + (touchesTop ? 1 : 0) + (touchesBottom ? 1 : 0);

      HoleLocation location;
      if (wallCount == 1) {
        location = touchesLeft ? "left" : touchesRight ? "right" : touchesTop ? "top" : "bottom";
      } else if (touchesLeft != touchesRight && touchesTop != touchesBottom) {
        // exactly one horizontal and one vertical wall: an adjacent pair, i.e. a corner
        if (touchesTop) {
          location = touchesLeft ? "top-left" : "top-right";
        } else {
          location = touchesLeft ? "bottom-left" : "bottom-right";
        }
      } else {
        // no outer wall at all, an opposite pair of walls (the gap spans the
        // full width or height), or three-plus walls -- not a supported shape
        continue;
      }

      holes.push_back({rect, location});
    }
  }

  return holes;
}

} // namespace deskflow
