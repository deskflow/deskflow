/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Computer.h"

class ComputerList : public QList<Computer>
{
  int m_width = 5;

public:
  explicit ComputerList(int width = 5);

  /**
   * @brief addComputerByPriority adds a new computer according to the following
   * priority: 1.left side of the server 2.right side of the server 3.top 4.down
   * 5.top left-hand diagonally
   * 6.top right-hand diagonally
   * 7.bottom right-hand diagonally
   * 8.bottom left-hand diagonally
   * 9.In case all places from the list have already booked, place in any spare
   * place
   * @param newComputer
   */
  void addComputerByPriority(const Computer &newComputer);

  /**
   * @brief addComputerToFirstEmpty adds computer into the first empty place
   * @param newComputer
   */
  void addComputerToFirstEmpty(const Computer &newComputer);

  /**
   * @brief Returns true if computers are equal
   * @param sc
   */
  bool operator==(const ComputerList &sc) const;
};
