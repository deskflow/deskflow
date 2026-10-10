/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerList.h"

#include "LayoutLinks.h"

#include <algorithm>

using deskflow::gui::layout::overlapsAny;

QList<QRect> ComputerList::rects(const Computer *except) const
{
  QList<QRect> result;
  for (const auto &computer : *this) {
    if (&computer != except && !computer.isNull() && computer.geometry().isValid())
      result.append(computer.geometry());
  }
  return result;
}

QRect ComputerList::freeSpotNextTo(const QRect &anchor, const QSize &size) const
{
  const auto others = rects();
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
  const auto size = computer.geometry().isValid() ? computer.geometry().size() : kDefaultSize;
  const auto anchor = server != end() && server->geometry().isValid() ? server->geometry() : QRect(QPoint(), size);
  computer.setGeometry(freeSpotNextTo(anchor, size));
  append(computer);
}

bool ComputerList::resizeComputer(const QString &name, const QSize &size)
{
  const auto it = std::ranges::find_if(*this, [&name](const Computer &c) { return c.name() == name; });
  if (it == end() || size.isEmpty() || it->geometry().size() == size)
    return false;

  const auto others = rects(&*it);
  QRect rect(it->geometry().topLeft(), size);
  for (bool moved = true; moved;) {
    moved = false;
    for (const auto &other : others) {
      if (rect.intersects(other)) {
        rect.moveLeft(other.x() + other.width());
        moved = true;
      }
    }
  }
  it->setGeometry(rect);
  return true;
}
