/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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
	m_isPrimary(platformScreen->isPrimary()),
	m_enabled(false),
	m_entered(m_isPrimary),
	m_screenSaverSync(true),
	m_toggleKeys(0)
{
	assert(m_screen != NULL);

	// open screen
	m_screen->setKeyState(this);

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
	assert(m_entered == m_isPrimary);
	delete m_screen;
	LOG((CLOG_DEBUG "closed display"));
}

void
CScreen::enable()
{
	assert(!m_enabled);

	m_screen->enable();
	if (m_isPrimary) {
		enablePrimary();
	}
	else {
		enableSecondary();
	}

	// note activation
	m_enabled = true;
}

void
CScreen::disable()
{
	assert(m_enabled);

	if (!m_isPrimary && m_entered) {
		leave();
	}
	else if (m_isPrimary && !m_entered) {
		enter();
	}
	m_screen->disable();
	if (m_isPrimary) {
		disablePrimary();
	}
	else {
		disableSecondary();
	}

	// note deactivation
	m_enabled = false;
}

void
CScreen::enter()
{
	assert(m_entered == false);
	LOG((CLOG_INFO "entering screen"));

	// now on screen
	m_entered = true;

	if (m_isPrimary) {
		enterPrimary();
	}
	else {
		enterSecondary();
	}
	m_screen->enter();
}

bool
CScreen::leave()
{
	assert(m_entered == true);
	LOG((CLOG_INFO "leaving screen"));

	if (!m_screen->leave()) {
		return false;
	}
	if (m_isPrimary) {
		leavePrimary();
	}
	else {
		leaveSecondary();
	}

	// make sure our idea of clipboard ownership is correct
	m_screen->checkClipboards();

	// now not on screen
	m_entered = false;

	return true;
}

void
CScreen::reconfigure(UInt32 activeSides)
{
	assert(m_isPrimary);
	m_screen->reconfigure(activeSides);
}

void
CScreen::warpCursor(SInt32 x, SInt32 y)
{
	assert(m_isPrimary);
	m_screen->warpCursor(x, y);
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
CScreen::keyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
	assert(!m_isPrimary);

	// check for ctrl+alt+del emulation
	if (id == kKeyDelete &&
		(mask & (KeyModifierControl | KeyModifierAlt)) ==
				(KeyModifierControl | KeyModifierAlt)) {
		LOG((CLOG_DEBUG "emulating ctrl+alt+del press"));
		if (m_screen->fakeCtrlAltDel()) {
			return;
		}
	}

	// get the sequence of keys to simulate key press and the final
	// modifier state.
	Keystrokes keys;
	KeyButton key = m_screen->mapKey(keys, *this, id, mask, false);
	if (keys.empty()) {
		// do nothing if there are no associated keys
		LOG((CLOG_DEBUG2 "cannot map key 0x%08x", id));
		return;
	}

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is down
	updateKeyState(button, key, true);
}

void
CScreen::keyRepeat(KeyID id,
				KeyModifierMask mask, SInt32 count, KeyButton button)
{
	assert(!m_isPrimary);

	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}

	// get the sequence of keys to simulate key repeat and the final
	// modifier state.
	Keystrokes keys;
	KeyButton key = m_screen->mapKey(keys, *this, id, mask, true);
	if (key == 0) {
		LOG((CLOG_DEBUG2 "cannot map key 0x%08x", id));
		return;
	}
	if (keys.empty()) {
		// do nothing if there are no associated keys
		return;
	}

	// if the keycode for the auto-repeat is not the same as for the
	// initial press then mark the initial key as released and the new
	// key as pressed.  this can happen when we auto-repeat after a
	// dead key.  for example, a dead accent followed by 'a' will
	// generate an 'a with accent' followed by a repeating 'a'.  the
	// keycodes for the two keysyms might be different.
	key &= 0xffu;
	if (key != index->second) {
		// replace key up with previous key id but leave key down
		// alone so it uses the new keycode and store that keycode
		// in the server key map.
		for (Keystrokes::iterator index2 = keys.begin();
								index2 != keys.end(); ++index2) {
			if ((index2->m_key & 0xffu) == key) {
				index2->m_key = index->second;
				break;
			}
		}

		// note that old key is now up
		m_keys[index->second]     &= ~kDown;
		m_fakeKeys[index->second] &= ~kDown;

		// map server key to new key
		index->second              = key;

		// note that new key is now down
		m_keys[index->second]     |= kDown;
		m_fakeKeys[index->second] |= kDown;
	}

	// generate key events
	doKeystrokes(keys, count);
}

