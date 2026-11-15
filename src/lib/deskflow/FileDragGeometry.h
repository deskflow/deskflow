/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileDragPolicy.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

/**
 * @file FileDragGeometry.h
 * @brief Window placement for a file drag, kept free of any window system so it can be
 * tested on every platform.
 *
 * Rectangles are in the primary's global screen coordinates (y grows downwards, the main
 * display's top left corner is 0,0), as Deskflow uses them everywhere. Platforms whose
 * windows use another origin (macOS: bottom left, y up) convert with flipVertical().
 */

namespace deskflow::filedrag {

/**
 * The edge windows for \p side when the desktop \p desktop (the box around all displays)
 * is made of \p displays: one strip on every display whose edge is that side of the
 * desktop, so each strip lies on a single display (a window spanning two displays shows
 * on only one of them when every display has its own Space, as on macOS).
 *
 * With no display list the whole desktop edge is used (edgeStrip()).
 */
inline std::vector<EdgeRect> edgeStripsOnDisplays(
    Direction side, uint32_t sides, const EdgeRect &desktop, const std::vector<EdgeRect> &displays,
    int32_t thickness = kEdgeStripThickness
)
{
  std::vector<EdgeRect> strips;
  if (displays.empty()) {
    if (const auto strip = edgeStrip(side, sides, desktop, thickness)) {
      strips.push_back(*strip);
    }
    return strips;
  }
  for (const auto &display : displays) {
    bool touches = false;
    switch (side) {
    case Direction::Left:
      touches = display.x == desktop.x;
      break;
    case Direction::Right:
      touches = display.x + display.w == desktop.x + desktop.w;
      break;
    case Direction::Top:
      touches = display.y == desktop.y;
      break;
    case Direction::Bottom:
      touches = display.y + display.h == desktop.y + desktop.h;
      break;
    default:
      break;
    }
    if (!touches) {
      continue;
    }
    if (const auto strip = edgeStrip(side, sides, display, thickness)) {
      strips.push_back(*strip);
    }
  }
  return strips;
}

//! The display that contains the point, if any
inline std::optional<EdgeRect> displayAt(int32_t x, int32_t y, const std::vector<EdgeRect> &displays)
{
  for (const auto &display : displays) {
    if (x >= display.x && x < display.x + display.w && y >= display.y && y < display.y + display.h) {
      return display;
    }
  }
  return std::nullopt;
}

/**
 * A square of \p side centred on the point, moved (never shrunk) so it stays on
 * \p display. Used for the windows that start and park a drag under the cursor; the point
 * stays inside the square as long as it is on the display.
 */
inline EdgeRect squareAround(int32_t x, int32_t y, int32_t side, const std::optional<EdgeRect> &display)
{
  EdgeRect r{x - side / 2, y - side / 2, side, side};
  if (!display || display->w < side || display->h < side) {
    return r;
  }
  r.x = std::clamp(r.x, display->x, display->x + display->w - side);
  r.y = std::clamp(r.y, display->y, display->y + display->h - side);
  return r;
}

/**
 * Converts between top-left origin (y down) and bottom-left origin (y up) coordinates.
 * \p mainHeight is the height of the main display, whose top left corner is 0,0 in the
 * first and whose bottom left corner is 0,0 in the second. The conversion is its own
 * inverse.
 */
constexpr EdgeRect flipVertical(const EdgeRect &r, int32_t mainHeight)
{
  return EdgeRect{r.x, mainHeight - (r.y + r.h), r.w, r.h};
}

/**
 * True for the path of a macOS file reference URL ("file:///.file/id=6571367.2773272/").
 * Finder puts these on the drag pasteboard; they must be resolved to a real path before
 * use, and one that cannot be resolved is skipped.
 */
constexpr bool isFileReferencePath(std::string_view path)
{
  return path.starts_with("/.file/id=");
}

} // namespace deskflow::filedrag
