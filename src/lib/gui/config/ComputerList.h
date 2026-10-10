/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Computer.h"

#include <QRect>
#include <QSize>

class ComputerList : public QList<Computer>
{
public:
  inline static constexpr QSize kDefaultSize{1920, 1080};

  /**
   * @brief rects returns the geometry of every computer except @p except
   */
  QList<QRect> rects(const Computer *except = nullptr) const;

  /**
   * @brief freeSpotNextTo finds a free rect of @p size flush against @p anchor, trying
   * left, right, up then down; otherwise to the right of all computers
   */
  QRect freeSpotNextTo(const QRect &anchor, const QSize &size) const;

  /**
   * @brief addComputerByPriority adds a new computer flush against the server
   * @param newComputer
   */
  void addComputerByPriority(const Computer &newComputer);

  /**
   * @brief resizeComputer changes the size of a computer, keeping its top left corner;
   * it is moved right until it no longer overlaps another computer
   * @return true if the computer was found and its geometry changed
   */
  bool resizeComputer(const QString &name, const QSize &size);
};