void
CScreen::keyUp(KeyID, KeyModifierMask, KeyButton button)
{
	assert(!m_isPrimary);

	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}
	KeyButton key = index->second;

	// get the sequence of keys to simulate key release
	Keystrokes keys;
	Keystroke keystroke;
	keystroke.m_key    = key;
	keystroke.m_press  = false;
	keystroke.m_repeat = false;
	keys.push_back(keystroke);

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now up
	updateKeyState(button, key, false);
}

void
CScreen::mouseDown(ButtonID button)
{
	assert(!m_isPrimary);
	m_screen->fakeMouseButton(button, true);
}

void
CScreen::mouseUp(ButtonID button)
{
	assert(!m_isPrimary);
	m_screen->fakeMouseButton(button, false);
}

void
CScreen::mouseMove(SInt32 x, SInt32 y)
{
	assert(!m_isPrimary);
	m_screen->fakeMouseMove(x, y);
}

void
CScreen::mouseWheel(SInt32 delta)
{
	assert(!m_isPrimary);
	m_screen->fakeMouseWheel(delta);
}

void
CScreen::resetOptions()
{
	// reset options
	m_numLockHalfDuplex  = false;
	m_capsLockHalfDuplex = false;

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
	bool oldScreenSaverSync = m_screenSaverSync;
	for (UInt32 i = 0, n = options.size(); i < n; i += 2) {
		if (options[i] == kOptionScreenSaverSync) {
			m_screenSaverSync = (options[i + 1] != 0);
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
}

void
CScreen::setSequenceNumber(UInt32 seqNum)
{
	m_screen->setSequenceNumber(seqNum);
}

bool
CScreen::isOnScreen() const
{
	return m_entered;
}

bool
CScreen::isLockedToScreen() const
{
	// check for pressed mouse buttons
	if (m_screen->isAnyMouseButtonDown()) {
		LOG((CLOG_DEBUG "locked by mouse button"));
		return true;
	}

	// we don't keep primary key state up to date so get the
	// current state.
	const_cast<CScreen*>(this)->updateKeys();

	// check for scroll lock toggled on
	if (isModifierActive(KeyModifierScrollLock)) {
		LOG((CLOG_DEBUG "locked by scroll lock"));
		return true;
	}

	// check for any pressed key
	KeyButton key = isAnyKeyDown();
	if (key != 0) {
		LOG((CLOG_DEBUG "locked by %s", m_screen->getKeyName(key)));
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
CScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	m_screen->getCursorPos(x, y);
}

void
CScreen::updateKeys()
{
	// clear key state
	memset(m_keys,     0, sizeof(m_keys));
	memset(m_fakeKeys, 0, sizeof(m_fakeKeys));
	m_maskToKeys.clear();
	m_keyToMask.clear();

	// let subclass set m_keys
	m_screen->updateKeys();

	// figure out active modifier mask
	m_mask = getModifierMask();
	LOG((CLOG_DEBUG2 "modifiers on update: 0x%04x", m_mask));
}

void
CScreen::releaseKeys()
{
	// release keys that we've synthesized a press for and only those
	// keys.  we don't want to synthesize a release on a key the user
	// is still physically pressing.
	for (KeyButton i = 1; i < 256; ++i) {
		if ((m_fakeKeys[i] & kDown) != 0) {
			fakeKeyEvent(i, false, false);
			m_keys[i]     &= ~kDown;
			m_fakeKeys[i] &= ~kDown;
		}
	}
}

void
CScreen::setKeyDown(KeyButton key)
{
	m_keys[key & 0xffu] |= kDown;
}

void
CScreen::setToggled(KeyModifierMask mask)
{
	if (!isToggle(mask)) {
		return;
	}
	MaskToKeys::const_iterator i = m_maskToKeys.find(mask);
	if (i == m_maskToKeys.end()) {
		return;
	}
	for (KeyButtons::const_iterator j = i->second.begin();
							j != i->second.end(); ++j) {
		m_keys[(*j) & 0xffu] |= kToggled;
	}
}

void
CScreen::addModifier(KeyModifierMask mask, KeyButtons& keys)
{
	// the modifier must have associated keys
	if (keys.empty()) {
		return;
	}

	// the mask must not be zero
	assert(mask != 0);

	// the mask must have exactly one high bit
	assert((mask & (mask - 1)) == 0);

	// index mask by keycodes
	for (KeyButtons::iterator j = keys.begin(); j != keys.end(); ++j) {
		// key must be valid
		assert(((*j) & 0xffu) != 0);
		m_keyToMask[static_cast<KeyButton>((*j) & 0xffu)] = mask;
	}

	// index keys by mask
	m_maskToKeys[mask].swap(keys);
}

void
CScreen::setToggleState(KeyModifierMask mask)
{
	// toggle modifiers that don't match the desired state
	KeyModifierMask different = (m_mask ^ mask);
	if ((different & KeyModifierCapsLock)   != 0) {
		toggleKey(KeyModifierCapsLock);
	}
	if ((different & KeyModifierNumLock)    != 0) {
		toggleKey(KeyModifierNumLock);
	}
	if ((different & KeyModifierScrollLock) != 0) {
		toggleKey(KeyModifierScrollLock);
	}
}

KeyButton
CScreen::isAnyKeyDown() const
{
	for (UInt32 i = 1; i <  256; ++i) {
		if ((m_keys[i] & kDown) != 0) {
			return static_cast<KeyButton>(i);
		}
	}
	return 0;
}

bool
CScreen::isKeyDown(KeyButton key) const
{
	key &= 0xffu;
	return (key != 0 && ((m_keys[key] & kDown) != 0));
}

bool
CScreen::isToggle(KeyModifierMask mask) const
{
	static const KeyModifierMask s_toggleMask =
		KeyModifierCapsLock | KeyModifierNumLock | KeyModifierScrollLock;
	return ((mask & s_toggleMask) != 0);
}

bool
CScreen::isHalfDuplex(KeyModifierMask mask) const
{
	return ((mask == KeyModifierCapsLock && m_capsLockHalfDuplex) ||
			(mask == KeyModifierNumLock  && m_numLockHalfDuplex));
}

bool
CScreen::isModifierActive(KeyModifierMask mask) const
{
	MaskToKeys::const_iterator i = m_maskToKeys.find(mask);
	if (i == m_maskToKeys.end()) {
		return false;
	}

	KeyButtons::const_iterator j = i->second.begin();
	if (isToggle(mask)) {
		// modifier is a toggle
		if (isKeyToggled(*j)) {
			return true;
		}
	}
	else {
		// modifier is not a toggle
		for (; j != i->second.end(); ++j) {
			if (isKeyDown(*j)) {
				return true;
			}
		}
	}
	return false;
}

KeyModifierMask
CScreen::getActiveModifiers() const
{
	if (m_isPrimary) {
		// we don't keep primary key state up to date so get the
		// current state.
		const_cast<CScreen*>(this)->updateKeys();
	}
	return m_mask;
}

bool
CScreen::mapModifier(Keystrokes& keys, Keystrokes& undo,
				KeyModifierMask mask, bool desireActive) const
{
	// look up modifier
	MaskToKeys::const_iterator i = m_maskToKeys.find(mask);
	if (i == m_maskToKeys.end()) {
		return false;
	}

	// ignore if already in desired state
	if (isModifierActive(mask) == desireActive) {
		return true;
	}

	// initialize keystroke
	Keystroke keystroke;
	keystroke.m_repeat = false;

	// handle toggles
	if (isToggle(mask)) {
		keystroke.m_key   = i->second.front();
		keystroke.m_press = true;
		keys.push_back(keystroke);
		keystroke.m_press = false;
		keys.push_back(keystroke);
		keystroke.m_press = false;
		undo.push_back(keystroke);
		keystroke.m_press = true;
		undo.push_back(keystroke);
	}

	else if (desireActive) {
		// press
		keystroke.m_key   = i->second.front();
		keystroke.m_press = true;
		keys.push_back(keystroke);
		keystroke.m_press = false;
		undo.push_back(keystroke);
	}

	else {
		// releasing a modifier is quite different from pressing one.
		// when we release a modifier we have to release every keycode that
		// is assigned to the modifier since the modifier is active if any
		// one of them is down.  when we press a modifier we just have to
		// press one of those keycodes.
		for (KeyButtons::const_iterator j = i->second.begin();
								j != i->second.end(); ++j) {
			if (isKeyDown(*j)) {
				keystroke.m_key   = *j;
				keystroke.m_press = false;
				keys.push_back(keystroke);
				keystroke.m_press = true;
				undo.push_back(keystroke);
			}
		}
	}

	return true;
}

KeyModifierMask
CScreen::getMaskForKey(KeyButton key) const
{
	KeyToMask::const_iterator i = m_keyToMask.find(key);
	if (i == m_keyToMask.end()) {
		return 0;
	}
	else {
		return i->second;
	}
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
CScreen::enterSecondary()
{
	// update our keyboard state to reflect the local state
	updateKeys();

	// remember toggle key state.  we'll restore this when we leave.
	m_toggleKeys = m_mask;
}

void
CScreen::leavePrimary()
{
	// do nothing
}

void
CScreen::leaveSecondary()
{
	// release any keys we think are still down
	releaseKeys();

	// restore toggle key state
	setToggleState(m_toggleKeys);
}

KeyModifierMask
CScreen::getModifierMask() const
{
	KeyModifierMask mask = 0;
	if (isModifierActive(KeyModifierShift)) {
		mask |= KeyModifierShift;
	}
	if (isModifierActive(KeyModifierControl)) {
		mask |= KeyModifierControl;
	}
	if (isModifierActive(KeyModifierAlt)) {
		mask |= KeyModifierAlt;
	}
	if (isModifierActive(KeyModifierMeta)) {
		mask |= KeyModifierMeta;
	}
	if (isModifierActive(KeyModifierSuper)) {
		mask |= KeyModifierSuper;
	}
	if (isModifierActive(KeyModifierModeSwitch)) {
		mask |= KeyModifierModeSwitch;
	}
	if (isModifierActive(KeyModifierNumLock)) {
		mask |= KeyModifierNumLock;
	}
	if (isModifierActive(KeyModifierCapsLock)) {
		mask |= KeyModifierCapsLock;
	}
	if (isModifierActive(KeyModifierScrollLock)) {
		mask |= KeyModifierScrollLock;
	}
	return mask;
}

void
CScreen::doKeystrokes(const Keystrokes& keys, SInt32 count)
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
					fakeKeyEvent(k->m_key, k->m_press, true);
				}
			}

			// note -- k is now on the first non-repeat key after the
			// repeat keys, exactly where we'd like to continue from.
		}
		else {
			// send event
			fakeKeyEvent(k->m_key, k->m_press, false);

			// next key
			++k;
		}
	}
}

