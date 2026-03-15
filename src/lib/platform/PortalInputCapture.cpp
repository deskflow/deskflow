#include "PortalInputCapture.h"
#include "config.h"

#include <glib.h>
#include <gio/gio.h>
#include <libportal/portal.h>
#include <libportal/inputcapture.h>

#if HAVE_LIBPORTAL_0_8
#define XDP_INPUT_CAPTURE_SESSION_CONNECT_TO_EIS_FLAGS XDG_DESKTOP_PORTAL_USE_TOKEN
#else
#define XDP_INPUT_CAPTURE_SESSION_CONNECT_TO_EIS_FLAGS 0
#endif

PortalInputCapture::PortalInputCapture(
    IEventQueue *events, const char *screenName, bool enableCrypto, IEventQueueBuffer &eventQueueBuffer)
    : PlatformScreen(events, screenName, enableCrypto),
      m_portal(nullptr),
      m_session(nullptr),
      m_connectToEISCalled(false),
      m_eventQueueBuffer(eventQueueBuffer)
{
}

PortalInputCapture::~PortalInputCapture()
{
    if (m_session) {
        g_object_unref(m_session);
        m_session = nullptr;
    }
    if (m_portal) {
        g_object_unref(m_portal);
        m_portal = nullptr;
    }
}

void PortalInputCapture::enable()
{
    if (!m_portal) {
        m_portal = xdp_portal_new();
    }

    if (!m_session) {
        g_autoptr(GError) error = nullptr;
        XdpInputCaptureSession *session = nullptr;

        session = xdp_input_capture_session_new(m_portal, nullptr, &error);
        if (!session) {
            g_warning("Failed to create input capture session: %s", error->message);
            return;
        }
        m_session = session;
    }

    if (!m_connectToEISCalled) {
        g_autoptr(GError) error = nullptr;
        int fd = -1;

        // 尝试使用持久化授权标志连接到 EIS
        fd = xdp_input_capture_session_connect_to_eis(m_session, XDP_INPUT_CAPTURE_SESSION_CONNECT_TO_EIS_FLAGS, &error);
        if (fd < 0) {
            g_warning("Failed to connect to EIS with persist flag: %s. Retrying without persist flag.", error->message);
            g_clear_error(&error);
            // 回退到普通连接（不使用持久化标志）
            fd = xdp_input_capture_session_connect_to_eis(m_session, 0, &error);
            if (fd < 0) {
                g_warning("Failed to connect to EIS: %s", error->message);
                return;
            }
        }

        m_connectToEISCalled = true;
        // 此处省略将 fd 传递给 Barrier / Screen 处理的代码
        // 实际使用时应在后续逻辑中关闭 fd
        close(fd);
    }
}

void PortalInputCapture::disable()
{
    if (m_session) {
        g_object_unref(m_session);
        m_session = nullptr;
    }
    m_connectToEISCalled = false;
}

void PortalInputCapture::enter()
{
    // 待实现
}

void PortalInputCapture::leave()
{
    // 待实现
}

bool PortalInputCapture::canRestart()
{
    return true;
}

void PortalInputCapture::reconfigure(UInt32)
{
    // 待实现
}

void PortalInputCapture::updateKeymap()
{
    // 待实现
}

void PortalInputCapture::fakeKeyDown(KeyID, KeyModifierMask, KeyButton)
{
    // 待实现
}

void PortalInputCapture::fakeKeyRepeat(KeyID, KeyModifierMask, SInt32, KeyButton)
{
    // 待实现
}

void PortalInputCapture::fakeKeyUp(KeyButton)
{
    // 待实现
}

void PortalInputCapture::fakeAllKeysUp()
{
    // 待实现
}

SInt32 PortalInputCapture::getJumpZoneSize() const
{
    return 1;
}

bool PortalInputCapture::isPrimary() const
{
    return true;
}

void* PortalInputCapture::getEventTarget() const
{
    return const_cast<PortalInputCapture*>(this);
}

bool PortalInputCapture::getClipboard(ClipboardID, IClipboard*) const
{
    return false;
}

void PortalInputCapture::getShape(SInt32&, SInt32&, SInt32&, SInt32&) const
{
    // 待实现
}

void PortalInputCapture::getCursorPos(SInt32&, SInt32&) const
{
    // 待实现
}

void PortalInputCapture::resetOptions()
{
    // 待实现
}

void PortalInputCapture::setOptions(const OptionsList&)
{
    // 待实现
}

void PortalInputCapture::setSequenceNumber(UInt32)
{
    // 待实现
}

bool PortalInputCapture::isOnScreen() const
{
    return true;
}

void PortalInputCapture::fakeMouseButton(ButtonID, bool)
{
    // 待实现
}

void PortalInputCapture::fakeMouseMove(SInt32, SInt32)
{
    // 待实现
}

void PortalInputCapture::fakeMouseRelativeMove(SInt32, SInt32) const
{
    // 待实现
}

void PortalInputCapture::fakeMouseWheel(SInt32, SInt32) const
{
    // 待实现
}

bool PortalInputCapture::fakeMediaKey(KeyID)
{
    return false;
}

bool PortalInputCapture::fakeHotKey(KeyID, KeyModifierMask)
{
    return false;
}

void PortalInputCapture::enableDragDrop(bool)
{
    // 待实现
}

void PortalInputCapture::setDragging(bool)
{
    // 待实现
}

bool PortalInputCapture::isDraggingEnabled() const
{
    return false;
}

bool PortalInputCapture::isDraggingActive() const
{
    return false;
}

void PortalInputCapture::setDropTarget(void*)
{
    // 待实现
}

bool PortalInputCapture::setDrop(ClipboardID, const String&, const String&, void*)
{
    return false;
}

void PortalInputCapture::clearDrop()
{
    // 待实现
}

bool PortalInputCapture::hasDrop(ClipboardID) const
{
    return false;
}

void PortalInputCapture::setHalfDuplex(bool)
{
    // 待实现
}

void PortalInputCapture::setSecureInput(bool) const
{
    // 待实现
}
