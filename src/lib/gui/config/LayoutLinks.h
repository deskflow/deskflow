/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QList>
#include <QPair>
#include <QRect>
#include <QString>

namespace deskflow::gui::layout {

// Declared in the order links are written for each computer.
enum class Side
{
  Right,
  Left,
  Up,
  Down
};

/**
 * @brief A link from one edge range of a computer to an edge range of another.
 * Ranges are percentages (0 to 100) of the respective computer's edge.
 */
struct Link
{
  QString from;
  Side side;
  double fromStart;
  double fromEnd;
  QString to;
  double toStart;
  double toEnd;
};

using Placed = QPair<QString, QRect>;

/**
 * @brief Computes links between computers whose rectangles share an edge.
 * Computers that only touch at a corner or are separated by a gap are not linked.
 */
QList<Link> computeLinks(const QList<Placed> &computers);

/**
 * @brief Formats a link for the server config links section, e.g. `right(0,50) = b(25,75)`.
 * Full edge ranges (0 to 100) are omitted.
 */
QString formatLink(const Link &link);

/**
 * @brief Moves @p moving flush against the nearest edge of @p others within @p threshold,
 * then aligns it with that neighbour's parallel edges if they are also within @p threshold.
 */
QRect snap(const QRect &moving, const QList<QRect> &others, int threshold);

bool overlapsAny(const QRect &rect, const QList<QRect> &others);

} // namespace deskflow::gui::layout
