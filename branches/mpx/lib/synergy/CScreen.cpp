/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CScreen.h"
#include "IPlatformScreen.h"
#include "ProtocolTypes.h"
#include "CLog.h"
#include "IEventQueue.h"

//
// CScreen
//

CScreen::CScreen(IPlatformScreen* platformScreen) :
	m_screen(platformScreen),
	m_dev(CDeviceManager::getInstance()),
	m_isPrimary(platformScreen->isPrimary()),
	m_enabled(false),
	m_screenSaverSync(true),
	m_fakeInput(false)
{
	assert(m_screen != NULL);

	// reset options
	resetOptions();

	LOG((CLOG_DEBUG "opened display"));
}

CScreen::~CScreen()
{
	if (m_enabled) {
		disable();
	}
	assert(!m_enabled);
	//assert(m_entered == m_isPrimary);
	delete m_screen;
	LOG((CLOG_DEBUG "closed display"));
}

void
CScreen::enable()
{
	assert(!m_enabled);
	std::list<UInt8> ptrIDs;
	std::list<UInt8>::iterator i;
	m_dev->getAllPointerIDs(ptrIDs);
	for(i=ptrIDs.begin(); i != ptrIDs.end(); ++i)
	{
	    m_screen->updateKeyMap(*i);
	    m_screen->updateKeyState(*i);
	    m_screen->enable();
	    if (m_isPrimary) {
		    enablePrimary();
	    }
	    else {
		    enableSecondary();
	    }
	}

	// note activation
	m_enabled = true;
}

void
CScreen::disable()
{
	assert(m_enabled);
	std::list<UInt8> ptrIDs;
	std::list<UInt8>::iterator i;
	m_dev->getAllPointerIDs(ptrIDs);
	for(i=ptrIDs.begin(); i != ptrIDs.end(); ++i)
	{
	    if (!m_isPrimary && m_dev->hasEntered(*i)) {	 
		leave(*i);
	    }
	    else if (m_isPrimary && !m_dev->hasEntered(*i)) {
		enter(0, m_dev->getAttachment(*i),*i);
	    }
	    m_screen->disable();
	    if (m_isPrimary) {
		disablePrimary();
	    }
	    else {
		disableSecondary();
	    }
	}
	  // note deactivation
	  m_enabled = false;
	  
}

void
CScreen::enter(KeyModifierMask toggleMask, UInt8 kId, UInt8 pId)
{
  // FIXXME CScreen::enter assert
	//assert(m_entered == false);
	LOG((CLOG_INFO "entering screen"));
	
	m_screen->enter(kId, pId);
	if (m_isPrimary) {
		enterPrimary();
	}
	else {
		enterSecondary(toggleMask);
	}
	
	// now on screen
	if(!m_isPrimary)
	  m_dev->setEntered(true, m_dev->getIdFromSid(pId));
}

bool
CScreen::leave(UInt8 id)
{
//	assert(m_entered == true);
	LOG((CLOG_INFO "leaving screen"));

	if (!m_screen->leave(id)) {
		return false;
	}
	if (m_isPrimary) {
		leavePrimary(id);
	}
	else {
		leaveSecondary(id);
	}

	// make sure our idea of clipboard ownership is correct
	m_screen->checkClipboards();

	return true;
}

void
CScreen::reconfigure(UInt32 activeSides)
{
	assert(m_isPrimary);
	m_screen->reconfigure(activeSides);
}

void
CScreen::warpCursor(SInt32 x, SInt32 y, UInt8 id)
{
	assert(m_isPrimary);
	m_screen->warpCursor(x, y, id);
}

void
CScreen::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
	m_screen->setClipboard(id, clipboard);
}

void
CScreen::grabClipboard(ClipboardID id)
{
	m_screen->setClipboard(id, NULL);
}

void
CScreen::screensaver(bool activate)
{
	if (!m_isPrimary) {
		// activate/deactivation screen saver iff synchronization enabled
		if (m_screenSaverSync) {
			m_screen->screensaver(activate);
		}
	}
}

