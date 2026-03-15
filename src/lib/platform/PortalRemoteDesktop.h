#pragma once

#include "lib/deskflow/Clipboard.h"
#include <string>
#include <functional>
#include <memory>

namespace deskflow {
namespace platform {

class PortalRemoteDesktop {
public:
    PortalRemoteDesktop();
    ~PortalRemoteDesktop();

    bool connect();
    void disconnect();
    bool isConnected() const;

    // Clipboard integration for Wayland via XDG Desktop Portal
    void setClipboard(Clipboard* clipboard);
    Clipboard* clipboard() const;

    bool requestClipboard();
    bool setClipboardData(const std::string& mimeType, const std::string& data);
    std::string getClipboardData(const std::string& mimeType) const;

    // Callbacks for portal events
    using ClipboardCallback = std::function<void(const std::string& mimeType, const std::string& data)>;
    void setClipboardCallback(ClipboardCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    Clipboard* m_clipboard;
    ClipboardCallback m_clipboardCallback;
};

} // namespace platform
} // namespace deskflow
