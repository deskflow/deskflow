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

#include "CKeyState.h"
#include "IEventQueue.h"
#include "CLog.h"
#include <string.h>

//
// CKeyState
//

CKeyState::CKeyState() :
	m_halfDuplex(0),
	m_mask(0)
{
	memset(&m_keys, 0, sizeof(m_keys));
	memset(&m_serverKeyMap, 0, sizeof(m_serverKeyMap));
	memset(&m_keyToMask, 0, sizeof(m_keyToMask));
}

CKeyState::~CKeyState()
{
	// do nothing
}

void
CKeyState::setKeyDown(KeyButton button, bool down)
{
	button &= kButtonMask;
	updateKeyState(button, button, down, false);
}

void
CKeyState::setToggled(KeyModifierMask modifier)
{
	if (isToggle(modifier)) {
		const KeyButtons& buttons = m_maskToKeys[getIndexForModifier(modifier)];
		for (KeyButtons::const_iterator j = buttons.begin();
									j != buttons.end(); ++j) {
			m_keys[(*j) & kButtonMask] |= kToggled;
		}
	}
}

void
CKeyState::sendKeyEvent(
				void* target, bool press, bool isAutoRepeat,
				KeyID key, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	if (isHalfDuplex(m_keyToMask[button])) {
		if (isAutoRepeat) {
			// ignore auto-repeat on half-duplex keys
		}
		else {
			EVENTQUEUE->addEvent(CEvent(getKeyDownEvent(), target,
							CKeyInfo::alloc(key, mask, button, 1)));
			EVENTQUEUE->addEvent(CEvent(getKeyUpEvent(), target,
							CKeyInfo::alloc(key, mask, button, 1)));
		}
	}
	else {
		if (isAutoRepeat) {
			EVENTQUEUE->addEvent(CEvent(getKeyRepeatEvent(), target,
							 CKeyInfo::alloc(key, mask, button, count)));
		}
		else if (press) {
			EVENTQUEUE->addEvent(CEvent(getKeyDownEvent(), target,
							CKeyInfo::alloc(key, mask, button, 1)));
		}
		else {
			EVENTQUEUE->addEvent(CEvent(getKeyUpEvent(), target,
							CKeyInfo::alloc(key, mask, button, 1)));
		}
	}
}

void
CKeyState::updateKeys()
{
	static const KeyModifierMask s_masks[] = {
		KeyModifierShift,
		KeyModifierControl,
		KeyModifierAlt,
		KeyModifierMeta,
		KeyModifierSuper,
		KeyModifierModeSwitch,
		KeyModifierCapsLock,
		KeyModifierNumLock,
		KeyModifierScrollLock
	};

	// reset our state
	memset(&m_keys, 0, sizeof(m_keys));
	memset(&m_serverKeyMap, 0, sizeof(m_serverKeyMap));
	memset(&m_keyToMask, 0, sizeof(m_keyToMask));
	for (UInt32 i = 0; i < sizeof(m_maskToKeys)/sizeof(m_maskToKeys[0]); ++i) {
		m_maskToKeys[i].clear();
	}

	// let subclass set the state
	doUpdateKeys();

	// figure out the active modifiers
	m_mask = 0;
	for (UInt32 i = 0; i < sizeof(s_masks) / sizeof(s_masks[0]); ++i) {
		if (isModifierActive(s_masks[i])) {
			m_mask |= s_masks[i];
		}
	}
	LOG((CLOG_DEBUG2 "modifiers on update: 0x%04x", m_mask));
}

void
CKeyState::setHalfDuplexMask(KeyModifierMask mask)
{
	m_halfDuplex = mask & (KeyModifierCapsLock |
							KeyModifierNumLock |
							KeyModifierScrollLock);
}

void
CKeyState::fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
	// get the sequence of keys to simulate key press and the final
	// modifier state.
	Keystrokes keys;
	KeyButton localID =
		(KeyButton)(mapKey(keys, id, mask, false) & kButtonMask);
	if (keys.empty()) {
		// do nothing if there are no associated keys
		LOG((CLOG_DEBUG2 "cannot map key 0x%08x", id));
		return;
	}

	// generate key events
	fakeKeyEvents(keys, 1);

	// note that key is down
	updateKeyState((KeyButton)(button & kButtonMask), localID, true, true);
}

