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
	{
		CLock lock(&m_mutex);
		updateKeys();
	}

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

	sync();

	// now active
	m_active = true;

	// subclass hook
	onPreEnter();

	// update our keyboard state to reflect the local state
	updateKeys();

	// toggle modifiers that don't match the desired state and
	// remember previous toggle key state.
	m_toggleKeys = m_mask;
	setToggleState(mask);

	// warp to requested location
	fakeMouseMove(x, y);

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

	sync();

	// subclass hook
	onPreLeave();

	// restore toggle key state
	setToggleState(m_toggleKeys);

	// hide mouse
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

CSecondaryScreen::SysKeyID
CSecondaryScreen::getUnhanded(SysKeyID) const
{
	// no key represents both left and right sides of any key
	return 0;
}

CSecondaryScreen::SysKeyID
CSecondaryScreen::getOtherHanded(SysKeyID) const
{
	// no key represents both left and right sides of any key
	return 0;
}

bool
CSecondaryScreen::synthesizeCtrlAltDel(EKeyAction)
{
	// pass keys through unchanged
	return false;
}

void
CSecondaryScreen::sync() const
{
	// do nothing
}

void
CSecondaryScreen::flush()
{
	// do nothing
}

void
CSecondaryScreen::doKeystrokes(const Keystrokes& keys, SInt32 count)
{
	// do nothing if no keys or no repeats
	if (count < 1 || keys.empty()) {
		return;
	}

	// generate key events
	LOG((CLOG_DEBUG2 "keystrokes:"));
	for (Keystrokes::const_iterator k = keys.begin(); k != keys.end(); ) {
		if (k->m_repeat) {
			// repeat from here up to but not including the next key
			// with m_repeat == false count times.
			Keystrokes::const_iterator start = k;
			for (; count > 0; --count) {
				// send repeating events
				for (k = start; k != keys.end() && k->m_repeat; ++k) {
					LOG((CLOG_DEBUG2 "  %d %s repeat", k->m_sysKeyID, k->m_press ? "down" : "up"));
					fakeKeyEvent(k->m_sysKeyID, k->m_press);
				}
			}

			// note -- k is now on the first non-repeat key after the
			// repeat keys, exactly where we'd like to continue from.
		}
		else {
			// send event
			LOG((CLOG_DEBUG2 "  %d %s", k->m_sysKeyID, k->m_press ? "down" : "up"));
			fakeKeyEvent(k->m_sysKeyID, k->m_press);

			// next key
			++k;
		}
	}

	flush();
}

void
CSecondaryScreen::keyDown(KeyID key,
				KeyModifierMask mask, KeyButton button)
{
	CLock lock(&m_mutex);
	sync();

	// check for ctrl+alt+del emulation
	if (key == kKeyDelete &&
		(mask & (KeyModifierControl | KeyModifierAlt)) ==
				(KeyModifierControl | KeyModifierAlt)) {
		LOG((CLOG_DEBUG "emulating ctrl+alt+del press"));
		if (synthesizeCtrlAltDel(kPress)) {
			return;
		}
	}

	// get the sequence of keys to simulate key press and the final
	// modifier state.
	Keystrokes keys;
	SysKeyID sysKeyID;
	m_mask = mapKey(keys, sysKeyID, key, m_mask, mask, kPress);
	if (keys.empty()) {
		// do nothing if there are no associated keys (i.e. lookup failed)
		return;
	}
	sysKeyID &= 0xffu;

	// generate key events
	doKeystrokes(keys, 1);

	// do not record button down if button or system key is 0 (invalid)
	if (button != 0 && sysKeyID != 0) {
		// note that key is now down
		SysKeyID unhandedSysKeyID = getUnhanded(sysKeyID);
		m_serverKeyMap[button]    = sysKeyID;
		m_keys[sysKeyID]         |= kDown;
		m_fakeKeys[sysKeyID]     |= kDown;
		if (unhandedSysKeyID != 0) {
			m_keys[unhandedSysKeyID]     |= kDown;
			m_fakeKeys[unhandedSysKeyID] |= kDown;
		}
	}
}