void
CScreen::keyDown(KeyID kId, KeyModifierMask mask, KeyButton button, UInt8 id)
{
	assert(!m_isPrimary || m_fakeInput);

	// check for ctrl+alt+del emulation
	if (kId == kKeyDelete &&
		(mask & (KeyModifierControl | KeyModifierAlt)) ==
				(KeyModifierControl | KeyModifierAlt)) {
		LOG((CLOG_DEBUG "emulating ctrl+alt+del press"));
		if (m_screen->fakeCtrlAltDel(id)) {
			return;
		}
	}
	m_screen->fakeKeyDown(kId, mask, button, id);
}

void
CScreen::keyRepeat(KeyID kId,
				KeyModifierMask mask, SInt32 count, KeyButton button, UInt8 id)
{
	assert(!m_isPrimary);
	m_screen->fakeKeyRepeat(kId, mask, count, button, id);
}

void
CScreen::keyUp(KeyID, KeyModifierMask, KeyButton button, UInt8 id)
{
	assert(!m_isPrimary || m_fakeInput);
	m_screen->fakeKeyUp(button, id);
}

void
CScreen::mouseDown(ButtonID button, UInt8 id)
{
	assert(!m_isPrimary);
	m_screen->fakeButtonEvent(button, true, id);
}

void
CScreen::mouseUp(ButtonID button, UInt8 id)
{
	assert(!m_isPrimary);
	m_screen->fakeButtonEvent(button, false, id);
}

void
CScreen::mouseMove(SInt32 x, SInt32 y, UInt8 id)
{
	assert(!m_isPrimary);
	m_screen->fakeMotionEvent(x, y, id);
}

void
CScreen::mouseRelativeMove(SInt32 dx, SInt32 dy, UInt8 id)
{
	assert(!m_isPrimary);
	m_screen->fakeRelativeMotionEvent(dx, dy, id);
}

void
CScreen::mouseWheel(SInt32 xDelta, SInt32 yDelta, UInt8 id)
{
	assert(!m_isPrimary);
	m_screen->fakeMouseWheelEvent(xDelta, yDelta, id);
}

void
CScreen::resetOptions()
{
	// reset options
	m_halfDuplex = 0;

	// if screen saver synchronization was off then turn it on since
	// that's the default option state.
	if (!m_screenSaverSync) {
		m_screenSaverSync = true;
		if (!m_isPrimary) {
			m_screen->openScreensaver(false);
		}
	}

	// let screen handle its own options
	m_screen->resetOptions();
}

void
CScreen::setOptions(const COptionsList& options)
{
	// update options
	std::list<UInt8> ptrIDs;
	std::list<UInt8>::iterator j;
	m_dev->getAllPointerIDs(ptrIDs);
	for(j=ptrIDs.begin(); j != ptrIDs.end(); ++j)
	{
	bool oldScreenSaverSync = m_screenSaverSync;
	for (UInt32 i = 0, n = (UInt32)options.size(); i < n; i += 2) {
		if (options[i] == kOptionScreenSaverSync) {
			m_screenSaverSync = (options[i + 1] != 0);
			LOG((CLOG_DEBUG1 "screen saver synchronization %s", m_screenSaverSync ? "on" : "off"));
		}
		else if (options[i] == kOptionHalfDuplexCapsLock) {
			if (options[i + 1] != 0) {
				m_halfDuplex |=  KeyModifierCapsLock;
			}
			else {
				m_halfDuplex &= ~KeyModifierCapsLock;
			}
			LOG((CLOG_DEBUG1 "half-duplex caps-lock %s", ((m_halfDuplex & KeyModifierCapsLock) != 0) ? "on" : "off"));
		}
		else if (options[i] == kOptionHalfDuplexNumLock) {
			if (options[i + 1] != 0) {
				m_halfDuplex |=  KeyModifierNumLock;
			}
			else {
				m_halfDuplex &= ~KeyModifierNumLock;
			}
			LOG((CLOG_DEBUG1 "half-duplex num-lock %s", ((m_halfDuplex & KeyModifierNumLock) != 0) ? "on" : "off"));
		}
		else if (options[i] == kOptionHalfDuplexScrollLock) {
			if (options[i + 1] != 0) {
				m_halfDuplex |=  KeyModifierScrollLock;
			}
			else {
				m_halfDuplex &= ~KeyModifierScrollLock;
			}
			LOG((CLOG_DEBUG1 "half-duplex scroll-lock %s", ((m_halfDuplex & KeyModifierScrollLock) != 0) ? "on" : "off"));
		}
	}

	// update half-duplex options
	m_screen->setHalfDuplexMask(m_halfDuplex, *j);
	
	// update screen saver synchronization
	if (!m_isPrimary && oldScreenSaverSync != m_screenSaverSync) {
		if (m_screenSaverSync) {
			m_screen->openScreensaver(false);
		}
		else {
			m_screen->closeScreensaver();
		}
	}

	// let screen handle its own options
	m_screen->setOptions(options);
	} // iterator over devices

}

