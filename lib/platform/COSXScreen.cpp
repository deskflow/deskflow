/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#include "COSXScreen.h"
#include "COSXClipboard.h"
#include "COSXEventQueueBuffer.h"
#include "COSXKeyState.h"
#include "COSXScreenSaver.h"
#include "CClipboard.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"

//
// COSXScreen
//

COSXScreen::COSXScreen(bool isPrimary) :
	m_isPrimary(isPrimary),
	m_isOnScreen(m_isPrimary),
	m_cursorPosValid(false),
	m_cursorHidden(false),
	m_keyState(NULL),
	m_sequenceNumber(0),
	m_screensaver(NULL),
	m_screensaverNotify(false),
	m_ownClipboard(false)
{
	try {
		m_displayID = CGMainDisplayID();
		updateScreenShape();
		m_screensaver = new COSXScreenSaver();
		m_keyState    = new COSXKeyState();
		LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d", m_x, m_y, m_w, m_h));
	}
	catch (...) {
		delete m_keyState;
		delete m_screensaver;
		throw;
	}

	// install event handlers
	EVENTQUEUE->adoptHandler(CEvent::kSystem, IEventQueue::getSystemTarget(),
							new TMethodEventJob<COSXScreen>(this,
								&COSXScreen::handleSystemEvent));

	// install the platform event queue
	EVENTQUEUE->adoptBuffer(new COSXEventQueueBuffer);
}

COSXScreen::~COSXScreen()
{
	disable();
	EVENTQUEUE->adoptBuffer(NULL);
	EVENTQUEUE->removeHandler(CEvent::kSystem, IEventQueue::getSystemTarget());
	delete m_keyState;
	delete m_screensaver;
}

void*
COSXScreen::getEventTarget() const
{
	return const_cast<COSXScreen*>(this);
}

bool
COSXScreen::getClipboard(ClipboardID, IClipboard* dst) const
{
	COSXClipboard src;
	CClipboard::copy(dst, &src);
	return true;
}

void
COSXScreen::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

void
COSXScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	Point mouse;
	GetGlobalMouse(&mouse);
	x                = mouse.h;
	y                = mouse.v;
	m_cursorPosValid = true;
	m_xCursor        = x;
	m_yCursor        = y;
}

void
COSXScreen::reconfigure(UInt32 activeSides)
{
	// FIXME
	(void)activeSides;
}

void
COSXScreen::warpCursor(SInt32 x, SInt32 y)
{
	// move cursor without generating events
	CGPoint pos;
	pos.x = x;
	pos.y = y;
	CGWarpMouseCursorPosition(pos);

	// save new cursor position
	m_xCursor        = x;
	m_yCursor        = y;
	m_cursorPosValid = true;
}

SInt32
COSXScreen::getJumpZoneSize() const
{
	// FIXME -- is this correct?
	return 1;
}

bool
COSXScreen::isAnyMouseButtonDown() const
{
	// FIXME
	return false;
}

void
COSXScreen::getCursorCenter(SInt32& x, SInt32& y) const
{
	x = m_xCenter;
	y = m_yCenter;
}

void
COSXScreen::postMouseEvent(const CGPoint & pos) const
{
	// synthesize event.  CGPostMouseEvent is a particularly good
	// example of a bad API.  we have to shadow the mouse state to
	// use this API and if we want to support more buttons we have
	// to recompile.
	//
	// the order of buttons on the mac is:
	// 1 - Left
	// 2 - Right
	// 3 - Middle
	// Whatever the USB device defined.
	// It is a bit weird that the behaviour of buttons over 3 are dependent
	// on currently plugged in USB devices.
	CGPostMouseEvent(pos, true, sizeof(m_buttons) / sizeof(m_buttons[0]),
				m_buttons[0],
				m_buttons[2],
				m_buttons[1],
				m_buttons[3],
				m_buttons[4]);
}


void
COSXScreen::fakeMouseButton(ButtonID id, bool press) const
{
	// get button index
	UInt32 index = id - kButtonLeft;
	if (index >= sizeof(m_buttons) / sizeof(m_buttons[0])) {
		return;
	}

	// update state
	m_buttons[index] = press;

	CGPoint pos;
	if (!m_cursorPosValid) {
		SInt32 x, y;
		getCursorPos(x, y);
	}
	pos.x = m_xCursor;
	pos.y = m_yCursor;
	postMouseEvent(pos);
}

