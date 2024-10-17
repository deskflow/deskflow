/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2021 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <IOKit/pwr_mgt/IOPMLib.h>

class OSXPowerManager
{
public:
  OSXPowerManager();
  ~OSXPowerManager();

  /**
   * @brief Prevents the system from sleep automatically
   */
  void disableSleep();

  /**
   * @brief Enable automatically sleeping
   */
  void enableSleep();

  OSXPowerManager(const OSXPowerManager &) = delete;
  OSXPowerManager &operator=(const OSXPowerManager &) = delete;

private:
  // handler for assertion preventing the system from going to sleep
  IOPMAssertionID m_sleepPreventionAssertionID = 0;
};
