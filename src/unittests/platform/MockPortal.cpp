/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MockPortal.h"
#include "base/Log.h"

namespace deskflow::test {

#if WINAPI_LIBPORTAL

MockPortal::MockPortal() : m_mockPortal(nullptr), m_initialized(false)
{
}

MockPortal::~MockPortal()
{
  cleanup();
}

bool MockPortal::initialize()
{
  if (m_initialized) {
    return true;
  }

  try {
    // Create a mock portal instance
    // Note: This is a simplified mock - in a real implementation,
    // we would need to create a proper GObject mock
    m_mockPortal = reinterpret_cast<XdpPortal *>(g_object_new(G_TYPE_OBJECT, nullptr));

    if (m_mockPortal) {
      m_initialized = true;
      LOG_DEBUG("mock portal initialized");
      return true;
    }
  } catch (...) {
    LOG_ERR("exception while initializing mock portal");
  }

  return false;
}

void MockPortal::cleanup()
{
  if (m_mockPortal) {
    g_object_unref(m_mockPortal);
    m_mockPortal = nullptr;
  }
  m_initialized = false;
  m_clipboardData.clear();
}

void MockPortal::setClipboardData(const std::string &mimeType, const std::string &data)
{
  if (!m_initialized) {
    LOG_WARN("mock portal not initialized");
    return;
  }

  LOG_DEBUG("setting mock clipboard data: %s (%zu bytes)", mimeType.c_str(), data.size());
  m_clipboardData[mimeType] = data;
  notifyClipboardChange();
}

std::string MockPortal::getClipboardData(const std::string &mimeType) const
{
  if (!m_initialized) {
    return "";
  }

  auto it = m_clipboardData.find(mimeType);
  if (it != m_clipboardData.end()) {
    LOG_DEBUG("getting mock clipboard data: %s (%zu bytes)", mimeType.c_str(), it->second.size());
    return it->second;
  }

  LOG_DEBUG("mock clipboard data not found for MIME type: %s", mimeType.c_str());
  return "";
}

std::vector<std::string> MockPortal::getAvailableMimeTypes() const
{
  std::vector<std::string> mimeTypes;
  for (const auto &pair : m_clipboardData) {
    mimeTypes.push_back(pair.first);
  }
  return mimeTypes;
}

void MockPortal::clearClipboard()
{
  if (!m_initialized) {
    return;
  }

  LOG_DEBUG("clearing mock clipboard");
  m_clipboardData.clear();
  notifyClipboardChange();
}

void MockPortal::setClipboardChangeCallback(ClipboardChangeCallback callback)
{
  m_changeCallback = std::move(callback);
}

void MockPortal::simulateClipboardChange(const std::map<std::string, std::string> &data)
{
  if (!m_initialized) {
    LOG_WARN("mock portal not initialized");
    return;
  }

  LOG_DEBUG("simulating clipboard change with %zu MIME types", data.size());
  m_clipboardData = data;
  notifyClipboardChange();
}

bool MockPortal::isAvailable() const
{
  return m_initialized;
}

XdpPortal *MockPortal::getPortalInstance() const
{
  return m_mockPortal;
}

void MockPortal::notifyClipboardChange()
{
  if (m_changeCallback) {
    std::vector<std::string> mimeTypes = getAvailableMimeTypes();
    try {
      m_changeCallback(mimeTypes);
    } catch (...) {
      LOG_ERR("exception in mock clipboard change callback");
    }
  }
}

// MockPortalScope implementation

MockPortalScope::MockPortalScope() : m_mockPortal(std::make_unique<MockPortal>())
{
  m_mockPortal->initialize();
}

MockPortalScope::~MockPortalScope()
{
  if (m_mockPortal) {
    m_mockPortal->cleanup();
  }
}

MockPortal &MockPortalScope::portal()
{
  return *m_mockPortal;
}

bool MockPortalScope::isAvailable() const
{
  return m_mockPortal && m_mockPortal->isAvailable();
}

#endif

} // namespace deskflow::test