void
COSXScreen::fakeMouseMove(SInt32 x, SInt32 y) const
{
	// synthesize event
	CGPoint pos;
	pos.x = x;
	pos.y = y;
	postMouseEvent(pos);

	// save new cursor position
	m_xCursor        = x;
	m_yCursor        = y;
	m_cursorPosValid = true;
}

void
COSXScreen::fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const
{
	// OS X does not appear to have a fake relative mouse move function.
	// simulate it by getting the current mouse position and adding to
	// that.  this can yield the wrong answer but there's not much else
	// we can do.

	// get current position
	Point oldPos;
	GetGlobalMouse(&oldPos);

	// synthesize event
	CGPoint pos;
	pos.x = oldPos.h + dx;
	pos.y = oldPos.v + dy;
	postMouseEvent(pos);

	// we now assume we don't know the current cursor position
	m_cursorPosValid = false;
}

void
COSXScreen::fakeMouseWheel(SInt32 delta) const
{
	CFPropertyListRef pref = ::CFPreferencesCopyValue(
							CFSTR("com.apple.scrollwheel.scaling"),
							kCFPreferencesAnyApplication,
							kCFPreferencesCurrentUser,
							kCFPreferencesAnyHost);

	int32_t wheelIncr = 10;

	if (pref != NULL) {
		CFTypeID id = CFGetTypeID(pref);
		if (id == CFNumberGetTypeID()) {
			CFNumberRef value = static_cast<CFNumberRef>(pref);

			double scaling;
			if (CFNumberGetValue(value, kCFNumberDoubleType, &scaling)) {
				wheelIncr = (int32_t)(8 * scaling);
			}
		}
		CFRelease(pref);
	}

	if (delta < 0) {
		wheelIncr = -wheelIncr;
	}

	CGPostScrollWheelEvent(1, wheelIncr);
}

void
COSXScreen::enable()
{
	// FIXME -- install clipboard snooper (if we need one)

	if (m_isPrimary) {
		// FIXME -- start watching jump zones
	}
	else {
		// FIXME -- prevent system from entering power save mode

		// hide cursor
		if (!m_cursorHidden) {
			CGDisplayHideCursor(m_displayID);
			m_cursorHidden = true;
		}

		// warp the mouse to the cursor center
		fakeMouseMove(m_xCenter, m_yCenter);

		// FIXME -- prepare to show cursor if it moves
	}

	updateKeys();
}

void
COSXScreen::disable()
{
	if (m_isPrimary) {
		// FIXME -- stop watching jump zones, stop capturing input
	}
	else {
		// show cursor
		if (m_cursorHidden) {
			CGDisplayShowCursor(m_displayID);
			m_cursorHidden = false;
		}

		// FIXME -- allow system to enter power saving mode
	}

	// FIXME -- uninstall clipboard snooper (if we needed one)

	m_isOnScreen = m_isPrimary;
}

void
COSXScreen::enter()
{
	if (m_isPrimary) {
		// FIXME -- stop capturing input, watch jump zones
	}
	else {
		// show cursor
		if (m_cursorHidden) {
			CGDisplayShowCursor(m_displayID);
			m_cursorHidden = false;
		}

		// reset buttons
		for (UInt32 i = 0; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
			m_buttons[i] = false;
		}
	}

	// now on screen
	m_isOnScreen = true;
}

bool
COSXScreen::leave()
{
	// FIXME -- choose keyboard layout if per-process and activate it here

	if (m_isPrimary) {
		// update key and button state
		updateKeys();

		// warp to center
		warpCursor(m_xCenter, m_yCenter);

		// capture events
		// FIXME
	}
	else {
		// hide cursor
		if (!m_cursorHidden) {
			CGDisplayHideCursor(m_displayID);
			m_cursorHidden = true;
		}

		// warp the mouse to the cursor center
		fakeMouseMove(m_xCenter, m_yCenter);

		// FIXME -- prepare to show cursor if it moves

		// take keyboard focus	
		// FIXME
	}

	// now off screen
	m_isOnScreen = false;

	return true;
}

bool
COSXScreen::setClipboard(ClipboardID, const IClipboard* src)
{
	COSXClipboard dst;
	m_ownClipboard = true;
	if (src != NULL) {
		// save clipboard data
		return CClipboard::copy(&dst, src);
	}
	else {
		// assert clipboard ownership
		if (!dst.open(0)) {
			return false;
		}
		dst.empty();
		dst.close();
		return true;
	}
}

