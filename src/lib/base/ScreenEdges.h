/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

enum class Direction : uint8_t;

namespace deskflow {

inline constexpr int32_t VisibleEdgeBand = 8;

struct ScreenRect
{
  int32_t x = 0;
  int32_t y = 0;
  int32_t w = 0;
  int32_t h = 0;
};

struct ScreenEdgeInterval
{
  double start = 0.0;
  double end = 0.0;
};

bool projectToVisibleEdge(
    const std::vector<ScreenRect> &screens, uint32_t activeSides, int32_t edgeBandSize, const ScreenRect &bounds,
    int32_t &x, int32_t &y
);

bool projectFromVisibleEdge(const std::vector<ScreenRect> &screens, int32_t edgeInset, int32_t &x, int32_t &y);

bool remapToVisibleEdge(const std::vector<ScreenRect> &screens, const ScreenRect &bounds, int32_t &x, int32_t &y);

std::optional<ScreenEdgeInterval> visibleEdgeInterval(
    const std::vector<ScreenRect> &screens, const ScreenRect &bounds, const ScreenRect &screen, Direction side
);

bool projectToVisibleBottomEdge(
    const std::vector<ScreenRect> &screens, uint32_t activeSides, int32_t edgeBandSize, const ScreenRect &bounds,
    int32_t &x, int32_t &y
);

bool projectFromVisibleBottomEdge(const std::vector<ScreenRect> &screens, int32_t edgeInset, int32_t &x, int32_t &y);

std::optional<ScreenEdgeInterval>
visibleBottomEdgeInterval(const std::vector<ScreenRect> &screens, const ScreenRect &bounds, const ScreenRect &screen);

} // namespace deskflow
