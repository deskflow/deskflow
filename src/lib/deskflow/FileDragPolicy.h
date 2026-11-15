/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/DirectionTypes.h"
#include "deskflow/MouseTypes.h"

#include <cstdint>
#include <optional>

/**
 * @file FileDragPolicy.h
 * @brief The decisions of a file drag that must stay the same everywhere, kept free of any
 * window system so they can be tested on their own: where a computer watches for a drag
 * of files, and when a held mouse button may take the cursor to another computer.
 *
 * Shared by the server, the client, the Windows platform layer and the deskflow-filedrag
 * helper, so the core and the helper can never disagree about which side of the screen is
 * meant.
 *
 * Two directions: from the server to a client (the server watches its own edges) and from
 * a client back to the server (the server, which owns the switching and knows where the
 * cursor is on the client, asks the client to watch the edges that lead to the server).
 */

namespace deskflow::filedrag {

//! Where a file drag stands on the server
enum class DragPhase : uint8_t
{
  Idle,     ///< no file drag
  Offered,  ///< an OLE drag of files is over the edge window, the button is still held there
  Remote,   ///< the cursor took the drag to the destination, the button is still held
  Released, ///< let go on the destination, waiting for it to say how the drop went
  Dropped,  ///< dropped on the destination, its files are still copying
};

constexpr const char *dragPhaseName(DragPhase phase)
{
  switch (phase) {
  case DragPhase::Idle:
    return "none";
  case DragPhase::Offered:
    return "files at the edge";
  case DragPhase::Remote:
    return "on the other computer";
  case DragPhase::Released:
    return "released";
  case DragPhase::Dropped:
    return "dropped";
  }
  return "?";
}

//! What the server knows when the cursor reaches a switch edge
struct SwitchCheck
{
  bool lockedByUser = false; ///< locked with scroll lock or the lock hotkey: never crosses
  uint32_t buttonsDown = 0;  ///< mouse buttons held on the primary, bit per ButtonID
  DragPhase phase = DragPhase::Idle;
  bool fromPrimary = false;      ///< the cursor is on the primary now
  bool fromDragTarget = false;   ///< the cursor is on the computer the file drag went to
  bool toPrimary = false;        ///< the destination is the primary
  bool toTakesFileDrags = false; ///< the destination is a secondary that can receive file drags

  // client to server drags (the files come from a secondary)
  bool fromSecondary = false;         ///< the drag's files come from a secondary, not the primary
  bool fromDragSource = false;        ///< the cursor is on the secondary the files come from
  bool toDragSource = false;          ///< the destination is the secondary the files come from
  bool primaryTakesFileDrags = false; ///< the primary can show a file drag and receive files
};

//! True when only the left mouse button is held
constexpr bool onlyLeftButtonDown(uint32_t buttonsDown)
{
  return buttonsDown == (1u << kButtonLeft);
}

/**
 * A held mouse button keeps the cursor on its computer: that is what lets a window be
 * dragged to the screen edge to snap it, and keeps text selections, scroll bar drags and
 * drags of anything other than files on the computer they started on.
 *
 * The one exception is a drag of files or folders. It counts only once the platform has
 * seen an OLE drag carrying files arrive on its edge window (DragPhase::Offered), so a
 * window drag, Aero Snap or a text drag never gets past this. Then the left button alone
 * may take the cursor to a secondary that can receive the files, and back to the primary
 * before the drop (which cancels it there).
 *
 * The same holds the other way round, on the client: only a drag of files that the
 * client's edge window saw (and reported with kMsgDFileDragOffer) may take the held left
 * button from that client to the primary, and only back to that client before the drop.
 *
 * Only decides the button lock; the switch delay, double tap and locked corners still apply.
 */
constexpr bool fileDragAllowsSwitch(const SwitchCheck &c)
{
  if (c.lockedByUser || !onlyLeftButtonDown(c.buttonsDown)) {
    return false;
  }
  if (c.fromSecondary) {
    if (c.phase == DragPhase::Offered) {
      return c.fromDragSource && c.toPrimary && c.primaryTakesFileDrags;
    }
    if (c.phase == DragPhase::Remote) {
      return c.fromPrimary && c.toDragSource;
    }
    return false;
  }
  if (c.phase == DragPhase::Offered) {
    return c.fromPrimary && !c.toPrimary && c.toTakesFileDrags;
  }
  if (c.phase == DragPhase::Remote) {
    return c.fromDragTarget && c.toPrimary;
  }
  return false;
}

//! Whether the platform's "files are at the edge" report starts a file drag
constexpr bool fileDragAcceptsOffer(DragPhase phase, bool onPrimary, uint32_t buttonsDown)
{
  return phase == DragPhase::Idle && onPrimary && onlyLeftButtonDown(buttonsDown);
}

/**
 * Whether a client's "files are at the edge" report (kMsgDFileDragOffer) starts a client to
 * server drag: the cursor must be on that client, the server must have asked it to watch
 * (the sides it sent, not 0), and the left button alone must be held.
 */
constexpr bool
fileDragAcceptsRemoteOffer(DragPhase phase, bool onThatClient, uint32_t watchedSides, uint32_t buttonsDown)
{
  return phase == DragPhase::Idle && onThatClient && watchedSides != 0 && onlyLeftButtonDown(buttonsDown);
}

//! An area of the screen, in the primary's screen coordinates
struct EdgeRect
{
  int32_t x = 0;
  int32_t y = 0;
  int32_t w = 0;
  int32_t h = 0;