void
CSecondaryScreen::keyRepeat(KeyID key,
				KeyModifierMask mask, SInt32 count, KeyButton button)
{
	CLock lock(&m_mutex);
	sync();

	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}

	// get the sequence of keys to simulate key repeat and the final
	// modifier state.
	Keystrokes keys;
	SysKeyID sysKeyID;
	m_mask = mapKey(keys, sysKeyID, key, m_mask, mask, kRepeat);
	if (keys.empty()) {
		return;
	}
	sysKeyID &= 0xffu;

	// if this key shouldn't auto-repeat then ignore
	if (!isAutoRepeating(sysKeyID)) {
		return;
	}

	// if the keycode for the auto-repeat is not the same as for the
	// initial press then mark the initial key as released and the new
	// key as pressed.  this can happen when we auto-repeat after a
	// dead key.  for example, a dead accent followed by 'a' will
	// generate an 'a with accent' followed by a repeating 'a'.  the
	// keycodes for the two keysyms might be different.
	if (sysKeyID != index->second) {
		// replace key up with previous key id but leave key down
		// alone so it uses the new keycode and store that keycode
		// in the server key map.
		for (Keystrokes::iterator index2 = keys.begin();
								index2 != keys.end(); ++index2) {
			if ((index2->m_sysKeyID & 0xffu) == sysKeyID) {
				index2->m_sysKeyID = index->second;
				break;
			}
		}

		// note that old key is now up
		m_keys[index->second]     &= ~kDown;
		m_fakeKeys[index->second] &= ~kDown;

		// map server key to new key
		index->second              = sysKeyID;

		// note that new key is now down
		m_keys[index->second]     |= kDown;
		m_fakeKeys[index->second] |= kDown;
	}

	// generate key events
	doKeystrokes(keys, count);
}

void
CSecondaryScreen::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
	CLock lock(&m_mutex);
	sync();

	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}
	SysKeyID sysKeyID = index->second;

	// check for ctrl+alt+del emulation
	if (key == kKeyDelete &&
		(mask & (KeyModifierControl | KeyModifierAlt)) ==
				(KeyModifierControl | KeyModifierAlt)) {
		LOG((CLOG_DEBUG "emulating ctrl+alt+del release"));
		if (synthesizeCtrlAltDel(kRelease)) {
			return;
		}
	}

	// get the sequence of keys to simulate key release
	Keystrokes keys;
	Keystroke keystroke;
	keystroke.m_sysKeyID = sysKeyID;
	keystroke.m_press    = false;
	keystroke.m_repeat   = false;
	keys.push_back(keystroke);

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now up
	SysKeyID unhandedSysKeyID     = getUnhanded(sysKeyID);
	m_serverKeyMap.erase(index);
	m_keys[sysKeyID]             &= ~kDown;
	m_fakeKeys[sysKeyID]         &= ~kDown;
	if (unhandedSysKeyID != 0) {
		SysKeyID otherHandedSysKeyID      = getOtherHanded(sysKeyID);
		if ((m_keys[otherHandedSysKeyID] & kDown) == 0) {
			m_keys[unhandedSysKeyID]     &= ~kDown;
			m_fakeKeys[unhandedSysKeyID] &= ~kDown;
		}
	}

	// get the new modifier state
	mask = getModifierKeyMask(sysKeyID);
	if (mask != 0) {
		// key is a modifier key
		if ((mask & (KeyModifierCapsLock |
					KeyModifierNumLock |
					KeyModifierScrollLock)) != 0) {
			// modifier is a toggle
			m_mask ^= mask;
		}
		else if (!isModifierActive(sysKeyID)) {
			// all keys for this modifier are released
			m_mask &= ~mask;
		}
	}
}

void
CSecondaryScreen::mouseDown(ButtonID button)
{
	CLock lock(&m_mutex);
	sync();
	fakeMouseButton(button, true);
	flush();
}

void
CSecondaryScreen::mouseUp(ButtonID button)
{
	CLock lock(&m_mutex);
	sync();
	fakeMouseButton(button, false);
	flush();
}

void
CSecondaryScreen::mouseMove(SInt32 x, SInt32 y)
{
	CLock lock(&m_mutex);
	sync();
	fakeMouseMove(x, y);
	flush();
}

void
CSecondaryScreen::mouseWheel(SInt32 delta)
{
	CLock lock(&m_mutex);
	sync();
	fakeMouseWheel(delta);
	flush();
}

