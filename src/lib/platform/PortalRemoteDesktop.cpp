#include "PortalRemoteDesktop.h"
#include "deskflow/Clipboard.h"

namespace deskflow {

class PortalRemoteDesktopImpl : public PortalRemoteDesktop
{
public:
    void setClipboard(IClipboard* clipboard) override
    {
        m_clipboard = clipboard;
    }

    void clipboardOwnershipChanged(bool owned) override
    {
        if (!owned) {
            m_clipboard = nullptr;
        }
    }

private:
    IClipboard* m_clipboard = nullptr;
};

std::unique_ptr<PortalRemoteDesktop> createPortalRemoteDesktop()
{
    return std::make_unique<PortalRemoteDesktopImpl>();
}

} // namespace deskflow
