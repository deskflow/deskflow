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

#include "CSecondaryScreen.h"
#include "IScreen.h"
#include "CLock.h"
#include "CThread.h"
#include "CLog.h"

//
// CSecondaryScreen
//

CSecondaryScreen::CSecondaryScreen()
{
	// do nothing
}

CSecondaryScreen::~CSecondaryScreen()
{
	// do nothing
}

void
CSecondaryScreen::mainLoop()
{
	// change our priority
	CThread::getCurrentThread().setPriority(-7);

	// run event loop
	try {
		log((CLOG_DEBUG "entering event loop"));
		onPreMainLoop();
		getScreen()->mainLoop();
		onPostMainLoop();
		log((CLOG_DEBUG "exiting event loop"));
	}
	catch (...) {
		onPostMainLoop();
		log((CLOG_DEBUG "exiting event loop"));
		throw;
	}
}

void
CSecondaryScreen::exitMainLoop()
{
	getScreen()->exitMainLoop();
}

void
CSecondaryScreen::open()
{
	try {
		// subclass hook
		onPreOpen();

		// open the screen
		getScreen()->open();

		// create and prepare our window
		createWindow();

		// assume primary has all clipboards
		for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
			grabClipboard(id);
		}

		// update keyboard state
		updateKeys();

		// disable the screen saver
		getScreen()->openScreensaver(false);

		// subclass hook
		onPostOpen();
	}
	catch (...) {
		close();
		throw;
	}

	// hide the cursor
	{
		CLock lock(&m_mutex);
		m_active = true;
	}
	leave();
}

void
CSecondaryScreen::close()
{
	onPreClose();
	getScreen()->closeScreensaver();
	destroyWindow();
	getScreen()->close();
	onPostClose();
}

void
CSecondaryScreen::enter(SInt32 x, SInt32 y, KeyModifierMask mask)
{
	CLock lock(&m_mutex);
	assert(m_active == false);

	log((CLOG_INFO "entering screen at %d,%d mask=%04x", x, y, mask));

	getScreen()->syncDesktop();

	// now active
	m_active = true;

	// subclass hook
	onPreEnter();

	// update our keyboard state to reflect the local state
	updateKeys();

	// toggle modifiers that don't match the desired state
	setToggleState(mask);

	// warp to requested location
	warpCursor(x, y);

	// show mouse
	hideWindow();

	// subclass hook
	onPostEnter();
}

void
CSecondaryScreen::leave()
{
	log((CLOG_INFO "leaving screen"));
	CLock lock(&m_mutex);
	assert(m_active == true);

	getScreen()->syncDesktop();

	// subclass hook
	onPreLeave();

	// hide mouse
	showWindow();

	// subclass hook
	onPostLeave();

	// not active anymore
	m_active = false;

	// make sure our idea of clipboard ownership is correct
	getScreen()->checkClipboards();
}

void
CSecondaryScreen::setClipboard(ClipboardID id,
				const IClipboard* clipboard)
{
	getScreen()->setClipboard(id, clipboard);
}

void
CSecondaryScreen::grabClipboard(ClipboardID id)
{
	getScreen()->setClipboard(id, NULL);
}

void
CSecondaryScreen::screensaver(bool activate)
{
	getScreen()->screensaver(activate);
}

bool
CSecondaryScreen::isActive() const
{
	CLock lock(&m_mutex);
	return m_active;
}

void
CSecondaryScreen::getClipboard(ClipboardID id,
				IClipboard* clipboard) const
{
	getScreen()->getClipboard(id, clipboard);
}

void
CSecondaryScreen::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	getScreen()->syncDesktop();
	getScreen()->getShape(x, y, w, h);
}

void
CSecondaryScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	getScreen()->syncDesktop();
	getScreen()->getCursorPos(x, y);
}

void
CSecondaryScreen::onPreMainLoop()
{
	// do nothing
}

void
CSecondaryScreen::onPostMainLoop()
{
	// do nothing
}

void
CSecondaryScreen::onPreOpen()
{
	// do nothing
}

void
CSecondaryScreen::onPostOpen()
{
	// do nothing
}

void
CSecondaryScreen::onPreClose()
{
	// do nothing
}

void
CSecondaryScreen::onPostClose()
{
	// do nothing
}

void
CSecondaryScreen::onPreEnter()
{
	// do nothing
}

void
CSecondaryScreen::onPostEnter()
{
	// do nothing
}

void
CSecondaryScreen::onPreLeave()
{
	// do nothing
}

void
CSecondaryScreen::onPostLeave()
{
	// do nothing
}
