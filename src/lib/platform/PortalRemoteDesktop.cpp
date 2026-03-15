#include "PortalRemoteDesktop.h"
#include "common/Log.h"
#include <gio/gio.h>

namespace deskflow {

PortalRemoteDesktop::PortalRemoteDesktop() = default;

PortalRemoteDesktop::~PortalRemoteDesktop() {
    cleanup();
}

bool PortalRemoteDesktop::init() {
    if (!startSession()) {
        LOG((CLOG_ERR "Failed to start XDG Desktop Portal session"));
        return false;
    }
    return true;
}

void PortalRemoteDesktop::cleanup() {
    disableClipboard();
    
    if (m_sessionProxy) {
        g_object_unref(m_sessionProxy);
        m_sessionProxy = nullptr;
    }
}

bool PortalRemoteDesktop::startSession() {
    GError* error = nullptr;
    
    // Connect to the session bus
    GDBusConnection* bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (!bus) {
        LOG((CLOG_ERR "Failed to connect to session bus: %s", error->message));
        g_error_free(error);
        return false;
    }

    // Create proxy for RemoteDesktop portal
    m_sessionProxy = g_dbus_proxy_new_sync(
        bus,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        nullptr,
        &error
    );
    
    g_object_unref(bus);
    
    if (!m_sessionProxy) {
        LOG((CLOG_ERR "Failed to create RemoteDesktop proxy: %s", error->message));
        g_error_free(error);
        return false;
    }
    
    return true;
}

void PortalRemoteDesktop::enableClipboard() {
    if (m_clipboardEnabled) return;
    
    setupClipboardProxy();
    m_clipboardEnabled = true;
    LOG((CLOG_INFO "Clipboard enabled via XDG Desktop Portal"));
}

void PortalRemoteDesktop::disableClipboard() {
    if (!m_clipboardEnabled) return;
    
    if (m_clipboardProxy) {
        g_object_unref(m_clipboardProxy);
        m_clipboardProxy = nullptr;
    }
    m_clipboardEnabled = false;
    LOG((CLOG_INFO "Clipboard disabled via XDG Desktop Portal"));
}

void PortalRemoteDesktop::setupClipboardProxy() {
    GError* error = nullptr;
    
    GDBusConnection* bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (!bus) {
        LOG((CLOG_ERR "Failed to connect to session bus for clipboard: %s", error->message));
        g_error_free(error);
        return;
    }

    m_clipboardProxy = g_dbus_proxy_new_sync(
        bus,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        nullptr,
        &error
    );
    
    g_object_unref(bus);
    
    if (!m_clipboardProxy) {
        LOG((CLOG_ERR "Failed to create Clipboard proxy: %s", error->message));
        g_error_free(error);
        return;
    }
}

bool PortalRemoteDesktop::setClipboard(ClipboardID id, const IClipboard* clipboard) {
    if (!m_clipboardEnabled || !m_clipboardProxy) {
        return false;
    }
    
    if (!clipboard) {
        return false;
    }
    
    // Get clipboard data in available formats
    m_clipboard.open(id);
    m_clipboard.close();
    m_clipboardData = m_clipboard.marshall();
    
    GError* error = nullptr;
    
    // Set selection via XDG Desktop Portal
    GVariant* result = g_dbus_proxy_call_sync(
        m_clipboardProxy,
        "SetSelection",
        g_variant_new("(sb)", "application/octet-stream", TRUE),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        LOG((CLOG_ERR "Failed to set clipboard selection: %s", error->message));
        g_error_free(error);
        return false;
    }
    
    g_variant_unref(result);
    LOG((CLOG_DEBUG "Clipboard selection set via XDG Desktop Portal"));
    return true;
}

void PortalRemoteDesktop::checkClipboard() {
    if (!m_clipboardEnabled || !m_clipboardProxy) {
        return;
    }
    
    GError* error = nullptr;
    
    // Query selection via XDG Desktop Portal
    GVariant* result = g_dbus_proxy_call_sync(
        m_clipboardProxy,
        "QuerySelection",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        LOG((CLOG_DEBUG "No clipboard selection available"));
        return;
    }
    
    g_variant_unref(result);
}

void PortalRemoteDesktop::handleSelectionTransfer(const std::string& mimeType, const std::vector<char>& data) {
    LOG((CLOG_DEBUG "Received clipboard selection transfer: %zu bytes, mime: %s", data.size(), mimeType.c_str()));
    
    // Process incoming clipboard data
    if (!data.empty()) {
        m_clipboardData.assign(data.begin(), data.end());
        m_clipboard.unmarshall(m_clipboardData, 0);
    }
}

} // namespace deskflow
