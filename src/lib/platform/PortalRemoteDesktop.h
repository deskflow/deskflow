#pragma once

#include "deskflow/Clipboard.h"
#include <glib.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations for XDP types if needed or include specific headers
// Assuming xdp.h is available or we use GDBus directly for minimal implementation

// Minimal XDP constants for Clipboard (if not in a common header)
#define XDP_DESKTOP_PORTAL_OBJECT_PATH "/org/freedesktop/portal/desktop"
#define XDP_DESKTOP_PORTAL_INTERFACE "org.freedesktop.impl.portal.RemoteDesktop"
#define XDP_DESKTOP_PORTAL_REQUEST_INTERFACE "org.freedesktop.portal.Request"

// Generic type alias for clarity in this patch context
using ClipboardID = uint32_t;

class PortalRemoteDesktop {
public:
    PortalRemoteDesktop();
    ~PortalRemoteDesktop();

    // Entry point to enable clipboard handling
    void enableClipboard();
    
    // Method to set the clipboard content (from local to remote)
    void setClipboard(const deskflow::Clipboard::EFormat format, const std::string& data);

private:
    // GDBus connection and signal handlers
    GDBusConnection* m_connection = nullptr;
    guint m_signalSubscriptionId = 0;
    guint m_ownerId = 0;
    
    // Clipboard state
    std::unique_ptr<deskflow::Clipboard> m_clipboard;
    ClipboardID m_clipboardId = 0;

    void connectToPortal();
    static void onSignalReceived(GDBusConnection* connection,
                                 const gchar* sender_name,
                                 const gchar* object_path,
                                 const gchar* interface_name,
                                 const gchar* signal_name,
                                 GVariant* parameters,
                                 gpointer user_data);
    
    void handleSelectionTransfer(GVariant* parameters);
    void sendClipboardData(ClipboardID id, const std::string& mimeType);
};
