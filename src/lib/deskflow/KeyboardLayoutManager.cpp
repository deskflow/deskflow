/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardLayoutManager.h"
#include "base/Log.h"

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

KeyboardLayoutManager::KeyboardLayoutManager(const std::vector<std::string> &localLayouts)
    : m_localLayouts(localLayouts)
{
  LOG_INFO("local layouts: %s", vectorToString(m_localLayouts, ", ").c_str());
}

void KeyboardLayoutManager::setRemoteLayouts(const std::string_view &remoteLayouts)
{
  m_remoteLayouts.clear();
  if (!remoteLayouts.empty()) {
    for (size_t i = 0; i <= remoteLayouts.size() - 2; i += 2) {
      auto rLangs = remoteLayouts.substr(i, 2);
      m_remoteLayouts.emplace_back(rLangs);
    }
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
