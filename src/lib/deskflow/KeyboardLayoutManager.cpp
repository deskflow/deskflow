/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardLayoutManager.h"
#include "base/Log.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#endif

namespace {

bool looksLikeKlid(const std::string_view &layout)
{
  return layout.size() == 8 &&
         std::all_of(layout.begin(), layout.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
}

std::vector<std::string> splitLayouts(const std::string_view &layouts)
{
  std::vector<std::string> result;

  if (layouts.empty()) {
    return result;
  }

  if (layouts.find(',') != std::string_view::npos) {
    size_t start = 0;
    while (start < layouts.size()) {
      const auto end = layouts.find(',', start);
      const auto token = layouts.substr(
          start,
          end == std::string_view::npos ? layouts.size() - start : end - start
      );
      if (!token.empty()) {
        result.emplace_back(token);
      }
      if (end == std::string_view::npos) {
        break;
      }
      start = end + 1;
    }
    return result;
  }

  if (looksLikeKlid(layouts)) {
    result.emplace_back(layouts);
    return result;
  }

  if (layouts.size() % 8 == 0) {
    bool allKlids = true;
    for (size_t i = 0; i < layouts.size(); i += 8) {
      if (!looksLikeKlid(layouts.substr(i, 8))) {
        allKlids = false;
        break;
      }
    }

    if (allKlids) {
      for (size_t i = 0; i < layouts.size(); i += 8) {
        result.emplace_back(layouts.substr(i, 8));
      }
      return result;
    }
  }

  if (layouts.size() % 2 == 0) {
    for (size_t i = 0; i < layouts.size(); i += 2) {
      result.emplace_back(layouts.substr(i, 2));
    }
  }

  return result;
}

#if defined(_WIN32)
std::string wideCharToUtf8(const std::wstring &value)
{
  if (value.empty()) {
    return {};
  }

  const auto size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (size <= 1) {
    return {};
  }

  std::string result(static_cast<size_t>(size), '\0');
  if (WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr) == 0) {
    return {};
  }

  result.resize(static_cast<size_t>(size - 1));
  return result;
}

std::string formatWindowsLayoutForLog(const std::string_view &layout)
{
  if (!looksLikeKlid(layout)) {
    return std::string(layout);
  }

  const auto layoutString = std::string(layout);
  const auto layoutValue = static_cast<unsigned long>(std::strtoul(layoutString.c_str(), nullptr, 16));
  const auto localeId = MAKELCID(static_cast<WORD>(layoutValue & 0xffffu), SORT_DEFAULT);

  std::string result(layout);

  const auto localeSize = LCIDToLocaleName(localeId, nullptr, 0, 0);
  if (localeSize > 1) {
    std::wstring localeName(static_cast<size_t>(localeSize), L'\0');
    if (LCIDToLocaleName(localeId, localeName.data(), localeSize, 0) != 0) {
      localeName.resize(wcslen(localeName.c_str()));
      const auto localeTag = wideCharToUtf8(localeName);
      if (!localeTag.empty()) {
        result += " [" + localeTag + "]";
      }
    }
  }

  const auto keyPath = std::wstring(L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\") +
                       std::wstring(layout.begin(), layout.end());
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_READ, &key) == ERROR_SUCCESS) {
    DWORD type = 0;
    DWORD bytes = 0;
    if (RegQueryValueExW(key, L"Layout Text", nullptr, &type, nullptr, &bytes) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ) && bytes >= sizeof(wchar_t)) {
      std::wstring text(bytes / sizeof(wchar_t), L'\0');
      if (RegQueryValueExW(key, L"Layout Text", nullptr, &type, reinterpret_cast<LPBYTE>(text.data()), &bytes) ==
          ERROR_SUCCESS) {
        text.resize(wcslen(text.c_str()));
        const auto displayName = wideCharToUtf8(text);
        if (!displayName.empty()) {
          result += " ";
          result += displayName;
        }
      }
    }
    RegCloseKey(key);
  }

  return result;
}
#endif

std::string rawVectorToString(const std::vector<std::string> &vector, const std::string_view &delimiter = "")
{
  std::string string;
  for (size_t i = 0; i < vector.size(); ++i) {
    if (i != 0) {
      string += delimiter;
    }
    string += vector[i];
  }
  return string;
}

std::string logVectorToString(const std::vector<std::string> &vector, const std::string_view &delimiter = "")
{
  std::string string;
  for (size_t i = 0; i < vector.size(); ++i) {
    if (i != 0) {
      string += delimiter;
    }
#if defined(_WIN32)
    string += formatWindowsLayoutForLog(vector[i]);
#else
    string += vector[i];
#endif
  }
  return string;
}

} // anonymous namespace

namespace deskflow {

KeyboardLayoutManager::KeyboardLayoutManager(const std::vector<std::string> &localLayouts)
    : m_localLayouts(localLayouts)
{
  LOG_INFO("local layouts: %s", logVectorToString(m_localLayouts, ", ").c_str());
}

void KeyboardLayoutManager::setRemoteLayouts(const std::string_view &remoteLayouts)
{
  m_remoteLayouts = splitLayouts(remoteLayouts);
  LOG_INFO("remote layouts: %s", logVectorToString(m_remoteLayouts, ", ").c_str());
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
  return rawVectorToString(m_localLayouts, ",");
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
