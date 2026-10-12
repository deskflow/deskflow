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
A hole touching one wall (e.g. "bottom") sits in the middle of that edge,
with real screen on both of its other sides, e.g. the two gaps either side
of the stem of a T-shaped arrangement of monitors.  A hole touching two
adjacent walls (e.g. "bottom-right") is a missing corner, e.g. the fourth
quadrant of an L-shaped arrangement.

This is the exact string a user writes in the server config file (e.g.
"bottom-right = someComputer") and the exact string logged for it -- there
is no separate enum-to-string conversion, this \e is the canonical form.
Only the 8 values parseHoleLocation() accepts are ever valid; everything
that produces one (findShapeHoles(), parseHoleLocation()) is responsible
for only ever producing one of those 8 strings.
*/
using HoleLocation = std::string;

//! Parse a hole location from its name, e.g. "bottom-right"
/*!
Validates \c name against the fixed set of 8 known location names (case
sensitive) and returns it unchanged if it matches, or std::nullopt if it
doesn't match any of them.
*/
std::optional<HoleLocation> parseHoleLocation(const std::string &name);

//! The directions from which a hole at `location` can be entered or exited
/*!
A hole touching one outer wall has real screen on its other three sides, so
it can be approached from three directions; a hole touching two adjacent
walls (a corner) can be approached from two.  Returns an empty list for a
\c location that isn't one of the 8 known values.
*/
std::vector<Direction> activeDirections(const HoleLocation &location);

//! A single rectangular gap in an otherwise-rectangular arrangement of screens
struct ShapeHole
{
  Rect rect;
  HoleLocation location = "top";

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
