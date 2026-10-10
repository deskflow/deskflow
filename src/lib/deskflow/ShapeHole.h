/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/DirectionTypes.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace deskflow {

//! An axis-aligned rectangle, in logical output coordinates
struct Rect
{
  std::int64_t x = 0;
  std::int64_t y = 0;
  std::int64_t w = 0;
  std::int64_t h = 0;

  bool operator==(const Rect &) const = default;
};

//! Which outer wall(s) of the bounding box a shape hole touches
/*!
A hole touching one wall (e.g. Bottom) sits in the middle of that edge, with
real screen on both of its other sides, e.g. the two gaps either side of the
stem of a T-shaped arrangement of monitors.  A hole touching two adjacent
walls (e.g. BottomRight) is a missing corner, e.g. the fourth quadrant of an
L-shaped arrangement.
*/
enum class HoleLocation : uint8_t
{
  Top,
  Bottom,
  Left,
  Right,
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
};

//! Human-readable name for a hole location, e.g. "bottom-right" (for logging
//! and for referring to a hole by name in the server config file)
const char *toString(HoleLocation location);

//! Parse a hole location from its name, e.g. "bottom-right"
/*!
The inverse of toString().  Returns std::nullopt if \c name doesn't match
any location.
*/
std::optional<HoleLocation> parseHoleLocation(const std::string &name);

//! The directions from which a hole at `location` can be entered or exited
/*!
A hole touching one outer wall has real screen on its other three sides, so
it can be approached from three directions; a hole touching two adjacent
walls (a corner) can be approached from two.
*/
std::vector<Direction> activeDirections(HoleLocation location);

//! A single rectangular gap in an otherwise-rectangular arrangement of screens
struct ShapeHole
{
  Rect rect;
  HoleLocation location = HoleLocation::Top;

  bool operator==(const ShapeHole &) const = default;
};

//! Find every rectangular gap in a monitor arrangement
/*!
Finds every rectangular gap in \c bounds that isn't covered by any rect in
\c regions -- e.g. the missing corner(s) of an L-shaped arrangement of
monitors, or the two gaps either side of the stem of a T-shaped one.  Each
gap must be a single clean rectangle touching exactly one wall, or exactly
two adjacent walls, of \c bounds.  A gap touching no wall, three or more
walls, or two opposite walls, or one that isn't a single clean rectangle, is
skipped -- that shape isn't supported, so edge-jumping simply won't trigger
there.
*/
std::vector<ShapeHole> findShapeHoles(const std::vector<Rect> &regions, const Rect &bounds);

} // namespace deskflow
