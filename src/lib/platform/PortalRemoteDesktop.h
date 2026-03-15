#pragma once

#include <memory>

namespace deskflow {

class IClipboard;

class PortalRemoteDesktop
{
public:
    virtual ~PortalRemoteDesktop() = default;

    // Clipboard integration for XDG Desktop Portal (Wayland)
    virtual void setClipboard(IClipboard* clipboard) = 0;
    virtual void clipboardOwnershipChanged(bool owned) = 0;
};

std::unique_ptr<PortalRemoteDesktop> createPortalRemoteDesktop();

} // namespace deskflow
