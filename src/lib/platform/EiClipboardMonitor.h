/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/Wayland.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QObject>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace deskflow {

class EiClipboard;

//! Clipboard change monitoring for Wayland
/*!
This class monitors clipboard changes on Wayland using the XDG Desktop Portal
system. It provides callbacks when clipboard content changes and can detect
available formats.
*/
class EiClipboardMonitor : public QObject
{
  Q_OBJECT

public:
  //! Callback function type for clipboard changes
  using ChangeCallback = std::function<void(const std::vector<std::string> &mimeTypes)>;

  EiClipboardMonitor();
  ~EiClipboardMonitor();

  //! Start monitoring clipboard changes
  bool startMonitoring();

  //! Stop monitoring clipboard changes
  void stopMonitoring();

  //! Check if monitoring is active
  bool isMonitoring() const;

  //! Set callback for clipboard changes
  void setChangeCallback(ChangeCallback callback);

  //! Get currently available MIME types
  std::vector<std::string> getAvailableMimeTypes() const;

  //! Check if portal clipboard monitoring is available
  bool isPortalMonitoringAvailable() const;

private:
  //! Initialize portal connection for monitoring
  bool initPortalMonitoring();

  //! Cleanup portal resources
  void cleanupPortalMonitoring();

  //! Handle clipboard change notification
  void handleClipboardChange(const std::vector<std::string> &mimeTypes);

  //! Polling thread function (fallback when portal signals not available)
  void pollingThreadFunc();

  //! Check clipboard state for polling
  bool checkClipboardState();

private Q_SLOTS:
  void onClipboardChanged(QStringList mimeTypes);

private:
  // D-Bus and Portal state
  QDBusConnection m_dbusConnection;
  std::unique_ptr<QDBusInterface> m_portalInterface;
  bool m_portalMonitoringAvailable;

  // Monitoring state
  std::atomic<bool> m_monitoring;
  ChangeCallback m_changeCallback;
  mutable std::mutex m_callbackMutex;

  // Polling fallback
  std::unique_ptr<std::thread> m_pollingThread;
  std::atomic<bool> m_usePolling;
  std::chrono::milliseconds m_pollingInterval;

  // Last reported MIME types
  mutable std::mutex m_stateMutex;
  std::vector<std::string> m_lastMimeTypes;
  std::string m_lastClipboardHash;
};

} // namespace deskflow
