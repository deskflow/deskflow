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

CSecondaryScreen::CSecondaryScreen() :
	m_remoteReady(false),
	m_active(false),
	m_toggleKeys(0),
	m_screenSaverSync(true)
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

		// create and prepare our window.  pretend we're active so
		// we don't try to show our window until later.
		{
			CLock lock(&m_mutex);
			assert(m_active == false);
			m_active = true;
		}
		createWindow();
		{
			CLock lock(&m_mutex);
			m_active = false;
		}

		// subclass hook
		onPostOpen();

		// reset options
		resetOptions();
	}
	catch (...) {
		close();
		throw;
	}
}

void
CSecondaryScreen::close()
{
	onPreClose();
	destroyWindow();
	getScreen()->close();
	onPostClose();
}

void
CSecondaryScreen::remoteControl()
{
	// assume primary has all clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		grabClipboard(id);
	}

	// update keyboard state
	updateKeys();

	// now remote ready.  fake being active for call to leave().
	bool screenSaverSync;
	{
		CLock lock(&m_mutex);
		m_remoteReady = true;
		m_active      = true;

		// copy screen saver synchronization state
		screenSaverSync = m_screenSaverSync;
	}

	// disable the screen saver if synchronization is enabled
	if (screenSaverSync) {
		getScreen()->openScreensaver(false);
	}

	// hide the cursor
	leave();
}

void
CSecondaryScreen::localControl()
{
	getScreen()->closeScreensaver();

	// not remote ready anymore
	CLock lock(&m_mutex);
	m_remoteReady = false;
}

void
CSecondaryScreen::enter(SInt32 x, SInt32 y, KeyModifierMask mask)
{
	CLock lock(&m_mutex);
	assert(m_active == false);

	LOG((CLOG_INFO "entering screen at %d,%d mask=%04x", x, y, mask));

	getScreen()->syncDesktop();

	// now active
	m_active = true;

	// subclass hook
	onPreEnter();

	// update our keyboard state to reflect the local state
	updateKeys();

	// remember toggle key state
	m_toggleKeys = getToggleState();

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
	LOG((CLOG_INFO "leaving screen"));
	CLock lock(&m_mutex);
	assert(m_active == true);

	getScreen()->syncDesktop();

	// subclass hook
	onPreLeave();

	// restore toggle key state
	setToggleState(m_toggleKeys);

	// warp and hide mouse
	SInt32 x, y;
	getScreen()->getCursorCenter(x, y);
	showWindow(x, y);

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
	// get screen saver synchronization flag
	bool screenSaverSync;
	{
		CLock lock(&m_mutex);
		screenSaverSync = m_screenSaverSync;
	}

	// activate/deactivation screen saver iff synchronization enabled
	if (screenSaverSync) {
		getScreen()->screensaver(activate);
	}
}

void
CSecondaryScreen::resetOptions()
{
	// set screen saver synchronization flag and see if we need to
	// update the screen saver synchronization.
	bool screenSaverSyncOn;
	{
		CLock lock(&m_mutex);
		screenSaverSyncOn = (!m_screenSaverSync && m_remoteReady);
		m_screenSaverSync = true;
	}

	// update screen saver synchronization
	if (screenSaverSyncOn) {
		getScreen()->openScreensaver(false);
	}
}

void
CSecondaryScreen::setOptions(const COptionsList& options)
{
	// update options
	bool updateScreenSaverSync = false;
	bool oldScreenSaverSync;
	{
		CLock lock(&m_mutex);
		oldScreenSaverSync = m_screenSaverSync;
		for (UInt32 i = 0, n = options.size(); i < n; i += 2) {
			if (options[i] == kOptionScreenSaverSync) {
				updateScreenSaverSync = true;
				m_screenSaverSync     = (options[i + 1] != 0);
				LOG((CLOG_DEBUG1 "screen saver synchronization %s", m_screenSaverSync ? "on" : "off"));
			}
		}
		if (!m_remoteReady || oldScreenSaverSync == m_screenSaverSync) {
			updateScreenSaverSync = false;
		}
	}

	// update screen saver synchronization
	if (updateScreenSaverSync) {
		if (oldScreenSaverSync) {
			getScreen()->closeScreensaver();
		}
		else {
			getScreen()->openScreensaver(false);
		}
	}
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
