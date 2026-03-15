#include "PortalRemoteDesktop.h"
#include "common/log.h"
#include <giomm/dbus_connection.h>
#include <giomm/init.h>
#include <glibmm/main.h>
#include <iostream>

namespace deskflow {
namespace platform {

PortalRemoteDesktop::PortalRemoteDesktop()
    : m_clipboard(std::make_unique<Clipboard>()), m_clipboardEnabled(false) {
}

PortalRemoteDesktop::~PortalRemoteDesktop() {
    stop();
}

bool PortalRemoteDesktop::start() {
    try {
        Gio::init();
        auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION);
        if (!connection) {
            LOG((CLOG_ERR "Failed to connect to session bus"));
            return false;
        }

        m_portalProxy = Gio::DBus::Proxy::create_sync(
            connection,
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.RemoteDesktop");

        if (!m_portalProxy) {
            LOG((CLOG_ERR "Failed to create RemoteDesktop proxy"));
            return false;
        }

        // Setup clipboard proxy for XDP
        setupClipboardProxy();
        return true;
    } catch (const Glib::Error& e) {
        LOG((CLOG_ERR "PortalRemoteDesktop start error: %s", e.what().c_str()));
        return false;
    }
}

void PortalRemoteDesktop::stop() {
    disableClipboard();
    m_portalProxy.reset();
    m_clipboardProxy.reset();
}

void PortalRemoteDesktop::setupClipboardProxy() {
    try {
        auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION);
        m_clipboardProxy = Gio::DBus::Proxy::create_sync(
            connection,
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Clipboard");

        if (m_clipboardProxy) {
            m_clipboardProxy->signal_signal().connect(sigc::mem_fun(*this, &PortalRemoteDesktop::onClipboardChanged));
        }
    } catch (const Glib::Error& e) {
        LOG((CLOG_WARN "Clipboard proxy setup failed: %s", e.what().c_str()));
    }
}

void PortalRemoteDesktop::enableClipboard() {
    if (m_clipboardEnabled || !m_clipboardProxy) return;
    m_clipboardEnabled = true;
    LOG((CLOG_INFO "XDP Clipboard enabled"));
}

void PortalRemoteDesktop::disableClipboard() {
    m_clipboardEnabled = false;
}

std::string PortalRemoteDesktop::getClipboardData() {
    if (!m_clipboardEnabled || !m_clipboard) return "";
    return m_clipboard->get();
}

void PortalRemoteDesktop::setClipboardData(const std::string& data) {
    if (!m_clipboardEnabled || !m_clipboard || data.empty()) return;
    m_clipboard->set(data);
}

void PortalRemoteDesktop::onClipboardChanged() {
    if (!m_clipboardEnabled || !m_clipboard) return;
    // Handle clipboard change notification from XDP
    LOG((CLOG_DEBUG "XDP Clipboard changed signal received"));
}

} // namespace platform
} // namespace deskflow
