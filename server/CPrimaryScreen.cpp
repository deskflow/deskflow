#include "CPrimaryScreen.h"
#include "IScreen.h"
#include "IScreenReceiver.h"
#include "ProtocolTypes.h"
#include "CLock.h"
#include "CThread.h"
#include "CLog.h"

//
// CPrimaryScreen
//

CPrimaryScreen::CPrimaryScreen(IScreenReceiver* receiver) :
	m_receiver(receiver),
	m_active(false)
{
	// do nothing
}

CPrimaryScreen::~CPrimaryScreen()
{
	// do nothing
}

void
CPrimaryScreen::run()
{
	// change our priority
	CThread::getCurrentThread().setPriority(-3);

	// run event loop
	try {
		log((CLOG_DEBUG "entering event loop"));
		onPreRun();
		getScreen()->mainLoop();
		onPostRun();
		log((CLOG_DEBUG "exiting event loop"));
	}
	catch (...) {
		onPostRun();
		log((CLOG_DEBUG "exiting event loop"));
		throw;
	}
}

void
CPrimaryScreen::stop()
{
	getScreen()->exitMainLoop();
}

void
CPrimaryScreen::open()
{
	CClientInfo info;
	try {
		// subclass hook
		onPreOpen();

		// open the screen
		getScreen()->open();

		// create and prepare our window
		createWindow();

		// collect screen info
		getScreen()->getShape(info.m_x, info.m_y, info.m_w, info.m_h);
		getScreen()->getCursorPos(info.m_mx, info.m_my);
		info.m_zoneSize = getJumpZoneSize();

		// update keyboard state
		updateKeys();

		// get notified of screen saver activation/deactivation
		getScreen()->openScreensaver(true);

		// subclass hook
		onPostOpen();
	}
	catch (...) {
		close();
		throw;
	}

	// enter the screen
	{
		CLock lock(&m_mutex);
		enterNoWarp();
	}

	// send screen info
	m_receiver->onInfoChanged(info);
}

void
CPrimaryScreen::close()
{
	onPreClose();
	getScreen()->closeScreensaver();
	destroyWindow();
	getScreen()->close();
	onPostClose();
}

void
CPrimaryScreen::enter(SInt32 x, SInt32 y, bool forScreensaver)
{
	log((CLOG_INFO "entering primary at %d,%d%s", x, y, forScreensaver ? " for screen saver" : ""));
	CLock lock(&m_mutex);
	assert(m_active == true);

	if (!forScreensaver) {
		warpCursor(x, y);
	}
	else {
		onEnterScreensaver();
	}
	enterNoWarp();
}

void
CPrimaryScreen::enterNoWarp()
{
	// note -- must be locked on entry

	// not active anymore
	m_active = false;

	// subclass hook
	onPreEnter();

	// restore active window and hide our window
	hideWindow();

	// subclass hook
	onPostEnter();
}

bool
CPrimaryScreen::leave()
{
	log((CLOG_INFO "leaving primary"));
	CLock lock(&m_mutex);
	assert(m_active == false);

	// subclass hook
	onPreLeave();

	// show our window
	if (!showWindow()) {
		onPostLeave(false);
		return false;
	}

	// get keyboard state as we leave
	updateKeys();

	// subclass hook
	onPostLeave(true);

	// warp mouse to center
	warpCursorToCenter();

	// local client now active
	m_active = true;

	// make sure our idea of clipboard ownership is correct
	getScreen()->checkClipboards();

	return true;
}

void
CPrimaryScreen::setClipboard(ClipboardID id,
				const IClipboard* clipboard)
{
	getScreen()->setClipboard(id, clipboard);
}

void
CPrimaryScreen::grabClipboard(ClipboardID id)
{
	getScreen()->setClipboard(id, NULL);
}

bool
CPrimaryScreen::isActive() const
{
	CLock lock(&m_mutex);
	return m_active;
}

void
CPrimaryScreen::getClipboard(ClipboardID id,
				IClipboard* clipboard) const
{
	getScreen()->getClipboard(id, clipboard);
}

void
CPrimaryScreen::onPreRun()
{
	// do nothing
}

void
CPrimaryScreen::onPostRun()
{
	// do nothing
}

void
CPrimaryScreen::onPreOpen()
{
	// do nothing
}

void
CPrimaryScreen::onPostOpen()
{
	// do nothing
}

void
CPrimaryScreen::onPreClose()
{
	// do nothing
}

void
CPrimaryScreen::onPostClose()
{
	// do nothing
}

void
CPrimaryScreen::onPreEnter()
{
	// do nothing
}

void
CPrimaryScreen::onPostEnter()
{
	// do nothing
}

void
CPrimaryScreen::onEnterScreensaver()
{
	// do nothing
}

void
CPrimaryScreen::onPreLeave()
{
	// do nothing
}

void
CPrimaryScreen::onPostLeave(bool)
{
	// do nothing
}