void
CScreen::setSequenceNumber(UInt32 seqNum)
{
	m_screen->setSequenceNumber(seqNum);
}

UInt32
CScreen::registerHotKey(KeyID key, KeyModifierMask mask, UInt8 id)
{
	return m_screen->registerHotKey(key, mask, id);
}

void
CScreen::unregisterHotKey(UInt32 id)
{
	m_screen->unregisterHotKey(id);
}

void
CScreen::fakeInputBegin()
{
	assert(!m_fakeInput);

	m_fakeInput = true;
	m_screen->fakeInputBegin();
}

void
CScreen::fakeInputEnd()
{
	assert(m_fakeInput);

	m_fakeInput = false;
	m_screen->fakeInputEnd();
}

bool
CScreen::isOnScreen(UInt8 id) const
{
	return m_dev->hasEntered(id);
}

bool
CScreen::isLockedToScreen(UInt8 id) const
{
	// check for pressed mouse buttons
	if (m_screen->isAnyMouseButtonDown(id)) {
		LOG((CLOG_DEBUG "locked by mouse button"));
		return true;
	}

	// not locked
	return false;
}

SInt32
CScreen::getJumpZoneSize() const
{
	if (!m_isPrimary) {
		return 0;
	}
	else {
		return m_screen->getJumpZoneSize();
	}
}

void
CScreen::getCursorCenter(SInt32& x, SInt32& y) const
{
	m_screen->getCursorCenter(x, y);
}

KeyModifierMask
CScreen::getActiveModifiers(UInt8 id) const
{
	return m_screen->getActiveModifiers(id);
}

KeyModifierMask
CScreen::pollActiveModifiers(UInt8 id) const
{
	return m_screen->pollActiveModifiers(id);
}

void*
CScreen::getEventTarget() const
{
	return m_screen;
}

bool
CScreen::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
	return m_screen->getClipboard(id, clipboard);
}

void
CScreen::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	m_screen->getShape(x, y, w, h);
}

void
CScreen::getCursorPos(SInt32& x, SInt32& y, UInt8 id) const
{
	m_screen->getCursorPos(x, y, id);
}

void
CScreen::enablePrimary()
{
	// get notified of screen saver activation/deactivation
	m_screen->openScreensaver(true);

	// claim screen changed size
	EVENTQUEUE->addEvent(CEvent(getShapeChangedEvent(), getEventTarget()));
}

void
CScreen::enableSecondary()
{
	// assume primary has all clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		grabClipboard(id);
	}

	// disable the screen saver if synchronization is enabled
	if (m_screenSaverSync) {
		m_screen->openScreensaver(false);
	}
}

void
CScreen::disablePrimary()
{
	// done with screen saver
	m_screen->closeScreensaver();
}

void
CScreen::disableSecondary()
{
	// done with screen saver
	m_screen->closeScreensaver();
}

void
CScreen::enterPrimary()
{
	// do nothing
}

void
CScreen::enterSecondary(KeyModifierMask)
{
	// do nothing
}

void
CScreen::leavePrimary(UInt8 id)
{
	// we don't track keys while on the primary screen so update our
	// idea of them now.  this is particularly to update the state of
	// the toggle modifiers.
	m_screen->updateKeyState(id);
}

void
CScreen::leaveSecondary(UInt8 id)
{
    // release any keys we think are still down
	m_screen->fakeAllKeysUp(m_dev->getAttachment(id));
		// now not on screen
	m_dev->setEntered(false, id);
	
	m_dev->removeDevice(m_dev->getAttachment(id));
	m_dev->removeDevice(id);
	m_screen->cleanUp(id);
}
