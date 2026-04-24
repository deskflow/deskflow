/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"

#include <string>
#include <string_view>
#include <vector>

namespace deskflow {

class KeyboardLayoutManager
{
  std::vector<std::string> m_remoteLayouts;
  std::vector<std::string> m_localLayouts;

public:
  /**
   * @brief Creates a keyboard layout manager.
   * @param localLayouts The local keyboard layout identifiers to use.
   */
  explicit KeyboardLayoutManager(
      const std::vector<std::string> &localLayouts = AppUtil::instance().getKeyboardLayoutList()
  );

  /**
   * @brief Sets the remote keyboard layouts.
   * @param remoteLayouts A serialized string containing the remote keyboard layout identifiers.
   */
  void setRemoteLayouts(const std::string_view &remoteLayouts);

  /**
   * @brief Gets the remote keyboard layouts.
   * @return A vector containing the remote keyboard layout identifiers.
   */
  const std::vector<std::string> &getRemoteLayouts() const;

  /**
   * @brief Gets the local keyboard layouts.
   * @return A vector containing the local keyboard layout identifiers.
   */
  const std::vector<std::string> &getLocalLayouts() const;

  /**
   * @brief Gets the layouts that are present remotely but missing locally.
   * @return The missing local layouts as a comma-separated string.
   */
  std::string getMissedLayouts() const;

  /**
   * @brief Gets the serialized local keyboard layouts.
   * @return The local keyboard layout identifiers as a serialized string.
   */
  std::string getSerializedLocalLayouts() const;

  /**
   * @brief Checks whether a keyboard layout is installed locally.
   * @param layout The keyboard layout identifier to check.
   * @return true if the specified layout is installed locally, otherwise false.
   */
  bool isLayoutInstalled(const std::string &layout) const;
};

} // namespace deskflow
