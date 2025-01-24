/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
