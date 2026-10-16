/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Computer.h"

#include <QMap>
#include <QRect>
#include <QSize>

class ComputerList : public QList<Computer>
{
public:
  inline static constexpr QSize kDefaultSize{1920, 1080};

  /**
   * @brief rects returns the geometry of every placed computer except @p except, keyed by name
   */
  QMap<QString, QRect> rects(const QString &except = {}) const;

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
};
