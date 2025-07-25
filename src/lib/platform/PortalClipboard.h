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
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#endif

namespace deskflow {

#if WINAPI_LIBPORTAL
//! XDG Desktop Portal clipboard interface wrapper
/*!
This class provides a high-level interface to the XDG Desktop Portal
clipboard functionality. It handles the D-Bus communication and provides
async/sync conversion for clipboard operations.

Note: This implementation is prepared for the future clipboard portal
interface that doesn't exist yet. When the interface becomes available,
the TODO comments should be replaced with actual portal API calls.
*/
class PortalClipboard
{
public:
    //! Clipboard operation result
    struct ClipboardResult {
        bool success;
        std::string error;
        std::string data;
        std::string mimeType;
    };

    //! Callback for clipboard change notifications
    using ChangeCallback = std::function<void(const std::vector<std::string>& mimeTypes)>;

    PortalClipboard();
    ~PortalClipboard();

    //! Initialize portal clipboard interface
    bool initialize();

    //! Cleanup portal resources
    void cleanup();

    //! Check if portal clipboard interface is available
    bool isAvailable() const;

    //! Set clipboard data (async)
    std::future<ClipboardResult> setClipboardAsync(
        const std::string& mimeType, 
        const std::string& data,
        const std::string& parentWindow = "");

    //! Get clipboard data (async)
    std::future<ClipboardResult> getClipboardAsync(
        const std::string& mimeType,
        const std::string& parentWindow = "");

    //! Set clipboard data (sync)
    ClipboardResult setClipboard(
        const std::string& mimeType, 
        const std::string& data,
        const std::string& parentWindow = "",
        int timeoutMs = 5000);

    //! Get clipboard data (sync)
    ClipboardResult getClipboard(
        const std::string& mimeType,
        const std::string& parentWindow = "",
        int timeoutMs = 5000);

    //! Get available MIME types
    std::vector<std::string> getAvailableMimeTypes() const;

    //! Clear clipboard
    bool clearClipboard(const std::string& parentWindow = "");

    //! Set clipboard change callback
    void setChangeCallback(ChangeCallback callback);

    //! Start monitoring clipboard changes
    bool startMonitoring();

    //! Stop monitoring clipboard changes
    void stopMonitoring();

private:
    //! Portal callback for set clipboard operation
    static void onSetClipboardReady(GObject* source, GAsyncResult* result, gpointer user_data);

    //! Portal callback for get clipboard operation
    static void onGetClipboardReady(GObject* source, GAsyncResult* result, gpointer user_data);

    //! Portal callback for clipboard change signal
    static void onClipboardChanged(XdpPortal* portal, const char* const* mime_types, gpointer user_data);

    //! Handle clipboard change notification
    void handleClipboardChange(const std::vector<std::string>& mimeTypes);

    //! Create parent window identifier for portal calls
    std::string createParentWindow(const std::string& parentWindow) const;

    //! Wait for async operation to complete
    ClipboardResult waitForOperation(std::shared_ptr<std::promise<ClipboardResult>> promise, int timeoutMs);

    // Portal state
    XdpPortal* m_portal;
    bool m_initialized;
    bool m_available;
    gulong m_changeSignalId;

    // Callback management
    ChangeCallback m_changeCallback;
    std::mutex m_callbackMutex;

    // Operation tracking
    std::mutex m_operationMutex;
    std::map<gpointer, std::shared_ptr<std::promise<ClipboardResult>>> m_pendingOperations;
};

#else
// Stub implementation for non-portal builds
class PortalClipboard
{
public:
    struct ClipboardResult {
        bool success = false;
        std::string error = "Portal not available";
        std::string data;
        std::string mimeType;
    };

    using ChangeCallback = std::function<void(const std::vector<std::string>&)>;

    bool initialize() { return false; }
    void cleanup() {}
    bool isAvailable() const { return false; }
    
    ClipboardResult setClipboard(const std::string&, const std::string&, const std::string& = "", int = 5000) {
        return ClipboardResult{};
    }
    
    ClipboardResult getClipboard(const std::string&, const std::string& = "", int = 5000) {
        return ClipboardResult{};
    }
    
    std::vector<std::string> getAvailableMimeTypes() const { return {}; }
    bool clearClipboard(const std::string& = "") { return false; }
    void setChangeCallback(ChangeCallback) {}
    bool startMonitoring() { return false; }
    void stopMonitoring() {}
};
#endif

} // namespace deskflow
