/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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
CPrimaryScreen::mainLoop()
{
	// change our priority
	CThread::getCurrentThread().setPriority(-14);

	// run event loop
	try {
		LOG((CLOG_DEBUG "entering event loop"));
		onPreMainLoop();
		getScreen()->mainLoop();
		onPostMainLoop();
		LOG((CLOG_DEBUG "exiting event loop"));
	}
	catch (...) {
		onPostMainLoop();
		LOG((CLOG_DEBUG "exiting event loop"));
		throw;
	}
}

void
CPrimaryScreen::exitMainLoop()
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

		// reset options
		resetOptions();
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
	LOG((CLOG_INFO "entering primary at %d,%d%s", x, y, forScreensaver ? " for screen saver" : ""));
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
	LOG((CLOG_INFO "leaving primary"));
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

	// warp mouse to center
	warpCursorToCenter();

	// subclass hook
	onPostLeave(true);

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
CPrimaryScreen::onPreMainLoop()
{
	// do nothing
}

void
CPrimaryScreen::onPostMainLoop()
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