void
CKeyState::fakeKeyRepeat(
				KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	button &= kButtonMask;

	// if we haven't seen this button go down then ignore it
	KeyButton oldLocalID = m_serverKeyMap[button];
	if (oldLocalID == 0) {
		return;
	}

	// get the sequence of keys to simulate key repeat and the final
	// modifier state.
	Keystrokes keys;
	KeyButton localID = (KeyButton)(mapKey(keys, id, mask, true) & kButtonMask);
	if (localID == 0) {
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
	if (localID != oldLocalID) {
		// replace key up with previous key id but leave key down
		// alone so it uses the new keycode.
		for (Keystrokes::iterator index = keys.begin();
								index != keys.end(); ++index) {
			if (index->m_key == localID) {
				index->m_key = oldLocalID;
				break;
			}
		}

		// note that old key is now up
		m_keys[oldLocalID]    &= ~kDown;

		// map server key to new key
		m_serverKeyMap[button] = localID;

		// note that new key is now down
		m_keys[localID]       |= kDown;
	}

	// generate key events
	fakeKeyEvents(keys, count);
}

void
CKeyState::fakeKeyUp(KeyButton button)
{
	// if we haven't seen this button go down then ignore it
	KeyButton localID = m_serverKeyMap[button & kButtonMask];
	if (localID == 0) {
		return;
	}

	// get the sequence of keys to simulate key release
	Keystrokes keys;
	Keystroke keystroke;
	keystroke.m_key    = localID;
	keystroke.m_press  = false;
	keystroke.m_repeat = false;
	keys.push_back(keystroke);

	// generate key events
	fakeKeyEvents(keys, 1);

	// note that key is now up
	updateKeyState(button, localID, false, true);
}

void
CKeyState::fakeToggle(KeyModifierMask modifier)
{
	const KeyButtons& buttons = m_maskToKeys[getIndexForModifier(modifier)];
	if (buttons.empty() || !isToggle(modifier)) {
		return;
	}
	KeyButton button = buttons[0];

	// get the sequence of keys to simulate key toggle
	Keystrokes keys;
	Keystroke keystroke;
	keystroke.m_key    = button;
	keystroke.m_press  = true;
	keystroke.m_repeat = false;
	keys.push_back(keystroke);
	keystroke.m_press  = false;
	keys.push_back(keystroke);

	// generate key events
	fakeKeyEvents(keys, 1);

	// note the toggle
	m_keys[button] ^= kToggled;
	m_mask         ^= modifier;
}

bool
CKeyState::isKeyDown(KeyButton button) const
{
	return ((m_keys[button & kButtonMask] & kDown) != 0);
}

KeyModifierMask
CKeyState::getActiveModifiers() const
{
	return m_mask;
}

void
CKeyState::addModifier(KeyModifierMask modifier, const KeyButtons& buttons)
{
	// the mask must not be zero
	assert(modifier != 0);

	// the mask must have exactly one high bit
	assert((modifier & (modifier - 1)) == 0);

	for (KeyButtons::const_iterator j = buttons.begin();
							j != buttons.end(); ++j) {
		KeyButton button = static_cast<KeyButton>(((*j) & kButtonMask));
		if (button != 0) {
			m_keyToMask[button] = modifier;
		}
	}

	// index keys by mask
	m_maskToKeys[getIndexForModifier(modifier)] = buttons;
}

bool
CKeyState::mapModifier(Keystrokes& keys, Keystrokes& undo,
				KeyModifierMask mask, bool desireActive, bool force) const
{
	// look up modifier
	const KeyButtons& buttons = m_maskToKeys[getIndexForModifier(mask)];
	if (buttons.empty()) {
		return false;
	}

	// ignore if already in desired state
	if (!force && isModifierActive(mask) == desireActive) {
		return true;
	}

	// initialize keystroke
	Keystroke keystroke;
	keystroke.m_repeat = false;

	// handle toggles
	if (isToggle(mask)) {
		keystroke.m_key   = buttons[0];
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
		keystroke.m_key   = buttons[0];
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
		for (KeyButtons::const_iterator j = buttons.begin();
								j != buttons.end(); ++j) {
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

bool
CKeyState::isToggle(KeyModifierMask mask) const
{
	return (mask == KeyModifierCapsLock ||
			mask == KeyModifierNumLock ||
			mask == KeyModifierScrollLock);
}

bool
CKeyState::isHalfDuplex(KeyModifierMask mask) const
{
	return ((mask & m_halfDuplex) != 0);
}

bool
CKeyState::isModifierActive(KeyModifierMask mask) const
{
	const KeyButtons& buttons = m_maskToKeys[getIndexForModifier(mask)];
	KeyButtons::const_iterator j = buttons.begin();
	if (j != buttons.end()) {
		if (isToggle(mask)) {
			// modifier is a toggle
			if ((m_keys[*j] & kToggled) != 0) {
				return true;
			}
		}
		else {
			// modifier is not a toggle
			for (; j != buttons.end(); ++j) {
				if ((m_keys[*j] & kDown) != 0) {
					return true;
				}
			}
		}
	}
	return false;
}

UInt32
CKeyState::getIndexForModifier(KeyModifierMask mask) const
{
	switch (mask) {
	case KeyModifierShift:
		return 0;

	case KeyModifierControl:
		return 1;

	case KeyModifierAlt:
		return 2;

	case KeyModifierMeta:
		return 3;

	case KeyModifierSuper:
		return 4;

	case KeyModifierModeSwitch:
		return 5;

	case KeyModifierCapsLock:
		return 6;

	case KeyModifierNumLock:
		return 7;

	case KeyModifierScrollLock:
		return 8;

	default:
		assert(0 && "invalid modifier mask");
		return 0;
	}
}

void
CKeyState::fakeKeyEvents(const Keystrokes& keys, UInt32 count)
{
	// do nothing if no keys or no repeats
	if (count == 0 || keys.empty()) {
		return;
	}

	// generate key events
	LOG((CLOG_DEBUG2 "keystrokes:"));
	for (Keystrokes::const_iterator k = keys.begin(); k != keys.end(); ) {
		if (k->m_repeat) {
			// repeat from here up to but not including the next key
			// with m_repeat == false count times.
			Keystrokes::const_iterator start = k;
			while (count-- > 0) {
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
CKeyState::fakeKeyEvent(KeyButton button, bool press, bool isAutoRepeat)
{
	// half-duplex keys are special.  we ignore releases and convert
	// a press when the toggle is active to a release.
	KeyModifierMask mask = m_keyToMask[button];
	if (isHalfDuplex(mask)) {
		if (isAutoRepeat || !press) {
			return;
		}
		if (isModifierActive(mask)) {
			press = false;
		}
	}

	// send key event
	LOG((CLOG_DEBUG2 "  %d %s%s", button, press ? "down" : "up", isAutoRepeat ? " repeat" : ""));
	doFakeKeyEvent(button, press, isAutoRepeat);
}

void
CKeyState::updateKeyState(KeyButton serverID, KeyButton localID,
				bool press, bool fake)
{
	// ignore bogus keys
	if (serverID == 0 || localID == 0) {
		return;
	}

	// update key state.  state doesn't change when auto-repeating.
	if (press) {
		m_serverKeyMap[serverID] = localID;
		m_keys[localID]         |= kDown;
	}
	else {
		m_serverKeyMap[serverID] = 0;
		m_keys[localID]         &= ~kDown;
	}

	// update modifier state
	KeyModifierMask mask = m_keyToMask[localID];
	if (mask != 0) {
		if (isToggle(mask)) {
			// never report half-duplex keys as down
			if (isHalfDuplex(mask)) {
				m_keys[localID] &= ~kDown;
				// half-duplex keys on the primary screen don't send the
				// usual press/release pairs but instead send the press
				// when toggling on and the release when toggleing off.
				// since we normally toggle our shadow state on the press
				// we need to treat the release as a press on the primary
				// screen.  we know we're on the primary screen if fake is
				// false.  secondary screens always get press/release pairs.
				if (!fake) {
					press = true;
				}
			}

			// toggle on the press
			if (press) {
				m_keys[localID] ^= kToggled;
				m_mask          ^= mask;
			}
		}
		else {
			if (press) {
				m_mask |= mask;
			}
			else if (!isModifierActive(mask)) {
				m_mask &= ~mask;
			}
		}
		LOG((CLOG_DEBUG2 "new mask: 0x%04x", m_mask));
	}
}
