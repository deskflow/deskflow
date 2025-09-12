/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

class XDGPowerManager
{
public:
  XDGPowerManager() = default;
  ~XDGPowerManager();

  /**
   * @brief Prevent the system from sleep
   */
  void disableSleep() const;

  /**
   * @brief Enables automatical sleep
   */
  void enableSleep() const;

  XDGPowerManager(const XDGPowerManager &) = delete;
  XDGPowerManager &operator=(const XDGPowerManager &) = delete;

  enum class InhibitScreenServices
  {
    kScreenSaver,
    kSessionManager
  };
  static bool inhibitScreenCall(InhibitScreenServices serviceID, bool state, std::string &error);
};
