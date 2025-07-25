/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboardMonitor.h"
#include "base/Log.h"

#include <chrono>
#include <thread>

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QStringList>
#include <QVariant>

namespace deskflow {

EiClipboardMonitor::EiClipboardMonitor()
    : QObject(nullptr),
      m_dbusConnection(QDBusConnection::sessionBus()),
      m_portalInterface(nullptr),
      m_portalMonitoringAvailable(false),
      m_monitoring(false),
      m_usePolling(false),
      m_pollingInterval(std::chrono::milliseconds(500))
{
  if (m_dbusConnection.isConnected()) {
    initPortalMonitoring();
  } else {
    LOG_WARN("failed to connect to D-Bus session bus, clipboard monitoring disabled");
  }
}

EiClipboardMonitor::~EiClipboardMonitor()
{
  stopMonitoring();
  cleanupPortalMonitoring();
}

bool EiClipboardMonitor::startMonitoring()
{
  if (m_monitoring.load()) {
    LOG_WARN("clipboard monitoring already active");
    return true;
  }

  if (m_portalMonitoringAvailable) {
    LOG_INFO("starting portal clipboard monitoring");
    m_monitoring = true;
    return true;
  } else {
    // Fallback to polling if portal signals not available
    LOG_INFO("starting polling clipboard monitoring (portal signals not available)");
    m_usePolling = true;
    m_monitoring = true;

    m_pollingThread = std::make_unique<std::thread>(&EiClipboardMonitor::pollingThreadFunc, this);
    return true;
  }

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

  if (m_pollingThread && m_pollingThread->joinable()) {
    m_pollingThread->join();
    m_pollingThread.reset();
  }
  m_usePolling = false;
}

bool EiClipboardMonitor::isMonitoring() const
{
  return m_monitoring.load();
}

void EiClipboardMonitor::setChangeCallback(ChangeCallback callback)
{
  std::scoped_lock<std::mutex> lock(m_callbackMutex);
  m_changeCallback = std::move(callback);
}

std::vector<std::string> EiClipboardMonitor::getAvailableMimeTypes() const
{
  std::scoped_lock<std::mutex> lock(m_stateMutex);
  return m_lastMimeTypes;
}

bool EiClipboardMonitor::isPortalMonitoringAvailable() const
{
  return m_portalMonitoringAvailable;
}

bool EiClipboardMonitor::initPortalMonitoring()
{
  try {
    if (!m_dbusConnection.isConnected()) {
      LOG_WARN("failed to connect to D-Bus session bus");
      return false;
    }

    // Check if portal service is available
    QDBusInterface portalService(
        "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.DBus.Properties",
        m_dbusConnection
    );

    if (!portalService.isValid()) {
      LOG_WARN("XDG Desktop Portal service not available");
      return false;
    }

    // Create interface for clipboard portal
    m_portalInterface = std::make_unique<QDBusInterface>(
        "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Clipboard",
        m_dbusConnection
    );

    if (!m_portalInterface->isValid()) {
      LOG_WARN("XDG-Clipboard portal interface not available");
      return false;
    }

    // Connect to clipboard-changed signal
    bool connected = m_dbusConnection.connect(
        "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Clipboard",
        "ClipboardChanged", this, "onClipboardChanged(QStringList)"
    );

    if (!connected) {
      LOG_WARN("failed to connect to ClipboardChanged signal");
      return false;
    }

    m_portalMonitoringAvailable = true;
    LOG_INFO("portal monitoring initialized successfully");

    return true;
  } catch (...) {
    LOG_ERR("exception while initializing portal monitoring");
    m_portalMonitoringAvailable = false;
    return false;
  }
}

void EiClipboardMonitor::cleanupPortalMonitoring()
{
  if (m_portalInterface) {
    m_dbusConnection.disconnect(
        "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Clipboard",
        "ClipboardChanged", this, "onClipboardChanged(QStringList)"
    );
    m_portalInterface.reset();
  }
  m_portalMonitoringAvailable = false;
}

void EiClipboardMonitor::onClipboardChanged(QStringList mimeTypes)
{
  std::vector<std::string> mimeTypeList;
  for (const QString &mimeType : mimeTypes) {
    mimeTypeList.emplace_back(mimeType.toStdString());
  }
  handleClipboardChange(mimeTypeList);
}

void EiClipboardMonitor::handleClipboardChange(const std::vector<std::string> &mimeTypes)
{
  LOG_DEBUG("clipboard changed, %zu MIME types available", mimeTypes.size());

  {
    std::scoped_lock<std::mutex> lock(m_stateMutex);
    m_lastMimeTypes = mimeTypes;
  }

  {
    std::scoped_lock<std::mutex> lock(m_callbackMutex);
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

} // namespace deskflow