  bool operator==(const EdgeRect &) const = default;
};

//! How thick the edge windows are; the cursor has to be on one for OLE to report the drag
constexpr int32_t kEdgeStripThickness = 4;

//! How close to an edge the cursor must be (pixels) before that edge is watched
constexpr int32_t kWatchReach = 48;

/**
 * The sides of a screen (a DirectionMask) to watch while a held left button is at x, y on
 * it: those within reach of the cursor for which leadsThere says the neighbour on that side
 * should get the files. leadsThere(Direction side, int32_t nx, int32_t ny) gets the side of
 * THIS screen and a point just outside it, as the server's mapToNeighbor() expects.
 *
 * The server uses it for its own screen (sides leading to clients that take file drags)
 * and for a client's screen (sides leading back to the server).
 */
template <typename LeadsThere>
constexpr uint32_t
sidesToWatch(const EdgeRect &screen, int32_t x, int32_t y, LeadsThere leadsThere, int32_t reach = kWatchReach)
{
  if (screen.w <= 0 || screen.h <= 0) {
    return 0;
  }
  uint32_t sides = 0;
  auto check = [&sides, &leadsThere](Direction side, int32_t nx, int32_t ny) {
    if (leadsThere(side, nx, ny)) {
      sides |= static_cast<uint32_t>(1u << static_cast<int>(side));
    }
  };
  if (x < screen.x + reach) {
    check(Direction::Left, screen.x - 1, y);
  }
  if (x >= screen.x + screen.w - reach) {
    check(Direction::Right, screen.x + screen.w, y);
  }
  if (y < screen.y + reach) {
    check(Direction::Top, x, screen.y - 1);
  }
  if (y >= screen.y + screen.h - reach) {
    check(Direction::Bottom, x, screen.y + screen.h);
  }
  return sides;
}

//! A one pixel step (dx, dy)
struct Step
{
  int32_t dx = 0;
  int32_t dy = 0;
  bool operator==(const Step &) const = default;
};

/**
 * When a client reports files at its edge the cursor is usually already pressed against
 * that edge (held there by the button lock) and may not move again. The server then tries
 * the switch once more with this step, as if the cursor had pushed one pixel further; 0, 0
 * when the cursor is not on a watched side.
 */
constexpr Step stepAcross(const EdgeRect &screen, int32_t x, int32_t y, uint32_t sides)
{
  auto has = [sides](Direction side) { return (sides & (1u << static_cast<int>(side))) != 0; };
  if (has(Direction::Left) && x <= screen.x) {
    return {-1, 0};
  }
  if (has(Direction::Right) && x >= screen.x + screen.w - 1) {
    return {1, 0};
  }
  if (has(Direction::Top) && y <= screen.y) {
    return {0, -1};
  }
  if (has(Direction::Bottom) && y >= screen.y + screen.h - 1) {
    return {0, 1};
  }
  return {};
}

/**
 * The thin window the primary shows along \p side of \p screen while a held left button
 * nears that side, if \p sides (a DirectionMask, as the server sends it) includes it.
 */
constexpr std::optional<EdgeRect>
edgeStrip(Direction side, uint32_t sides, const EdgeRect &screen, int32_t thickness = kEdgeStripThickness)
{
  const auto bit = static_cast<uint32_t>(1u << static_cast<int>(side));
  if (side == Direction::NoDirection || (sides & bit) == 0 || screen.w <= 0 || screen.h <= 0) {
    return std::nullopt;
  }
  switch (side) {
  case Direction::Left:
    return EdgeRect{screen.x, screen.y, thickness, screen.h};
  case Direction::Right:
    return EdgeRect{screen.x + screen.w - thickness, screen.y, thickness, screen.h};
  case Direction::Top:
    return EdgeRect{screen.x, screen.y, screen.w, thickness};
  case Direction::Bottom:
    return EdgeRect{screen.x, screen.y + screen.h - thickness, screen.w, thickness};
  default:
    return std::nullopt;
  }
}

} // namespace deskflow::filedrag
