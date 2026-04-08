/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"
#include <vector>

namespace deskflow {

class KeyboardLayoutManager
{
  std::vector<std::string> m_remoteLayouts;
  std::vector<std::string> m_localLayouts;

public:
  explicit KeyboardLayoutManager(
      const std::vector<std::string> &localLayouts = AppUtil::instance().getKeyboardLayoutList()
  );

  /**
   * @brief setRemoteLayouts sets remote layouts
   * @param remoteLayouts is a string with sericalized layouts
   */
  void setRemoteLayouts(const std::string_view &remoteLayouts);

  /**
   * @brief getRemoteLayouts getter for remote layouts
   * @return vector of remote layouts
   */
  const std::vector<std::string> &getRemoteLayouts() const;

  /**
   * @brief getLocalLayouts getter for local layouts
   * @return vector of local layouts
   */
  const std::vector<std::string> &getLocalLayouts() const;

  /**
   * @brief getMissedLayouts getter for missed layouts on local machine
   * @return difference between remote and local layouts as a coma separated
   * string
   */
  std::string getMissedLayouts() const;

  /**
   * @brief getSerializedLocalLayouts getter for local serialized layouts
   * @return serialized local layouts as a string
   */
  std::string getSerializedLocalLayouts() const;

  /**
   * @brief isLayoutInstalled checks if layout is installed
   * @param layout which should be checked
   * @return true if the specified layout is installed
   */
  bool isLayoutInstalled(const std::string &layout) const;
};

} // namespace deskflow
