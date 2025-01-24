/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LanguageManager.h"
#include "base/Log.h"

#include <algorithm>

namespace {

std::string vectorToString(const std::vector<std::string> &vector, const std::string &delimiter = "")
{
  std::string string;
  for (const auto &item : vector) {
    if (&item != &vector[0]) {
      string += delimiter;
    }
    string += item;
  }
  return string;
}

} // anonymous namespace

namespace deskflow {

namespace languages {

LanguageManager::LanguageManager(const std::vector<std::string> &localLanguages) : m_localLanguages(localLanguages)
{
  LOG((CLOG_INFO "local languages: %s", vectorToString(m_localLanguages, ", ").c_str()));
}

void LanguageManager::setRemoteLanguages(const std::string &remoteLanguages)
{
  m_remoteLanguages.clear();
  if (!remoteLanguages.empty()) {
    for (size_t i = 0; i <= remoteLanguages.size() - 2; i += 2) {
      m_remoteLanguages.push_back(remoteLanguages.substr(i, 2));
    }
  }
  LOG((CLOG_INFO "remote languages: %s", vectorToString(m_remoteLanguages, ", ").c_str()));
}

const std::vector<std::string> &LanguageManager::getRemoteLanguages() const
{
  return m_remoteLanguages;
}

const std::vector<std::string> &LanguageManager::getLocalLanguages() const
{
  return m_localLanguages;
}

std::string LanguageManager::getMissedLanguages() const
{
  std::string missedLanguages;

  for (const auto &language : m_remoteLanguages) {
    if (!isLanguageInstalled(language)) {
      if (!missedLanguages.empty()) {
        missedLanguages += ", ";
      }
      missedLanguages += language;
    }
  }

  return missedLanguages;
}

std::string LanguageManager::getSerializedLocalLanguages() const
{
  return vectorToString(m_localLanguages);
}

bool LanguageManager::isLanguageInstalled(const std::string &language) const
{
  bool isInstalled = true;

  if (!m_localLanguages.empty()) {
    isInstalled = (std::find(m_localLanguages.begin(), m_localLanguages.end(), language) != m_localLanguages.end());
  }

  return isInstalled;
}

} // namespace languages

} // namespace deskflow
