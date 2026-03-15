#pragma once

#include "deskflow/Clipboard.h"
#include <giomm/dbus_proxy.h>
#include <glibmm.h>
#include <memory>
#include <string>
#include <vector>

namespace deskflow {
namespace platform {

class PortalRemoteDesktop {
public:
    PortalRemoteDesktop();
    ~PortalRemoteDesktop();

    bool start();
    void stop();

    // Clipboard integration
    void enableClipboard();
    void disableClipboard();
    std::string getClipboardData();
    void setClipboardData(const std::string& data);

private:
    void onClipboardChanged();
    void setupClipboardProxy();

    Glib::RefPtr<Gio::DBus::Proxy> m_portalProxy;
    Glib::RefPtr<Gio::DBus::Proxy> m_clipboardProxy;
    std::unique_ptr<Clipboard> m_clipboard;
    bool m_clipboardEnabled;
    std::string m_sessionHandle;
};

} // namespace platform
} // namespace deskflow
