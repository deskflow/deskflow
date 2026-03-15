#pragma once

#include "deskflow/Clipboard.h"
#include <string>
#include <vector>
#include <memory>

// Forward declarations for XDG Desktop Portal types (simplified for patch)
struct _GDBusProxy;
typedef struct _GDBusProxy GDBusProxy;

namespace deskflow {

class PortalRemoteDesktop {
public:
    PortalRemoteDesktop();
    ~PortalRemoteDesktop();

    bool init();
    void cleanup();
    
    // Clipboard integration
    void enableClipboard();
    void disableClipboard();
    bool setClipboard(ClipboardID id, const IClipboard* clipboard);
    void checkClipboard();

private:
    GDBusProxy* m_sessionProxy = nullptr;
    GDBusProxy* m_clipboardProxy = nullptr;
    bool m_clipboardEnabled = false;
    Clipboard m_clipboard;
    std::string m_clipboardData;
    
    bool startSession();
    void setupClipboardProxy();
    void handleSelectionTransfer(const std::string& mimeType, const std::vector<char>& data);
};

} // namespace deskflow
