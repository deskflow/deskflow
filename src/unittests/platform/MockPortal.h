/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/Wayland.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace deskflow::test {

// Lightweight stub implementation (libportal no longer required)
class MockPortal
{
public:
  using ClipboardChangeCallback = std::function<void(const std::vector<std::string> &)>;

  MockPortal() = default;
  ~MockPortal() = default;

  bool initialize()
  {
    return true;
  }
  void cleanup()
  {
    m_clipboardData.clear();
  }
  bool isAvailable() const
  {
    return true;
  }

  // Minimal API used by tests
  void setClipboardData(const std::string &mimeType, const std::string &data)
  {
    m_clipboardData[mimeType] = data;
  }
  std::string getClipboardData(const std::string &mimeType) const
  {
    auto it = m_clipboardData.find(mimeType);
    return it != m_clipboardData.end() ? it->second : std::string();
  }
  std::vector<std::string> getAvailableMimeTypes() const
  {
    std::vector<std::string> types;
    for (const auto &p : m_clipboardData)
      types.push_back(p.first);
    return types;
  }
  void clearClipboard()
  {
    m_clipboardData.clear();
  }

  void setClipboardChangeCallback(ClipboardChangeCallback cb)
  {
    m_changeCallback = std::move(cb);
  }

  void simulateClipboardChange(const std::map<std::string, std::string> &data)
  {
    m_clipboardData = data;
    if (m_changeCallback) {
      std::vector<std::string> types;
      for (const auto &p : data)
        types.push_back(p.first);
      m_changeCallback(types);
    }
  }

private:
  std::map<std::string, std::string> m_clipboardData;
  ClipboardChangeCallback m_changeCallback;
};

class MockPortalScope
{
public:
  MockPortal &portal()
  {
    static MockPortal stub;
    return stub;
  }
  bool isAvailable() const
  {
    return false;
  }
};

} // namespace deskflow::test