void
COSXScreen::checkClipboards()
{
	if (m_ownClipboard && !COSXClipboard::isOwnedBySynergy()) {
		static ScrapRef sScrapbook = NULL;
		ScrapRef currentScrap;
		GetCurrentScrap(&currentScrap);

		if (sScrapbook != currentScrap) {
			m_ownClipboard = false;
			sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardClipboard);
			sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardSelection);
			sScrapbook = currentScrap;
		}
	}
}

void
COSXScreen::openScreensaver(bool notify)
{
	m_screensaverNotify = notify;
	if (!m_screensaverNotify) {
		m_screensaver->disable();
	}
}

void
COSXScreen::closeScreensaver()
{
	if (!m_screensaverNotify) {
		m_screensaver->enable();
	}
}

void
COSXScreen::screensaver(bool activate)
{
	if (activate) {
		m_screensaver->activate();
	}
	else {
		m_screensaver->deactivate();
	}
}

void
COSXScreen::resetOptions()
{
	// no options
}

void
COSXScreen::setOptions(const COptionsList&)
{
	// no options
}

void
COSXScreen::setSequenceNumber(UInt32 seqNum)
{
	m_sequenceNumber = seqNum;
}

bool
COSXScreen::isPrimary() const
{
	return m_isPrimary;
}

void
COSXScreen::sendEvent(CEvent::Type type, void* data)
{
	EVENTQUEUE->addEvent(CEvent(type, getEventTarget(), data));
}

void
COSXScreen::sendClipboardEvent(CEvent::Type type, ClipboardID id)
{
	CClipboardInfo* info   = (CClipboardInfo*)malloc(sizeof(CClipboardInfo));
	info->m_id             = id;
	info->m_sequenceNumber = m_sequenceNumber;
	sendEvent(type, info);
}


void
COSXScreen::handleSystemEvent(const CEvent& event, void*)
{
/*
	EventRef * carbonEvent = reinterpret_cast<EventRef *>(event.getData());
	assert(carbonEvent != NULL);

	UInt32 eventClass = GetEventClass( *carbonEvent );
	
	switch( eventClass )  
	{
		case kEventClassMouse:
		{
			UInt32 eventKind = GetEventKind( *carbonEvent );
			switch( eventKind )
			{
				case kEventMouseMoved:
				{
					HIPoint point;
					GetEventParameter( *carbonEvent,
						kEventParamMouseDelta,
						typeHIPoint,
						NULL,
						sizeof(point),
						NULL,
						&point);
						
				} break;
			}
		}break;

		case kEventClassKeyboard: 
		{
		
		}

		default: 
		{
		
		} break;
	
	}
*/
}

void
COSXScreen::updateButtons()
{
	// FIXME -- get current button state into m_buttons[]
}

IKeyState*
COSXScreen::getKeyState() const
{
	return m_keyState;
}

void
COSXScreen::updateScreenShape()
{
	// get info for each display
	CGDisplayCount displayCount = 0;

	if (CGGetActiveDisplayList(0, NULL, &displayCount) != CGDisplayNoErr) {
		return;
	}
	
	if (displayCount == 0) {
		return;
	}

	CGDirectDisplayID* displays = 
		(CGDirectDisplayID*)malloc(displayCount * sizeof(CGDirectDisplayID));

	if (displays == NULL) {
		return;
	}

	if (CGGetActiveDisplayList(displayCount,
							displays, &displayCount) != CGDisplayNoErr) {
		free(displays);
		return;
	}

	// get smallest rect enclosing all display rects
	CGRect totalBounds = CGRectZero;
	for (CGDisplayCount i = 0; i < displayCount; ++i) {
		CGRect bounds = CGDisplayBounds(displays[i]);
		totalBounds   = CGRectUnion(totalBounds, bounds);
	}

	// get shape of default screen
	m_x = (SInt32)totalBounds.origin.x;
	m_y = (SInt32)totalBounds.origin.y;
	m_w = (SInt32)totalBounds.size.width;
	m_h = (SInt32)totalBounds.size.height;

	// get center of default screen
	// XXX -- this should compute the center of displays[0]
	m_xCenter = m_x + (m_w >> 1);
	m_yCenter = m_y + (m_h >> 1);

	LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d on %u %s", m_x, m_y, m_w, m_h, displayCount, (displayCount == 1) ? "display" : "displays"));

	free(displays);
}
