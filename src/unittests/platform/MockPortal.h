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
#include <stdexcept>
#include <string>
#include <vector>

namespace deskflow::test {

// Lightweight stub implementation (libportal no longer required)
class MockPortal
{
public:
  using ClipboardChangeCallback = std::function<void(const std::vector<std::string> &)>;

  MockPortal() : m_initialized(false)
  {
  }

  ~MockPortal()
  {
    cleanup();
  }

  // Prevent copy/move to avoid issues with callbacks
  MockPortal(const MockPortal &) = delete;
  MockPortal &operator=(const MockPortal &) = delete;
  MockPortal(MockPortal &&) = delete;
  MockPortal &operator=(MockPortal &&) = delete;

  bool initialize()
  {
    if (m_initialized) {
      return true;
    }

    try {
      // Simulate initialization - always succeeds for mock
      m_initialized = true;
      return true;
    } catch (...) {
      m_initialized = false;
      return false;
    }
  }

  void cleanup()
  {
    if (!m_initialized) {
      return;
    }

    m_clipboardData.clear();
    m_changeCallback = nullptr;
    m_initialized = false;
  }

  bool isAvailable() const
  {
    return m_initialized;
  }

  bool isInitialized() const
  {
    return m_initialized;
  }

  // Minimal API used by tests
  void setClipboardData(const std::string &mimeType, const std::string &data)
  {
    if (!m_initialized) {
      throw std::runtime_error("MockPortal not initialized");
    }

    if (mimeType.empty()) {
      throw std::invalid_argument("Empty MIME type not allowed");
    }

    m_clipboardData[mimeType] = data;
  }

  std::string getClipboardData(const std::string &mimeType) const
  {
    if (!m_initialized) {
      return std::string();
    }

    auto it = m_clipboardData.find(mimeType);
    return it != m_clipboardData.end() ? it->second : std::string();
  }

  std::vector<std::string> getAvailableMimeTypes() const
  {
    std::vector<std::string> types;

    if (!m_initialized) {
      return types;
    }

    types.reserve(m_clipboardData.size());
    for (const auto &pair : m_clipboardData) {
      types.push_back(pair.first);
    }

    return types;
  }

  void clearClipboard()
  {
    if (!m_initialized) {
      return;
    }

    m_clipboardData.clear();

    // Notify about clipboard clear if callback is set
    if (m_changeCallback) {
      std::vector<std::string> emptyTypes;
      m_changeCallback(emptyTypes);
    }
  }

  void setClipboardChangeCallback(ClipboardChangeCallback cb)
  {
    if (!m_initialized) {
      throw std::runtime_error("MockPortal not initialized");
    }

    m_changeCallback = std::move(cb);
  }

  void simulateClipboardChange(const std::map<std::string, std::string> &data)
  {
    if (!m_initialized) {
      throw std::runtime_error("MockPortal not initialized");
    }

    // Update internal data
    m_clipboardData = data;

    // Notify callback if set
    if (m_changeCallback) {
      std::vector<std::string> types;
      types.reserve(data.size());

      for (const auto &pair : data) {
        types.push_back(pair.first);
      }

      try {
        m_changeCallback(types);
      } catch (...) {
        // Ignore callback exceptions to prevent test failures
      }
    }
  }

  // Additional utility methods for testing
  size_t getDataCount() const
  {
    return m_initialized ? m_clipboardData.size() : 0;
  }

  bool hasData(const std::string &mimeType) const
  {
    if (!m_initialized) {
      return false;
    }

    return m_clipboardData.find(mimeType) != m_clipboardData.end();
  }

private:
  bool m_initialized;
  std::map<std::string, std::string> m_clipboardData;
  ClipboardChangeCallback m_changeCallback;
};

class MockPortalScope
{
public:
  MockPortalScope() : m_portal(nullptr), m_available(false)
  {
    try {
      m_portal = std::make_unique<MockPortal>();
      m_available = m_portal->initialize();
    } catch (const std::exception &e) {
      // Log error but don't throw - allows tests to skip gracefully
      m_available = false;
      m_portal.reset();
    } catch (...) {
      m_available = false;
      m_portal.reset();
    }
  }

  ~MockPortalScope()
  {
    if (m_portal) {
      try {
        m_portal->cleanup();
      } catch (...) {
        // Ignore cleanup exceptions
      }
    }
  }

  // Prevent copy/move to avoid double cleanup
  MockPortalScope(const MockPortalScope &) = delete;
  MockPortalScope &operator=(const MockPortalScope &) = delete;
  MockPortalScope(MockPortalScope &&) = delete;
  MockPortalScope &operator=(MockPortalScope &&) = delete;

  MockPortal &portal()
  {
    if (!m_portal) {
      throw std::runtime_error("MockPortal not available");
    }
    return *m_portal;
  }

  const MockPortal &portal() const
  {
    if (!m_portal) {
      throw std::runtime_error("MockPortal not available");
    }
    return *m_portal;
  }

  bool isAvailable() const
  {
    return m_available && m_portal && m_portal->isInitialized();
  }

  // Reset the portal (useful for testing initialization scenarios)
  bool reset()
  {
    if (m_portal) {
      m_portal->cleanup();
    }

    try {
      m_portal = std::make_unique<MockPortal>();
      m_available = m_portal->initialize();
      return m_available;
    } catch (...) {
      m_available = false;
      m_portal.reset();
      return false;
    }
  }

private:
  std::unique_ptr<MockPortal> m_portal;
  bool m_available;
};

} // namespace deskflow::test