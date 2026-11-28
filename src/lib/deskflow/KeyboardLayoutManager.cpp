/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardLayoutManager.h"
#include "base/Log.h"

#include <QLocale>

#include <algorithm>

namespace {

std::string vectorToString(const std::vector<std::string> &vector, const std::string_view &delimiter = "")
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

std::string KeyboardLayoutManager::languageForISO639_1(std::string_view languageTag)
{
  // Platform tags may use BCP 47 hyphens or locale-style underscores. Script,
  // region and variant subtags do not affect language-level layout matching.
  const auto primary = languageTag.substr(0, languageTag.find_first_of("-_"));
  const auto code = QString::fromUtf8(primary.data(), static_cast<qsizetype>(primary.size())).toLower();
  const auto language = QLocale::codeToLanguage(code);
  if (language == QLocale::C) {
    LOG_DEBUG("Unrecognized keyboard language tag: \"%s\"", std::string(languageTag).c_str());
    return {};
  }
  std::string result = QLocale::languageToCode(language, QLocale::ISO639Part1).toStdString();
  if (result.empty()) {
    LOG_DEBUG("Keyboard language tag has no ISO 639-1 code: \"%s\"", std::string(languageTag).c_str());
  }
  return result;
}

KeyboardLayoutManager::KeyboardLayoutManager(const std::vector<std::string> &localLayouts)
    : m_localLayouts(localLayouts)
{
  LOG_INFO("local layouts: %s", vectorToString(m_localLayouts, ", ").c_str());
}

void KeyboardLayoutManager::setRemoteLayouts(const std::string_view &remoteLayouts)
{
  if (!remoteLayouts.empty() && remoteLayouts.size() % 2 != 0) {
    LOG_ERR("remote layouts are the incorrect size, can not process them");
    return;
  }

  m_remoteLayouts.clear();
  for (size_t i = 0; i + 2 <= remoteLayouts.size(); i += 2) {
    auto rLangs = remoteLayouts.substr(i, 2);
    m_remoteLayouts.emplace_back(rLangs);
  }
  LOG_INFO("remote layouts: %s", vectorToString(m_remoteLayouts, ", ").c_str());
}

const std::vector<std::string> &KeyboardLayoutManager::getRemoteLayouts() const
{
  return m_remoteLayouts;
}

const std::vector<std::string> &KeyboardLayoutManager::getLocalLayouts() const
{
  return m_localLayouts;
}

std::string KeyboardLayoutManager::getMissedLayouts() const
{
  std::string missedLayouts;

  for (const auto &layout : m_remoteLayouts) {
    if (!isLayoutInstalled(layout)) {
      if (!missedLayouts.empty()) {
        missedLayouts += ", ";
      }
      missedLayouts += layout;
    }
  }

  return missedLayouts;
}

std::string KeyboardLayoutManager::getSerializedLocalLayouts() const
{
  return vectorToString(m_localLayouts);
}

bool KeyboardLayoutManager::isLayoutInstalled(const std::string &layout) const
{
  bool isInstalled = true;

  if (!m_localLayouts.empty()) {
    isInstalled = (std::find(m_localLayouts.begin(), m_localLayouts.end(), layout) != m_localLayouts.end());
  }

  return isInstalled;
}

} // namespace deskflow
