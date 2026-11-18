/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QRect>
#include <QString>

namespace deskflow::layout {
Q_NAMESPACE

// Declared in the order computeLinks returns the links of each computer.
enum class Side
{
  Right,
  Left,
  Up,
  Down
};
Q_ENUM_NS(Side)

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

  bool operator==(const Link &) const = default;
};

/**
 * @brief Links each edge of a computer to the nearest computers beyond it, skipping any gap between them.
 * Parts of an edge covered by a nearer computer are not linked to the ones behind it.
 * Computers that only touch at a corner are not linked.
 */
QList<Link> computeLinks(const QMap<QString, QRect> &computers);

/**
 * @brief Moves @p moving flush against the nearest edge of @p others within @p threshold,
 * then aligns it with that neighbour's parallel edges if they are also within @p threshold.
 */
QRect snap(const QRect &moving, const QList<QRect> &others, int threshold);

/**
 * @brief Moves the @p edges of @p resized onto the nearest parallel edge of @p others within @p threshold,
 * leaving its other edges in place.
 */
QRect snapResize(const QRect &resized, Qt::Edges edges, const QList<QRect> &others, int threshold);

bool overlapsAny(const QRect &rect, const QList<QRect> &others);

/**
 * @brief Returns @p rect if it overlaps none of @p others, otherwise the nearest spot of the same size that doesn't,
 * flush against one of them.
 */
QRect nearestFreeSpot(const QRect &rect, const QList<QRect> &others);

} // namespace deskflow::layout