void
CSecondaryScreen::setToggleState(KeyModifierMask mask)
{
	// toggle modifiers that don't match the desired state
	KeyModifierMask different = (m_mask ^ mask);
	if ((different & KeyModifierCapsLock)   != 0) {
		toggleKey(kKeyCapsLock, KeyModifierCapsLock);
	}
	if ((different & KeyModifierNumLock)    != 0) {
		toggleKey(kKeyNumLock, KeyModifierNumLock);
	}
	if ((different & KeyModifierScrollLock) != 0) {
		toggleKey(kKeyScrollLock, KeyModifierScrollLock);
	}
}

void
CSecondaryScreen::resetOptions()
{
	// set screen saver synchronization flag and see if we need to
	// update the screen saver synchronization.  reset other options.
	bool screenSaverSyncOn;
	{
		CLock lock(&m_mutex);
		screenSaverSyncOn    = (!m_screenSaverSync && m_remoteReady);
		m_screenSaverSync    = true;
		m_numLockHalfDuplex  = false;
		m_capsLockHalfDuplex = false;
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
			else if (options[i] == kOptionHalfDuplexCapsLock) {
				m_capsLockHalfDuplex = (options[i + 1] != 0);
				LOG((CLOG_DEBUG1 "half-duplex caps-lock %s", m_capsLockHalfDuplex ? "on" : "off"));
			}
			else if (options[i] == kOptionHalfDuplexNumLock) {
				m_numLockHalfDuplex = (options[i + 1] != 0);
				LOG((CLOG_DEBUG1 "half-duplex num-lock %s", m_numLockHalfDuplex ? "on" : "off"));
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

SInt32
CSecondaryScreen::getJumpZoneSize() const
{
	return 0;
}

void
CSecondaryScreen::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	sync();
	getScreen()->getShape(x, y, w, h);
}

void
CSecondaryScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	sync();
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

void
CSecondaryScreen::updateKeys()
{
	sync();

	// clear key state
	memset(m_keys,     0, sizeof(m_keys));
	memset(m_fakeKeys, 0, sizeof(m_fakeKeys));

	// let subclass set m_keys
	updateKeys(m_keys);

	// get m_mask from subclass
	m_mask = getModifiers();
	LOG((CLOG_DEBUG2 "modifiers on update: 0x%04x", m_mask));
}

void
CSecondaryScreen::releaseKeys()
{
	CLock lock(&m_mutex);
	sync();

	// release keys that we've synthesized a press for and only those
	// keys.  we don't want to synthesize a release on a key the user
	// is still physically pressing.
	for (UInt32 i = 1; i < 256; ++i) {
		if ((m_fakeKeys[i] & kDown) != 0) {
			fakeKeyEvent(i, false);
			m_keys[i]     &= ~kDown;
			m_fakeKeys[i] &= ~kDown;
		}
	}

	flush();
}

void
CSecondaryScreen::toggleKey(KeyID keyID, KeyModifierMask mask)
{
	// get the system key ID for this toggle key ID
	SysKeyID sysKeyID = getToggleSysKey(keyID);
	if (sysKeyID == 0) {
		return;
	}

	// toggle the key
	if (isKeyHalfDuplex(keyID)) {
		// "half-duplex" toggle
		fakeKeyEvent(sysKeyID, (m_mask & mask) == 0);
	}
	else {
		// normal toggle
		fakeKeyEvent(sysKeyID, true);
		fakeKeyEvent(sysKeyID, false);
	}
	flush();

	// toggle shadow state
	m_mask               ^= mask;
	sysKeyID             &= 0xffu;
	m_keys[sysKeyID]     ^= kToggled;
	m_fakeKeys[sysKeyID] ^= kToggled;
}

bool
CSecondaryScreen::isKeyDown(SysKeyID sysKeyID) const
{
	sysKeyID &= 0xffu;
	return (sysKeyID != 0 && ((m_keys[sysKeyID] & kDown) != 0));
}

bool
CSecondaryScreen::isKeyToggled(SysKeyID sysKeyID) const
{
	sysKeyID &= 0xffu;
	return (sysKeyID != 0 && ((m_keys[sysKeyID] & kToggled) != 0));
}

bool
CSecondaryScreen::isKeyHalfDuplex(KeyID keyID) const
{
	return ((keyID == kKeyCapsLock && m_capsLockHalfDuplex) ||
			(keyID == kKeyNumLock  && m_numLockHalfDuplex));
}
