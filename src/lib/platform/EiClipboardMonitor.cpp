/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboardMonitor.h"
#include "base/Log.h"

#include <chrono>
#include <thread>

namespace deskflow {

EiClipboardMonitor::EiClipboardMonitor()
#if WINAPI_LIBPORTAL
    : m_portal(nullptr),
      m_signalId(0),
      m_portalMonitoringAvailable(false),
      m_monitoring(false),
      m_usePolling(false),
      m_pollingInterval(std::chrono::milliseconds(500))
#else
    : m_monitoring(false)
#endif
{
#if WINAPI_LIBPORTAL
  if (deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard) {
    initPortalMonitoring();
  } else {
    LOG_DEBUG("portal clipboard monitoring not available");
  }
#else
  LOG_DEBUG("compiled without portal support, clipboard monitoring disabled");
#endif
}

EiClipboardMonitor::~EiClipboardMonitor()
{
  stopMonitoring();
#if WINAPI_LIBPORTAL
  cleanupPortalMonitoring();
#endif
}

bool EiClipboardMonitor::startMonitoring()
{
  if (m_monitoring.load()) {
    LOG_WARN("clipboard monitoring already active");
    return true;
  }

#if WINAPI_LIBPORTAL
  if (m_portalMonitoringAvailable) {
    LOG_INFO("starting portal clipboard monitoring");
    m_monitoring = true;
    return true;
  } else if (deskflow::platform::kHasPortal) {
    // Fallback to polling if portal signals not available
    LOG_INFO("starting polling clipboard monitoring (portal signals not available)");
    m_usePolling = true;
    m_monitoring = true;

    m_pollingThread = std::make_unique<std::thread>(&EiClipboardMonitor::pollingThreadFunc, this);
    return true;
  }
#endif

  LOG_WARN("clipboard monitoring not available");
  return false;
}

void EiClipboardMonitor::stopMonitoring()
{
  if (!m_monitoring.load()) {
    return;
  }

  LOG_DEBUG("stopping clipboard monitoring");
  m_monitoring = false;

#if WINAPI_LIBPORTAL
  if (m_pollingThread && m_pollingThread->joinable()) {
    m_pollingThread->join();
    m_pollingThread.reset();
  }
  m_usePolling = false;
#endif
}

bool EiClipboardMonitor::isMonitoring() const
{
  return m_monitoring.load();
}

void EiClipboardMonitor::setChangeCallback(ChangeCallback callback)
{
#if WINAPI_LIBPORTAL
  std::lock_guard<std::mutex> lock(m_callbackMutex);
#endif
  m_changeCallback = std::move(callback);
}

std::vector<std::string> EiClipboardMonitor::getAvailableMimeTypes() const
{
#if WINAPI_LIBPORTAL
  std::lock_guard<std::mutex> lock(m_stateMutex);
  return m_lastMimeTypes;
#else
  return {};
#endif
}

bool EiClipboardMonitor::isPortalMonitoringAvailable() const
{
#if WINAPI_LIBPORTAL
  return m_portalMonitoringAvailable;
#else
  return false;
#endif
}

#if WINAPI_LIBPORTAL
bool EiClipboardMonitor::initPortalMonitoring()
{
  try {
    m_portal = xdp_portal_new();
    if (!m_portal) {
      LOG_WARN("failed to create portal for monitoring");
      return false;
    }

    // TODO: Connect to clipboard change signal when API is available
    // m_signalId = g_signal_connect(m_portal, "clipboard-changed",
    //                               G_CALLBACK(onPortalClipboardChanged), this);

    // For now, assume portal monitoring is not available since interface doesn't exist
    m_portalMonitoringAvailable = false;
    LOG_DEBUG("portal monitoring initialized (signals not yet available)");

    return true;
  } catch (...) {
    LOG_ERR("exception while initializing portal monitoring");
    return false;
  }
}

void EiClipboardMonitor::cleanupPortalMonitoring()
{
  if (m_portal) {
    if (m_signalId > 0) {
      g_signal_handler_disconnect(m_portal, m_signalId);
      m_signalId = 0;
    }
    g_object_unref(m_portal);
    m_portal = nullptr;
  }
}

void EiClipboardMonitor::onPortalClipboardChanged(XdpPortal *portal, const char *const *mime_types, gpointer user_data)
{
  auto *monitor = static_cast<EiClipboardMonitor *>(user_data);

  std::vector<std::string> mimeTypeList;
  if (mime_types) {
    for (int i = 0; mime_types[i] != nullptr; ++i) {
      mimeTypeList.emplace_back(mime_types[i]);
    }
  }

  monitor->handleClipboardChange(mimeTypeList);
}

void EiClipboardMonitor::handleClipboardChange(const std::vector<std::string> &mimeTypes)
{
  LOG_DEBUG("clipboard changed, %zu MIME types available", mimeTypes.size());

  {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_lastMimeTypes = mimeTypes;
  }

  {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_changeCallback) {
      try {
        m_changeCallback(mimeTypes);
      } catch (...) {
        LOG_ERR("exception in clipboard change callback");
      }
    }
  }
}

void EiClipboardMonitor::pollingThreadFunc()
{
  LOG_DEBUG("starting clipboard polling thread");

  while (m_monitoring.load()) {
    try {
      if (checkClipboardState()) {
        // Clipboard changed, get available types
        // TODO: Implement actual clipboard state checking when portal API available
        std::vector<std::string> currentTypes = {"text/plain"}; // Placeholder
        handleClipboardChange(currentTypes);
      }
    } catch (...) {
      LOG_ERR("exception in clipboard polling thread");
    }

    std::this_thread::sleep_for(m_pollingInterval);
  }

  LOG_DEBUG("clipboard polling thread stopped");
}

bool EiClipboardMonitor::checkClipboardState()
{
  // TODO: Implement actual clipboard state checking
  // This would involve:
  // 1. Getting current clipboard content hash
  // 2. Comparing with last known state
  // 3. Detecting available MIME types

  // For now, return false (no change detected)
  return false;
}
#endif

} // namespace deskflow
