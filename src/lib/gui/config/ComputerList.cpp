/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerList.h"

#include "common/LayoutLinks.h"

#include <algorithm>

using deskflow::layout::overlapsAny;

QMap<QString, QRect> ComputerList::rects(const QString &except) const
{
  QMap<QString, QRect> result;
  for (const auto &computer : *this) {
    if (computer.name() != except && !computer.isNull() && computer.geometry().isValid())
      result.insert(computer.name(), computer.geometry());
  }
  return result;
}

QRect ComputerList::freeSpotNextTo(const QRect &anchor, const QSize &size) const
{
  const auto others = rects().values();
  const QList<QRect> candidates = {
      QRect(QPoint(anchor.x() - size.width(), anchor.y()), size),
      QRect(QPoint(anchor.x() + anchor.width(), anchor.y()), size),
      QRect(QPoint(anchor.x(), anchor.y() - size.height()), size),
      QRect(QPoint(anchor.x(), anchor.y() + anchor.height()), size),
  };
  for (const auto &candidate : candidates) {
    if (!overlapsAny(candidate, others))
      return candidate;
  }

  QRect bounds;
  for (const auto &rect : others)
    bounds |= rect;
  return QRect(QPoint(bounds.x() + bounds.width(), bounds.y()), size);
}

void ComputerList::addComputerByPriority(const Computer &newComputer)
{
  Computer computer = newComputer;
  const auto server = std::ranges::find_if(*this, [](const Computer &c) { return c.isServer(); });
  const bool serverPlaced = server != end() && server->geometry().isValid();
  // As big as the server so it isn't tiny next to computers that were resized.
  const auto size = computer.geometry().isValid() ? computer.geometry().size()
                                                  : (serverPlaced ? server->geometry().size() : kDefaultSize);
  const auto anchor = serverPlaced ? server->geometry() : QRect(QPoint(), size);
  computer.setGeometry(freeSpotNextTo(anchor, size));
  append(computer);
}
