/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Con Hennessy <con.hennessy@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ShapeHoleTests.h"

#include "deskflow/ShapeHole.h"

#include <algorithm>

using deskflow::findShapeHoles;
using deskflow::HoleLocation;
using deskflow::Rect;
using deskflow::ShapeHole;

namespace {
const ShapeHole *findByLocation(const std::vector<ShapeHole> &holes, HoleLocation location)
{
  const auto it =
      std::ranges::find_if(holes, [location](const ShapeHole &hole) { return hole.location == location; });
  return it == holes.end() ? nullptr : &(*it);
}
} // namespace

void ShapeHoleTests::findShapeHoles_singleRegionFillingBounds_noHoles()
{
  const Rect bounds{0, 0, 1920, 1080};
  const std::vector<Rect> regions{{0, 0, 1920, 1080}};
  QVERIFY(findShapeHoles(regions, bounds).empty());
}

void ShapeHoleTests::findShapeHoles_bottomRightCornerMissing_oneHoleFound()
{
  // two monitors on top, one on the bottom-left: an L with the bottom-right missing
  const Rect bounds{0, 0, 3840, 2160};
  const std::vector<Rect> regions{
      {0, 0, 1920, 1080},    // top-left
      {1920, 0, 1920, 1080}, // top-right
      {0, 1080, 1920, 1080}, // bottom-left
  };
  const auto holes = findShapeHoles(regions, bounds);
  QCOMPARE(holes.size(), 1);
  QCOMPARE(holes[0].location, "bottom-right");
  QCOMPARE(holes[0].rect.x, 1920);
  QCOMPARE(holes[0].rect.y, 1080);
  QCOMPARE(holes[0].rect.w, 1920);
  QCOMPARE(holes[0].rect.h, 1080);
}

void ShapeHoleTests::findShapeHoles_topLeftCornerMissing_oneHoleFound()
{
  const Rect bounds{0, 0, 3840, 2160};
  const std::vector<Rect> regions{
      {1920, 0, 1920, 1080},    // top-right
      {0, 1080, 1920, 1080},    // bottom-left
      {1920, 1080, 1920, 1080}, // bottom-right
  };
  const auto holes = findShapeHoles(regions, bounds);
  QCOMPARE(holes.size(), 1);
  QCOMPARE(holes[0].location, "top-left");
  QCOMPARE(holes[0].rect.x, 0);
  QCOMPARE(holes[0].rect.y, 0);
  QCOMPARE(holes[0].rect.w, 1920);
  QCOMPARE(holes[0].rect.h, 1080);
}

void ShapeHoleTests::findShapeHoles_bottomLeftCornerMissing_oneHoleFound()
{
  const Rect bounds{0, 0, 3840, 2160};
  const std::vector<Rect> regions{
      {0, 0, 1920, 1080},       // top-left
      {1920, 0, 1920, 1080},    // top-right
      {1920, 1080, 1920, 1080}, // bottom-right
  };
  const auto holes = findShapeHoles(regions, bounds);
  QCOMPARE(holes.size(), 1);
  QCOMPARE(holes[0].location, "bottom-left");
  QCOMPARE(holes[0].rect.x, 0);
  QCOMPARE(holes[0].rect.y, 1080);
  QCOMPARE(holes[0].rect.w, 1920);
  QCOMPARE(holes[0].rect.h, 1080);
}

void ShapeHoleTests::findShapeHoles_topRightCornerMissing_oneHoleFound()
{
  const Rect bounds{0, 0, 3840, 2160};
  const std::vector<Rect> regions{
      {0, 0, 1920, 1080},       // top-left
      {0, 1080, 1920, 1080},    // bottom-left
      {1920, 1080, 1920, 1080}, // bottom-right
  };
  const auto holes = findShapeHoles(regions, bounds);
  QCOMPARE(holes.size(), 1);
  QCOMPARE(holes[0].location, "top-right");
  QCOMPARE(holes[0].rect.x, 1920);
  QCOMPARE(holes[0].rect.y, 0);
  QCOMPARE(holes[0].rect.w, 1920);
  QCOMPARE(holes[0].rect.h, 1080);
}

void ShapeHoleTests::findShapeHoles_unequalMonitorSizes_holeFoundAtCorrectRect()
{
  // an L made of three differently-sized monitors: nothing requires a
  // uniform grid, just that the uncovered area is a single clean rectangle
  const Rect bounds{0, 0, 3840, 2340};
  const std::vector<Rect> regions{
      {0, 0, 2560, 1440},    // top-left, 2560x1440
      {2560, 0, 1280, 1440}, // top-right, 1280x1440 (narrower)
      {0, 1440, 2560, 900},  // bottom-left, 2560x900 (shorter)
  };
  const auto holes = findShapeHoles(regions, bounds);
  QCOMPARE(holes.size(), 1);
  QCOMPARE(holes[0].location, "bottom-right");
  QCOMPARE(holes[0].rect.x, 2560);
  QCOMPARE(holes[0].rect.y, 1440);
  QCOMPARE(holes[0].rect.w, 1280);
  QCOMPARE(holes[0].rect.h, 900);
}

