#include "CPrimaryClient.h"
#include "IServer.h"
#include "IPrimaryScreen.h"
#include "CClipboard.h"
#include "CLog.h"

// FIXME -- use factory to create screen
#if WINDOWS_LIKE
#include "CMSWindowsPrimaryScreen.h"
#elif UNIX_LIKE
#include "CXWindowsPrimaryScreen.h"
#endif

//
// CPrimaryClient
//

CPrimaryClient::CPrimaryClient(IServer* server, const CString& name) :
	m_server(server),
	m_name(name),
	m_seqNum(0)
{
	assert(m_server != NULL);

	// create screen
	log((CLOG_DEBUG1 "creating primary screen"));
#if WINDOWS_LIKE
	m_screen = new CMSWindowsPrimaryScreen(this, m_server);
#elif UNIX_LIKE
	m_screen = new CXWindowsPrimaryScreen(this, m_server);
#endif
}

CPrimaryClient::~CPrimaryClient()
{
	delete m_screen;
}

void
CPrimaryClient::stop()
{
	m_screen->stop();
}

void
CPrimaryClient::reconfigure(UInt32 activeSides)
{
	m_screen->reconfigure(activeSides);
}

void
CPrimaryClient::getClipboard(ClipboardID id, CString& data) const
{
	CClipboard clipboard;
	m_screen->getClipboard(id, &clipboard);
	data = clipboard.marshall();
}

bool
CPrimaryClient::isLockedToScreen() const
{
	return m_screen->isLockedToScreen();
}

KeyModifierMask
CPrimaryClient::getToggleMask() const
{
	return m_screen->getToggleMask();
}

void
CPrimaryClient::onInfoChanged(const CClientInfo& info)
{
	m_info = info;
	m_server->onInfoChanged(getName(), m_info);
}

bool
CPrimaryClient::onGrabClipboard(ClipboardID id)
{
	bool result = m_server->onGrabClipboard(getName(), id, m_seqNum);
	m_clipboardOwner[id] = result;
	return result;
}

void
CPrimaryClient::onClipboardChanged(ClipboardID id, const CString& data)
{
	m_server->onClipboardChanged(id, m_seqNum, data);
}

bool
CPrimaryClient::open()
{
	// all clipboards are clean and owned by us
	for (UInt32 i = 0; i < kClipboardEnd; ++i) {
		m_clipboardOwner[i] = true;
		m_clipboardDirty[i] = false;
	}

	// now open the screen
	m_screen->open();

	return true;
}

void
CPrimaryClient::run()
{
	m_screen->run();
}

void
CPrimaryClient::close()
{
	m_screen->close();
}

void
CPrimaryClient::enter(SInt32 xAbs, SInt32 yAbs,
				UInt32 seqNum, KeyModifierMask, bool screensaver)
{
	// note -- we must not call any server methods except onError().
	m_seqNum = seqNum;
	m_screen->enter(xAbs, yAbs, screensaver);
}

bool
CPrimaryClient::leave()
{
	// note -- we must not call any server methods except onError().
	return m_screen->leave();
}

void
CPrimaryClient::setClipboard(ClipboardID id, const CString& data)
{
	// note -- we must not call any server methods except onError().

	// ignore if this clipboard is already clean
	if (m_clipboardDirty[id]) {
		// this clipboard is now clean
		m_clipboardDirty[id] = false;

		// unmarshall data
		CClipboard clipboard;
		clipboard.unmarshall(data, 0);

		// set clipboard
		m_screen->setClipboard(id, &clipboard);
	}
}

void
CPrimaryClient::grabClipboard(ClipboardID id)
{
	// grab clipboard
	m_screen->grabClipboard(id);

	// clipboard is dirty (because someone else owns it now)
	m_clipboardOwner[id] = false;
	m_clipboardDirty[id] = true;
}

void
CPrimaryClient::setClipboardDirty(ClipboardID id, bool dirty)
{
	m_clipboardDirty[id] = dirty;
}

void
CPrimaryClient::keyDown(KeyID, KeyModifierMask)
{
	// ignore
}

void
CPrimaryClient::keyRepeat(KeyID, KeyModifierMask, SInt32)
{
	// ignore
}

void
CPrimaryClient::keyUp(KeyID, KeyModifierMask)
{
	// ignore
}

void
CPrimaryClient::mouseDown(ButtonID)
{
	// ignore
}

void
CPrimaryClient::mouseUp(ButtonID)
{
	// ignore
}

void
CPrimaryClient::mouseMove(SInt32 x, SInt32 y)
{
	m_screen->warpCursor(x, y);
}

void
CPrimaryClient::mouseWheel(SInt32)
{
	// ignore
}

void
CPrimaryClient::screenSaver(bool)
{
	// ignore
}

CString
CPrimaryClient::getName() const
{
	return m_name;
}

void
CPrimaryClient::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	x = m_info.m_x;
	y = m_info.m_y;
	w = m_info.m_w;
	h = m_info.m_h;
}

void
CPrimaryClient::getCenter(SInt32& x, SInt32& y) const
{
	x = m_info.m_mx;
	y = m_info.m_my;
}

void
CPrimaryClient::getMousePos(SInt32&, SInt32&) const
{
	assert(0 && "shouldn't be called");
}

SInt32
CPrimaryClient::getJumpZoneSize() const
{
	return m_info.m_zoneSize;
}
