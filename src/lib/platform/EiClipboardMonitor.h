/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/Wayland.h"

#if WINAPI_LIBPORTAL
#include <libportal/portal.h>
#include <glib.h>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#endif

namespace deskflow {

class EiClipboard;

//! Clipboard change monitoring for Wayland
/*!
This class monitors clipboard changes on Wayland using the XDG Desktop Portal
system. It provides callbacks when clipboard content changes and can detect
available formats.
*/
class EiClipboardMonitor
{
public:
    //! Callback function type for clipboard changes
    using ChangeCallback = std::function<void(const std::vector<std::string>& mimeTypes)>;

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
#if WINAPI_LIBPORTAL
    //! Initialize portal connection for monitoring
    bool initPortalMonitoring();

    //! Cleanup portal resources
    void cleanupPortalMonitoring();

    //! Portal callback for clipboard changes
    static void onPortalClipboardChanged(XdpPortal* portal, const char* const* mime_types, gpointer user_data);

    //! Handle clipboard change notification
    void handleClipboardChange(const std::vector<std::string>& mimeTypes);

    //! Polling thread function (fallback when portal signals not available)
    void pollingThreadFunc();

    //! Check clipboard state for polling
    bool checkClipboardState();

    // Portal state
    XdpPortal* m_portal;
    gulong m_signalId;
    bool m_portalMonitoringAvailable;

    // Monitoring state
    std::atomic<bool> m_monitoring;
    ChangeCallback m_changeCallback;
    mutable std::mutex m_callbackMutex;

    // Polling fallback
    std::unique_ptr<std::thread> m_pollingThread;
    std::atomic<bool> m_usePolling;
    std::chrono::milliseconds m_pollingInterval;

    // State tracking
    mutable std::mutex m_stateMutex;
    std::vector<std::string> m_lastMimeTypes;
    std::string m_lastClipboardHash;
#else
    // Stub members for non-portal builds
    std::atomic<bool> m_monitoring;
    ChangeCallback m_changeCallback;
#endif
};

} // namespace deskflow
