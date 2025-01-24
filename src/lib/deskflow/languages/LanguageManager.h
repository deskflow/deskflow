/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"
#include <vector>

namespace deskflow {

namespace languages {

class LanguageManager
{
  std::vector<std::string> m_remoteLanguages;
  std::vector<std::string> m_localLanguages;

public:
  explicit LanguageManager(
      const std::vector<std::string> &localLanguages = AppUtil::instance().getKeyboardLayoutList()
  );

  /**
   * @brief setRemoteLanguages sets remote languages
   * @param remoteLanguages is a string with sericalized languages
   */
  void setRemoteLanguages(const std::string &remoteLanguages);

  /**
   * @brief getRemoteLanguages getter for remote languages
   * @return vector of remote languages
   */
  const std::vector<std::string> &getRemoteLanguages() const;

  /**
   * @brief getLocalLanguages getter for local languages
   * @return vector of local languages
   */
  const std::vector<std::string> &getLocalLanguages() const;

  /**
   * @brief getMissedLanguages getter for missed languages on local machine
   * @return difference between remote and local languages as a coma separated
   * string
   */
  std::string getMissedLanguages() const;

  /**
   * @brief getSerializedLocalLanguages getter for local serialized languages
   * @return serialized local languages as a string
   */
  std::string getSerializedLocalLanguages() const;

  /**
   * @brief isLanguageInstalled checks if language is installed
   * @param language which should be checked
   * @return true if the specified language is installed
   */
  bool isLanguageInstalled(const std::string &language) const;
};

} // namespace languages

} // namespace deskflow
