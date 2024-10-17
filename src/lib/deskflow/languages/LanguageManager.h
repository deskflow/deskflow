/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2021 Symless Ltd.
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

#include "base/String.h"
#include "deskflow/AppUtil.h"
#include <vector>

namespace deskflow {

namespace languages {

class LanguageManager
{
  std::vector<String> m_remoteLanguages;
  std::vector<String> m_localLanguages;

public:
  explicit LanguageManager(const std::vector<String> &localLanguages = AppUtil::instance().getKeyboardLayoutList());

  /**
   * @brief setRemoteLanguages sets remote languages
   * @param remoteLanguages is a string with sericalized languages
   */
  void setRemoteLanguages(const String &remoteLanguages);

  /**
   * @brief getRemoteLanguages getter for remote languages
   * @return vector of remote languages
   */
  const std::vector<String> &getRemoteLanguages() const;

  /**
   * @brief getLocalLanguages getter for local languages
   * @return vector of local languages
   */
  const std::vector<String> &getLocalLanguages() const;

  /**
   * @brief getMissedLanguages getter for missed languages on local machine
   * @return difference between remote and local languages as a coma separated
   * string
   */
  String getMissedLanguages() const;

  /**
   * @brief getSerializedLocalLanguages getter for local serialized languages
   * @return serialized local languages as a string
   */
  String getSerializedLocalLanguages() const;

  /**
   * @brief isLanguageInstalled checks if language is installed
   * @param language which should be checked
   * @return true if the specified language is installed
   */
  bool isLanguageInstalled(const String &language) const;
};

} // namespace languages

} // namespace deskflow
