/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Synergy App Ltd
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
  /**
   * @brief Converts a platform language tag to an ISO 639-1 code for layout matching.
   *
   * Extracts the primary language subtag before the first hyphen or underscore
   * and uses QLocale to convert it to the two-letter code required by the
   * layout-list protocol. This does not validate the complete language tag.
   *
   * @param languageTag Platform language tag or language code, such as zh-Hans,
   * en_US or eng.
   * @return Lowercase two-letter ISO 639-1 code, or an empty string if the
   * language is unknown or has no ISO 639-1 code.
   */
  static std::string languageForISO639_1(std::string_view languageTag);

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
