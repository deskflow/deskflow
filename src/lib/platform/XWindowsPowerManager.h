/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

class XWindowsPowerManager
{
public:
  XWindowsPowerManager() = default;
  ~XWindowsPowerManager();

  /**
   * @brief Prevent the system from sleep
   */
  void disableSleep() const;

  /**
   * @brief Enables automatical sleep
   */
  void enableSleep() const;

  XWindowsPowerManager(const XWindowsPowerManager &) = delete;
  XWindowsPowerManager &operator=(const XWindowsPowerManager &) = delete;
};