void
CScreen::fakeKeyEvent(KeyButton key, bool press, bool repeat) const
{
	// half-duplex keys are special.  we ignore releases and convert
	// a press when the toggle is active to a release.
	KeyModifierMask mask = getMaskForKey(key);
	if (isHalfDuplex(mask)) {
		if (repeat || !press) {
			return;
		}
		if (isModifierActive(mask)) {
			press = false;
		}
	}

	// send key event
	LOG((CLOG_DEBUG2 "  %d %s%s", key, press ? "down" : "up", repeat ? " repeat" : ""));
	m_screen->fakeKeyEvent(key, press);
}

void
CScreen::updateKeyState(KeyButton button, KeyButton key, bool press)
{
	// ignore bogus keys
	key &= 0xffu;
	if (button == 0 || key == 0) {
		return;
	}

	// update shadow state.  shadow state doesn't change on auto-repeat.
	if (press) {
		// key is now down
		m_serverKeyMap[button] = key;
		m_keys[key]           |= kDown;
		m_fakeKeys[key]       |= kDown;
	}
	else {
		// key is now up
		m_serverKeyMap.erase(button);
		m_keys[key]     &= ~kDown;
		m_fakeKeys[key] &= ~kDown;
	}
	KeyModifierMask mask = getMaskForKey(key);
	if (mask != 0) {
		// key is a modifier
		if (isToggle(mask)) {
			// key is a toggle modifier
			if (press) {
				m_keys[key] ^= kToggled;
				m_mask      ^= mask;

				// if key is half duplex then don't report it as down
				if (isHalfDuplex(mask)) {
					m_keys[key]     &= ~kDown;
					m_fakeKeys[key] &= ~kDown;
				}
			}
		}
		else {
			// key is a normal modifier
			if (press) {
				m_mask |= mask;
			}
			else if (!isModifierActive(mask)) {
				// no key for modifier is down anymore
				m_mask &= ~mask;
			}
		}
		LOG((CLOG_DEBUG2 "new mask: 0x%04x", m_mask));
	}
}

void
CScreen::toggleKey(KeyModifierMask mask)
{
	// get the system key ID for this toggle key ID
	MaskToKeys::const_iterator i = m_maskToKeys.find(mask);
	if (i == m_maskToKeys.end()) {
		return;
	}
	KeyButton key = i->second.front();

	// toggle the key
	fakeKeyEvent(key, true, false);
	fakeKeyEvent(key, false, false);

	// toggle shadow state
	m_mask      ^= mask;
	key         &= 0xffu;
	m_keys[key] ^= kToggled;
}

bool
CScreen::isKeyToggled(KeyButton key) const
{
	key &= 0xffu;
	return (key != 0 && ((m_keys[key] & kToggled) != 0));
}