void ShapeHoleTests::findShapeHoles_tShape_twoHolesFoundBesideStem()
{
  // three monitors across the top, one hanging below the middle one only:
  // a T-shape, with a gap on either side of the "stem"
  const Rect bounds{0, 0, 5760, 2160};
  const std::vector<Rect> regions{
      {0, 0, 1920, 1080},       // top-left
      {1920, 0, 1920, 1080},    // top-middle
      {3840, 0, 1920, 1080},    // top-right
      {1920, 1080, 1920, 1080}, // the stem, below top-middle only
  };
  const auto holes = findShapeHoles(regions, bounds);
  QCOMPARE(holes.size(), 2);

  const auto *bottomLeft = findByLocation(holes, "bottom-left");
  QVERIFY(bottomLeft != nullptr);
  QCOMPARE(bottomLeft->rect.x, 0);
  QCOMPARE(bottomLeft->rect.y, 1080);
  QCOMPARE(bottomLeft->rect.w, 1920);
  QCOMPARE(bottomLeft->rect.h, 1080);

  const auto *bottomRight = findByLocation(holes, "bottom-right");
  QVERIFY(bottomRight != nullptr);
  QCOMPARE(bottomRight->rect.x, 3840);
  QCOMPARE(bottomRight->rect.y, 1080);
  QCOMPARE(bottomRight->rect.w, 1920);
  QCOMPARE(bottomRight->rect.h, 1080);
}

void ShapeHoleTests::findShapeHoles_notchInMiddleOfEdge_oneHoleFoundNotTouchingCorner()
{
  // a gap centered on the top edge, reaching neither the left nor the right
  // wall: not a corner, but still a single clean rectangle on one wall
  const Rect bounds{0, 0, 3000, 2000};
  const std::vector<Rect> regions{
      {0, 0, 1000, 2000},       // left column, full height
      {2000, 0, 1000, 2000},    // right column, full height
      {1000, 1000, 1000, 1000}, // bottom half of the middle column only
  };
  const auto holes = findShapeHoles(regions, bounds);
  QCOMPARE(holes.size(), 1);
  QCOMPARE(holes[0].location, "top");
  QCOMPARE(holes[0].rect.x, 1000);
  QCOMPARE(holes[0].rect.y, 0);
  QCOMPARE(holes[0].rect.w, 1000);
  QCOMPARE(holes[0].rect.h, 1000);
}

void ShapeHoleTests::findShapeHoles_interiorGap_noHoles()
{
  // a gap in the middle of the bounding box, touching no outer wall at all
  const Rect bounds{0, 0, 3000, 3000};
  const std::vector<Rect> regions{
      {0, 0, 3000, 1000},       // top strip
      {0, 2000, 3000, 1000},    // bottom strip
      {0, 1000, 1000, 1000},    // middle-left
      {2000, 1000, 1000, 1000}, // middle-right
  };
  QVERIFY(findShapeHoles(regions, bounds).empty());
}

void ShapeHoleTests::findShapeHoles_nonRectangularConnectedGap_noHoles()
{
  // only the bottom-right cell of a 2x2 grid is covered: the uncovered area
  // is a single connected (L-shaped) region but isn't itself a rectangle
  const Rect bounds{0, 0, 2000, 2000};
  const std::vector<Rect> regions{{1000, 1000, 1000, 1000}};
  QVERIFY(findShapeHoles(regions, bounds).empty());
}

void ShapeHoleTests::activeDirections_matchesEachLocation()
{
  using enum Direction;

  auto hasAll = [](const std::vector<Direction> &dirs, std::initializer_list<Direction> expected) {
    return dirs.size() == expected.size() &&
           std::ranges::all_of(expected, [&dirs](Direction d) { return std::ranges::find(dirs, d) != dirs.end(); });
  };

  QVERIFY(hasAll(deskflow::activeDirections("top-left"), {Left, Top}));
  QVERIFY(hasAll(deskflow::activeDirections("top-right"), {Right, Top}));
  QVERIFY(hasAll(deskflow::activeDirections("bottom-left"), {Left, Bottom}));
  QVERIFY(hasAll(deskflow::activeDirections("bottom-right"), {Right, Bottom}));
  QVERIFY(hasAll(deskflow::activeDirections("top"), {Left, Right, Top}));
  QVERIFY(hasAll(deskflow::activeDirections("bottom"), {Left, Right, Bottom}));
  QVERIFY(hasAll(deskflow::activeDirections("left"), {Top, Bottom, Left}));
  QVERIFY(hasAll(deskflow::activeDirections("right"), {Top, Bottom, Right}));
}

void ShapeHoleTests::parseHoleLocation_roundTrips()
{
  for (const auto location :
       {"top", "bottom", "left", "right", "top-left",
        "top-right", "bottom-left", "bottom-right"}) {
    const auto parsed = deskflow::parseHoleLocation(location);
    QVERIFY(parsed.has_value());
    QCOMPARE(*parsed, location);
  }
}

void ShapeHoleTests::parseHoleLocation_unknownName_returnsNullopt()
{
  QVERIFY(!deskflow::parseHoleLocation("diagonal").has_value());
  QVERIFY(!deskflow::parseHoleLocation("").has_value());
  QVERIFY(!deskflow::parseHoleLocation("Bottom-Left").has_value()); // case-sensitive
}

QTEST_MAIN(ShapeHoleTests)
