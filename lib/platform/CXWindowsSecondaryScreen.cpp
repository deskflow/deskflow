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

#include "CXWindowsSecondaryScreen.h"
#include "CXWindowsClipboard.h"
#include "CXWindowsScreen.h"
#include "CXWindowsScreenSaver.h"
#include "CXWindowsUtil.h"
#include "IScreenReceiver.h"
#include "XScreen.h"
#include "CThread.h"
#include "CLog.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/X.h>
#	include <X11/Xutil.h>
#	define XK_MISCELLANY
#	define XK_XKB_KEYS
#	include <X11/keysymdef.h>
#	if defined(HAVE_X11_EXTENSIONS_XTEST_H)
#		include <X11/extensions/XTest.h>
#	else
#		error The XTest extension is required to build synergy
#	endif
#	if defined(HAVE_X11_XF86KEYSYM_H)
#		include <X11/XF86keysym.h>
#	endif
#endif

//
// utility functions
//

inline
static
unsigned int			getBits(unsigned int src, unsigned int mask)
{
	return src & mask;
}

inline
static
unsigned int			setBits(unsigned int src, unsigned int mask)
{
	return src | mask;
}

inline
static
unsigned int			clearBits(unsigned int src, unsigned int mask)
{
	return src & ~mask;
}

inline
static
unsigned int			flipBits(unsigned int src, unsigned int mask)
{
	return src ^ mask;
}

inline
static
unsigned int			assignBits(unsigned int src,
							unsigned int mask, unsigned int value)
{
	return setBits(clearBits(src, mask), clearBits(value, ~mask));
}


//
// CXWindowsSecondaryScreen
//

CXWindowsSecondaryScreen::CXWindowsSecondaryScreen(IScreenReceiver* receiver) :
	CSecondaryScreen(),
	m_window(None)
{
	m_screen = new CXWindowsScreen(receiver, this);
}

CXWindowsSecondaryScreen::~CXWindowsSecondaryScreen()
{
	assert(m_window == None);
	delete m_screen;
}

void
CXWindowsSecondaryScreen::keyDown(KeyID key,
				KeyModifierMask mask, KeyButton button)
{
	Keystrokes keys;
	KeyCode keycode;

	// get the sequence of keys to simulate key press and the final
	// modifier state.
	m_mask = mapKey(keys, keycode, key, mask, kPress);
	if (keys.empty()) {
		return;
	}

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now down
	m_keys[keycode]     = true;
	m_fakeKeys[keycode] = true;

	// note which server key generated this key
	m_serverKeyMap[button] = keycode;
}

void
CXWindowsSecondaryScreen::keyRepeat(KeyID key,
				KeyModifierMask mask, SInt32 count, KeyButton button)
{
	Keystrokes keys;
	KeyCode keycode;

	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}

	// get the sequence of keys to simulate key repeat and the final
	// modifier state.
	m_mask = mapKey(keys, keycode, key, mask, kRepeat);
	if (keys.empty()) {
		return;
	}

	// if this keycode shouldn't auto-repeat then ignore
	if ((m_keyControl.auto_repeats[keycode >> 3] & (1 << (keycode & 7))) == 0) {
		return;
	}

	// if we've seen this button (and we should have) then make sure
	// we release the same key we pressed when we saw it.
	if (index != m_serverKeyMap.end() && keycode != index->second) {
		// replace key up with previous keycode but leave key down
		// alone so it uses the new keycode and store that keycode
		// in the server key map.
		for (Keystrokes::iterator index2 = keys.begin();
								index2 != keys.end(); ++index2) {
			if (index2->m_keycode == index->second) {
				index2->m_keycode = index->second;
				break;
			}
		}

		// note that old key is now up
		m_keys[index->second]     = false;
		m_fakeKeys[index->second] = false;

		// map server key to new key
		index->second = keycode;

		// note that new key is now down
		m_keys[index->second]     = true;
		m_fakeKeys[index->second] = true;
	}

	// generate key events
	doKeystrokes(keys, count);
}

void
CXWindowsSecondaryScreen::keyUp(KeyID key,
				KeyModifierMask mask, KeyButton button)
{
	Keystrokes keys;
	KeyCode keycode;

	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}

	// get the sequence of keys to simulate key release and the final
	// modifier state.
	m_mask = mapKey(keys, keycode, key, mask, kRelease);

	// if there are no keys to generate then we should at least generate
	// a key release for the key we pressed.
	if (keys.empty()) {
		Keystroke keystroke;
		keycode             = index->second;
		keystroke.m_keycode = keycode;
		keystroke.m_press   = False;
		keystroke.m_repeat  = false;
		keys.push_back(keystroke);
	}

	// if we've seen this button (and we should have) then make sure
	// we release the same key we pressed when we saw it.
	if (index != m_serverKeyMap.end() && keycode != index->second) {
		// replace key up with previous keycode
		for (Keystrokes::iterator index2 = keys.begin();
								index2 != keys.end(); ++index2) {
			if (index2->m_keycode == keycode) {
				index2->m_keycode = index->second;
				break;
			}
		}

		// use old keycode
		keycode = index->second;
	}

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now up
	m_keys[keycode]     = false;
	m_fakeKeys[keycode] = false;

	// remove server key from map
	if (index != m_serverKeyMap.end()) {
		m_serverKeyMap.erase(index);
	}
}

void
CXWindowsSecondaryScreen::mouseDown(ButtonID button)
{
	const unsigned int xButton = mapButton(button);
	if (xButton != 0) {
		CDisplayLock display(m_screen);
		XTestFakeButtonEvent(display, xButton, True, CurrentTime);
		XSync(display, False);
	}
}

void
CXWindowsSecondaryScreen::mouseUp(ButtonID button)
{
	const unsigned int xButton = mapButton(button);
	if (xButton != 0) {
		CDisplayLock display(m_screen);
		XTestFakeButtonEvent(display, xButton, False, CurrentTime);
		XSync(display, False);
	}
}

void
CXWindowsSecondaryScreen::mouseMove(SInt32 x, SInt32 y)
{
	warpCursor(x, y);
}

void
CXWindowsSecondaryScreen::mouseWheel(SInt32 delta)
{
	// choose button depending on rotation direction
	const unsigned int xButton = mapButton(static_cast<ButtonID>(
												(delta >= 0) ? -1 : -2));
	if (xButton == 0) {
		return;
	}

	// now use absolute value of delta
	if (delta < 0) {
		delta = -delta;
	}

	// send as many clicks as necessary
	CDisplayLock display(m_screen);
	for (; delta >= 120; delta -= 120) {
		XTestFakeButtonEvent(display, xButton, True, CurrentTime);
		XTestFakeButtonEvent(display, xButton, False, CurrentTime);
	}
	XSync(display, False);
}

void
CXWindowsSecondaryScreen::resetOptions()
{
	m_numLockHalfDuplex  = false;
	m_capsLockHalfDuplex = false;
	CSecondaryScreen::resetOptions();
}

void
CXWindowsSecondaryScreen::setOptions(const COptionsList& options)
{
	for (UInt32 i = 0, n = options.size(); i < n; i += 2) {
		if (options[i] == kOptionHalfDuplexCapsLock) {
			m_capsLockHalfDuplex = (options[i + 1] != 0);
			LOG((CLOG_DEBUG1 "half-duplex caps-lock %s", m_capsLockHalfDuplex ? "on" : "off"));
		}
		else if (options[i] == kOptionHalfDuplexNumLock) {
			m_numLockHalfDuplex = (options[i + 1] != 0);
			LOG((CLOG_DEBUG1 "half-duplex num-lock %s", m_numLockHalfDuplex ? "on" : "off"));
		}
	}
	CSecondaryScreen::setOptions(options);
}

IScreen*
CXWindowsSecondaryScreen::getScreen() const
{
	return m_screen;
}

void
CXWindowsSecondaryScreen::onScreensaver(bool)
{
	// ignore
}

bool
CXWindowsSecondaryScreen::onPreDispatch(const CEvent*)
{
	return false;
}

bool
CXWindowsSecondaryScreen::onEvent(CEvent* event)
{
	assert(event != NULL);
	XEvent& xevent = event->m_event;

	// handle event
	switch (xevent.type) {
	case MappingNotify: {
		// keyboard mapping changed
		CDisplayLock display(m_screen);
		doUpdateKeys(display);
		return true;
	}

	case LeaveNotify:
		// mouse moved out of hider window somehow.  hide the window.
		hideWindow();
		return true;
	}
}

void
CXWindowsSecondaryScreen::onOneShotTimerExpired(UInt32)
{
	// ignore
}

SInt32
CXWindowsSecondaryScreen::getJumpZoneSize() const
{
	return 0;
}

void
CXWindowsSecondaryScreen::onPreMainLoop()
{
	assert(m_window != None);
}

void
CXWindowsSecondaryScreen::onPreOpen()
{
	assert(m_window == None);
}

void
CXWindowsSecondaryScreen::onPostOpen()
{
	assert(m_window != None);

	// get the keyboard control state
	CDisplayLock display(m_screen);
	XGetKeyboardControl(display, &m_keyControl);
}

void
CXWindowsSecondaryScreen::onPreClose()
{
	if (m_keyControl.global_auto_repeat == AutoRepeatModeOn) {
		CDisplayLock display(m_screen);
		XAutoRepeatOn(display);
	}
}

void
CXWindowsSecondaryScreen::onPreEnter()
{
	assert(m_window != None);
}

void
CXWindowsSecondaryScreen::onPostEnter()
{
	assert(m_window != None);

	// get the keyboard control state
	CDisplayLock display(m_screen);
	XGetKeyboardControl(display, &m_keyControl);

	// turn off auto-repeat.  we do this so fake key press events don't
	// cause the local server to generate their own auto-repeats of
	// those keys.
	XAutoRepeatOff(display);
}

void
CXWindowsSecondaryScreen::onPreLeave()
{
	assert(m_window != None);

	// restore the previous keyboard auto-repeat state.  if the user
	// changed the auto-repeat configuration while on the client then
	// that state is lost.  that's because we can't get notified by
	// the X server when the auto-repeat configuration is changed so
	// we can't track the desired configuration.
	if (m_keyControl.global_auto_repeat == AutoRepeatModeOn) {
		CDisplayLock display(m_screen);
		XAutoRepeatOn(display);
	}
}

void
CXWindowsSecondaryScreen::createWindow()
{
	{
		CDisplayLock display(m_screen);

		// verify the availability of the XTest extension
		int majorOpcode, firstEvent, firstError;
		if (!XQueryExtension(display, XTestExtensionName,
								&majorOpcode, &firstEvent, &firstError)) {
			LOG((CLOG_ERR "XTEST extension not available"));
			throw XScreenOpenFailure();
		}

		// cursor hider window attributes.  this window is used to hide the
		// cursor when it's not on the screen.  the window is hidden as soon
		// as the cursor enters the screen or the display's real cursor is
		// moved.
		XSetWindowAttributes attr;
		attr.event_mask            = LeaveWindowMask;
		attr.do_not_propagate_mask = 0;
		attr.override_redirect     = True;
		attr.cursor                = m_screen->getBlankCursor();

		// create the cursor hider window
		m_window = XCreateWindow(display, m_screen->getRoot(),
								0, 0, 1, 1, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);
		if (m_window == None) {
			throw XScreenOpenFailure();
		}
		LOG((CLOG_DEBUG "window is 0x%08x", m_window));

		// become impervious to server grabs
		XTestGrabControl(display, True);
	}

	// tell generic screen about the window
	m_screen->setWindow(m_window);
}

void
CXWindowsSecondaryScreen::destroyWindow()
{
	{
		CDisplayLock display(m_screen);
		if (display != NULL) {
			// release keys that are still pressed
			doReleaseKeys(display);

			// no longer impervious to server grabs
			XTestGrabControl(display, False);

			// update
			XSync(display, False);
		}
	}

	// destroy window
	if (m_window != None) {
		m_screen->setWindow(None);
		CDisplayLock display(m_screen);
		if (display != NULL) {
			XDestroyWindow(display, m_window);
		}
		m_window = None;
	}
}

void
CXWindowsSecondaryScreen::showWindow(SInt32 x, SInt32 y)
{
	{
		CDisplayLock display(m_screen);

		// move hider window under the given position
		XMoveWindow(display, m_window, x, y);

		// raise and show the hider window.  take activation.
		// FIXME -- take focus?
		XMapRaised(display, m_window);
	}

	// now warp the mouse.  we warp after showing the window so we're
	// guaranteed to get the mouse leave event and to prevent the
	// keyboard focus from changing under point-to-focus policies.
	warpCursor(x, y);
}

void
CXWindowsSecondaryScreen::hideWindow()
{
	assert(m_window != None);

	CDisplayLock display(m_screen);
	XUnmapWindow(display, m_window);
}

void
CXWindowsSecondaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	CDisplayLock display(m_screen);
	Display* pDisplay = display;
	XTestFakeMotionEvent(display, DefaultScreen(pDisplay), x, y, CurrentTime);
	XSync(display, False);
}

void
CXWindowsSecondaryScreen::setToggleState(KeyModifierMask mask)
{
	CDisplayLock display(m_screen);

	// toggle modifiers that don't match the desired state
	unsigned int xMask = maskToX(mask);
	if ((xMask & m_capsLockMask)   != (m_mask & m_capsLockMask)) {
		toggleKey(display, XK_Caps_Lock, m_capsLockMask);
	}
	if ((xMask & m_numLockMask)    != (m_mask & m_numLockMask)) {
		toggleKey(display, XK_Num_Lock, m_numLockMask);
	}
	if ((xMask & m_scrollLockMask) != (m_mask & m_scrollLockMask)) {
		toggleKey(display, XK_Scroll_Lock, m_scrollLockMask);
	}
}

KeyModifierMask
CXWindowsSecondaryScreen::getToggleState() const
{
	KeyModifierMask mask = 0;
	if ((m_mask & m_capsLockMask) != 0) {
		mask |= KeyModifierCapsLock;
	}
	if ((m_mask & m_numLockMask) != 0) {
		mask |= KeyModifierNumLock;
	}
	if ((m_mask & m_scrollLockMask) != 0) {
		mask |= KeyModifierScrollLock;
	}
	return mask;
}

unsigned int
CXWindowsSecondaryScreen::mapButton(ButtonID id) const
{
	// map button -1 to button 4 (+wheel)
	if (id == static_cast<ButtonID>(-1)) {
		id = 4;
	}

	// map button -2 to button 5 (-wheel)
	else if (id == static_cast<ButtonID>(-2)) {
		id = 5;
	}

	// map buttons 4, 5, etc. to 6, 7, etc. to make room for buttons
	// 4 and 5 used to simulate the mouse wheel.
	else if (id >= 4) {
		id += 2;
	}

	// check button is in legal range
	if (id < 1 || id > m_buttons.size()) {
		// out of range
		return 0;
	}

	// map button
	return static_cast<unsigned int>(m_buttons[id - 1]);
}

KeyModifierMask
CXWindowsSecondaryScreen::mapKey(Keystrokes& keys, KeyCode& keycode,
				KeyID id, KeyModifierMask mask, EKeyAction action) const
{
	// note -- must have display locked on entry

	// the system translates key events into characters depending
	// on the modifier key state at the time of the event.  to
	// generate the right keysym we need to set the modifier key
	// states appropriately.
	//
	// the mask passed by the caller is the desired mask.  however,
	// there may not be a keycode mapping to generate the desired
	// keysym with that mask.  we override the bits in the mask
	// that cannot be accomodated.

	// note if the key is "half-duplex"
	const bool isHalfDuplex = ((id == kKeyCapsLock && m_capsLockHalfDuplex) ||
								(id == kKeyNumLock && m_numLockHalfDuplex));

	// ignore releases and repeats for half-duplex keys
	if (isHalfDuplex && action != kPress) {
		return m_mask;
	}

	// convert the id to a keysym and adjust the mask if necessary
	unsigned int outMask = m_mask;
	KeyCodeIndex keyIndex = findKey(id, outMask);
	if (keyIndex == noKey()) {
		// cannot convert id to keysym
		LOG((CLOG_DEBUG2 "no keysym for key"));
		return m_mask;
	}

	// get the keysym we're trying to generate and possible keycodes
	KeySym keysym = keyIndex->first;
	const KeyCodeMask& entry = keyIndex->second;

	// we can choose any of the available keycode/modifier states to
	// generate our keysym.  the most desireable is the one most
	// closely matching the input mask.  determine the order we
	// should try modifier states, from best match to worst.  this
	// doesn't concern itself with whether or not a given modifier
	// state has an associated keycode.  we'll just skip those later
	// if necessary.

	// default is none, shift, mode switch, shift + mode switch
	unsigned int desired = maskToX(mask);
	unsigned int index[4];
	index[0] = 0;
	index[1] = 1;
	index[2] = 2;
	index[3] = 3;

	// if mode switch is active then 2 and 3 are better than 0 and 1
	if (getBits(desired, m_modeSwitchMask) != 0) {
		index[0] ^= 2;
		index[1] ^= 2;
		index[2] ^= 2;
		index[3] ^= 2;
	}

	// if shift is active then 1 and 3 are better than 0 and 2.  however,
	// if the key is affected by NumLock and NumLock is active then 1 and
	// 3 are better if shift is *not* down (because NumLock acts like
	// shift for those keysyms and shift cancels NumLock).  similarly for
	// keys affected by CapsLock.
	bool desireShift = (getBits(desired, ShiftMask) != 0);
	bool invertShift = false;
	LOG((CLOG_DEBUG2 "desire shift: %s", desireShift ? "yes" : "no"));
	if (adjustForNumLock(keysym)) {
		LOG((CLOG_DEBUG2 "num lock sensitive"));
		if (m_numLockMask != 0) {
			LOG((CLOG_DEBUG2 "we have num lock"));
			if (getBits(desired, m_numLockMask) != 0) {
				LOG((CLOG_DEBUG2 "num lock desired, invert shift"));
				invertShift = true;
			}
		}
	}
	else if (adjustForCapsLock(keysym)) {
		LOG((CLOG_DEBUG2 "caps lock sensitive"));
		if (m_capsLockMask != 0) {
			LOG((CLOG_DEBUG2 "we have caps lock"));
			if (getBits(desired, m_capsLockMask) != 0) {
				LOG((CLOG_DEBUG2 "caps lock desired, invert shift"));
				invertShift = true;
			}
		}
	}
	if (desireShift != invertShift) {
		index[0] ^= 1;
		index[1] ^= 1;
		index[2] ^= 1;
		index[3] ^= 1;
	}

	// find the first modifier state with a keycode we can generate.
	// note that if m_modeSwitchMask is 0 then we can't generate
	// m_keycode[2] and m_keycode[3].
	unsigned int bestIndex;
	for (bestIndex = 0; bestIndex < 4; ++bestIndex) {
		if (entry.m_keycode[index[bestIndex]] != 0) {
			if (index[bestIndex] < 2 || m_modeSwitchMask != 0) {
				bestIndex = index[bestIndex];
				break;
			}
		}
	}
	if (bestIndex == 4) {
		// no keycode/modifiers to generate the keysym
		return m_mask;
	}

	// get the keycode
	keycode = entry.m_keycode[bestIndex];
	LOG((CLOG_DEBUG2 "bestIndex = %d, keycode = %d", bestIndex, keycode));

	// note if the key is a modifier
	ModifierMap::const_iterator modIndex = m_keycodeToModifier.find(keycode);
	unsigned int modifierBit = 0;
	if (modIndex != m_keycodeToModifier.end()) {
		modifierBit = (1 << modIndex->second);
	}

	// if the key is a modifier and that modifier is already in the
	// desired state then ignore the request since there's nothing
	// to do.  never ignore a toggle modifier on press or release,
	// though.
	if (modifierBit != 0) {
		if (action == kRepeat) {
			LOG((CLOG_DEBUG2 "ignore repeating modifier"));
			return m_mask;
		}
		if (getBits(m_toggleModifierMask, modifierBit) == 0) {
			if ((action == kPress   && (m_mask & modifierBit) != 0) ||
				(action == kRelease && (m_mask & modifierBit) == 0)) {
				LOG((CLOG_DEBUG2 "modifier in proper state: 0x%04x", m_mask));
				return m_mask;
			}
		}
	}

	// bestIndex tells us if shift and mode switch should be on or off,
	// except if caps lock or num lock was down then we invert the sense
	// of bestIndex's lowest bit.
	// we must match both.
	unsigned int required = ShiftMask | m_modeSwitchMask;
	if (((bestIndex & 1) == 0) != invertShift) {
		desired = clearBits(desired, ShiftMask);
	}
	else {
		desired = setBits(desired, ShiftMask);
	}
	if ((bestIndex & 2) == 0) {
		desired = clearBits(desired, m_modeSwitchMask);
	}
	else {
		desired = setBits(desired, m_modeSwitchMask);
	}

	// if the key is a modifier then remove it from the desired mask.
	// we'll be matching the modifiers in the desired mask then adding
	// a key press or release for the keysym.  if we don't clear the
	// modifier bit from the desired mask we'll end up dealing with
	// that key twice, once while matching modifiers and once while
	// handling the keysym.
	//
	// note that instead of clearing the bit, we make it identical to
	// the same bit in m_mask, meaning it's already in the right state.
	desired  = assignBits(desired, modifierBit, m_mask);
	required = clearBits(required, modifierBit);
	LOG((CLOG_DEBUG2 "desired = 0x%04x, current = 0x%04x", desired, m_mask));

	// some modifiers never have an effect on keysym lookup.  leave
	// those modifiers alone by copying their state from m_mask to
	// desired.
	desired = assignBits(desired,
							ControlMask |
							m_altMask |
							m_metaMask |
							m_superMask |
							m_scrollLockMask, m_mask);

	// add the key events required to get to the modifier state
	// necessary to generate an event yielding id.  also save the
	// key events required to restore the state.  if the key is
	// a modifier key then skip this because modifiers should not
	// modify modifiers.
	Keystrokes undo;
	Keystroke keystroke;
	if (desired != m_mask) {
		for (unsigned int i = 0; i < 8; ++i) {
			unsigned int bit = (1 << i);
			if (getBits(desired, bit) != getBits(m_mask, bit)) {
				LOG((CLOG_DEBUG2 "fix modifier %d", i));
				// get the keycode we're using for this modifier.  if
				// there isn't one then bail if the modifier is required
				// or ignore it if not required.
				KeyCode modifierKey = m_modifierToKeycode[i];
				if (modifierKey == 0) {
					LOG((CLOG_DEBUG2 "no key mapped to modifier 0x%04x", bit));
					if (getBits(required, bit) != 0) {
						keys.clear();
						return m_mask;
					}
					else {
						continue;
					}
				}

				keystroke.m_keycode = modifierKey;
				keystroke.m_repeat  = false;
				if (getBits(desired, bit)) {
					// modifier is not active but should be.  if the
					// modifier is a toggle then toggle it on with a
					// press/release, otherwise activate it with a
					// press.  use the first keycode for the modifier.
					LOG((CLOG_DEBUG2 "modifier 0x%04x is not active", bit));
					if (getBits(m_toggleModifierMask, bit) != 0) {
						LOG((CLOG_DEBUG2 "modifier 0x%04x is a toggle", bit));
						if ((bit == m_capsLockMask && m_capsLockHalfDuplex) ||
							(bit == m_numLockMask && m_numLockHalfDuplex)) {
							keystroke.m_press = True;
							keys.push_back(keystroke);
							keystroke.m_press = False;
							undo.push_back(keystroke);
						}
						else {
							keystroke.m_press = True;
							keys.push_back(keystroke);
							keystroke.m_press = False;
							keys.push_back(keystroke);
							undo.push_back(keystroke);
							keystroke.m_press = True;
							undo.push_back(keystroke);
						}
					}
					else {
						keystroke.m_press = True;
						keys.push_back(keystroke);
						keystroke.m_press = False;
						undo.push_back(keystroke);
					}
				}

				else {
					// modifier is active but should not be.  if the
					// modifier is a toggle then toggle it off with a
					// press/release, otherwise deactivate it with a
					// release.  we must check each keycode for the
					// modifier if not a toggle.
					LOG((CLOG_DEBUG2 "modifier 0x%04x is active", bit));
					if (getBits(m_toggleModifierMask, bit) != 0) {
						LOG((CLOG_DEBUG2 "modifier 0x%04x is a toggle", bit));
						if ((bit == m_capsLockMask && m_capsLockHalfDuplex) ||
							(bit == m_numLockMask && m_numLockHalfDuplex)) {
							keystroke.m_press = False;
							keys.push_back(keystroke);
							keystroke.m_press = True;
							undo.push_back(keystroke);
						}
						else {
							keystroke.m_press = True;
							keys.push_back(keystroke);
							keystroke.m_press = False;
							keys.push_back(keystroke);
							undo.push_back(keystroke);
							keystroke.m_press = True;
							undo.push_back(keystroke);
						}
					}
					else {
						for (unsigned int j = 0; j < m_keysPerModifier; ++j) {
							const KeyCode key =
								m_modifierToKeycodes[i * m_keysPerModifier + j];
							if (key != 0 && m_keys[key]) {
								keystroke.m_keycode = key;
								keystroke.m_press   = False;
								keys.push_back(keystroke);
								keystroke.m_press   = True;
								undo.push_back(keystroke);
							}
						}
					}
				}
			}
		}
	}

	// note if the press of a half-duplex key should be treated as a release
	if (isHalfDuplex && getBits(m_mask, modifierBit) != 0) {
		action = kRelease;
	}

	// add the key event
	keystroke.m_keycode = keycode;
	switch (action) {
	case kPress:
		keystroke.m_press  = True;
		keystroke.m_repeat = false;
		keys.push_back(keystroke);
		break;

	case kRelease:
		keystroke.m_press  = False;
		keystroke.m_repeat = false;
		keys.push_back(keystroke);
		break;

	case kRepeat:
		keystroke.m_press  = False;
		keystroke.m_repeat = true;
		keys.push_back(keystroke);
		keystroke.m_press  = True;
		keys.push_back(keystroke);
		break;
	}

	// add key events to restore the modifier state.  apply events in
	// the reverse order that they're stored in undo.
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	// if the key is a modifier key then compute the modifier map after
	// this key is pressed or released.
	mask = m_mask;
	if (modifierBit != 0) {
		// can't be repeating if we've gotten here
		assert(action != kRepeat);

		// toggle keys modify the state on release.  other keys set the
		// bit on press and clear the bit on release.  if half-duplex
		// then toggle each time we get here.
		if (getBits(m_toggleModifierMask, modifierBit) != 0) {
			if (isHalfDuplex || action == kRelease) {
				mask = flipBits(mask, modifierBit);
			}
		}
		else if (action == kPress) {
			mask = setBits(mask, modifierBit);
		}
		else if (action == kRelease) {
			// can't reset bit until all keys that set it are released.
			// scan those keys to see if any (except keycode) are pressed.
			bool down = false;
			for (unsigned int j = 0; !down && j < m_keysPerModifier; ++j) {
				KeyCode modKeycode = m_modifierToKeycodes[modIndex->second *
													m_keysPerModifier + j];
				if (modKeycode != 0 && modKeycode != keycode) {
					down = m_keys[modKeycode];
				}
			}
			if (!down) {
				mask = clearBits(mask, modifierBit);
			}
		}
	}

	LOG((CLOG_DEBUG2 "final mask: 0x%04x", mask));
	return mask;
}

void
CXWindowsSecondaryScreen::doKeystrokes(const Keystrokes& keys, SInt32 count)
{
	// do nothing if no keys or no repeats
	if (count < 1 || keys.empty()) {
		return;
	}

	// lock display
	CDisplayLock display(m_screen);

	// generate key events
	for (Keystrokes::const_iterator k = keys.begin(); k != keys.end(); ) {
		if (k->m_repeat) {
			// repeat from here up to but not including the next key
			// with m_repeat == false count times.
			Keystrokes::const_iterator start = k;
			for (; count > 0; --count) {
				// send repeating events
				for (k = start; k != keys.end() && k->m_repeat; ++k) {
					XTestFakeKeyEvent(display,
								k->m_keycode, k->m_press, CurrentTime);
				}
			}

			// note -- k is now on the first non-repeat key after the
			// repeat keys, exactly where we'd like to continue from.
		}
		else {
			// send event
			XTestFakeKeyEvent(display, k->m_keycode, k->m_press, CurrentTime);

			// next key
			++k;
		}
	}

	// update
	XSync(display, False);
}

unsigned int
CXWindowsSecondaryScreen::maskToX(KeyModifierMask inMask) const
{
	unsigned int outMask = 0;
	if (inMask & KeyModifierShift) {
		outMask |= ShiftMask;
	}
	if (inMask & KeyModifierControl) {
		outMask |= ControlMask;
	}
	if (inMask & KeyModifierAlt) {
		outMask |= m_altMask;
	}
	if (inMask & KeyModifierMeta) {
		outMask |= m_metaMask;
	}
	if (inMask & KeyModifierSuper) {
		outMask |= m_superMask;
	}
	if (inMask & KeyModifierModeSwitch) {
		outMask |= m_modeSwitchMask;
	}
	if (inMask & KeyModifierCapsLock) {
		outMask |= m_capsLockMask;
	}
	if (inMask & KeyModifierNumLock) {
		outMask |= m_numLockMask;
	}
	if (inMask & KeyModifierScrollLock) {
		outMask |= m_scrollLockMask;
	}
	return outMask;
}

void
CXWindowsSecondaryScreen::doReleaseKeys(Display* display)
{
	assert(display != NULL);

	// key release for each key that we faked a press for
	for (UInt32 i = 0; i < 256; ++i) {
		if (m_fakeKeys[i]) {
			XTestFakeKeyEvent(display, i, False, CurrentTime);
			m_fakeKeys[i] = false;
			m_keys[i]     = false;
		}
	}
}

void
CXWindowsSecondaryScreen::doUpdateKeys(Display* display)
{
	// query the button mapping
	UInt32 numButtons = XGetPointerMapping(display, NULL, 0);
	unsigned char* tmpButtons = new unsigned char[numButtons];
	XGetPointerMapping(display, tmpButtons, numButtons);

	// find the largest logical button id
	unsigned char maxButton = 0;
	for (UInt32 i = 0; i < numButtons; ++i) {
		if (tmpButtons[i] > maxButton) {
			maxButton = tmpButtons[i];
		}
	}

	// allocate button array
	m_buttons.resize(maxButton);

	// fill in button array values.  m_buttons[i] is the physical
	// button number for logical button i+1.
	for (UInt32 i = 0; i < numButtons; ++i) {
		m_buttons[i] = 0;
	}
	for (UInt32 i = 0; i < numButtons; ++i) {
		m_buttons[tmpButtons[i] - 1] = i + 1;
	}

	// clean up
	delete[] tmpButtons;

	// update mappings and current modifiers
	updateModifierMap(display);
	updateKeycodeMap(display);
	updateModifiers(display);
}

void
CXWindowsSecondaryScreen::updateKeys()
{
	CDisplayLock display(m_screen);

	// ask server which keys are pressed
	char keys[32];
	XQueryKeymap(display, keys);

	// transfer to our state
	for (UInt32 i = 0, j = 0; i < 32; j += 8, ++i) {
		m_keys[j + 0] = ((keys[i] & 0x01) != 0);
		m_keys[j + 1] = ((keys[i] & 0x02) != 0);
		m_keys[j + 2] = ((keys[i] & 0x04) != 0);
		m_keys[j + 3] = ((keys[i] & 0x08) != 0);
		m_keys[j + 4] = ((keys[i] & 0x10) != 0);
		m_keys[j + 5] = ((keys[i] & 0x20) != 0);
		m_keys[j + 6] = ((keys[i] & 0x40) != 0);
		m_keys[j + 7] = ((keys[i] & 0x80) != 0);
	}

	// we've fake pressed no keys
	m_fakeKeys.reset();

	// update mappings and current modifiers and mouse buttons
	doUpdateKeys(display);
}

void
CXWindowsSecondaryScreen::releaseKeys()
{
	CDisplayLock display(m_screen);
	if (display != NULL) {
		doReleaseKeys(display);
	}
}

void
CXWindowsSecondaryScreen::updateModifiers(Display* display)
{
	// query the pointer to get the keyboard state
	Window root, window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
	if (!XQueryPointer(display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state)) {
		state = 0;
	}

	// update active modifier mask
	m_mask = 0;
	for (unsigned int i = 0; i < 8; ++i) {
		const unsigned int bit = (1 << i);
		if ((bit & m_toggleModifierMask) == 0) {
			for (unsigned int j = 0; j < m_keysPerModifier; ++j) {
				if (m_keys[m_modifierToKeycodes[i * m_keysPerModifier + j]])
					m_mask |= bit;
			}
		}
		else if ((bit & state) != 0) {
			// toggle is on
			m_mask |= bit;
		}
	}
}

void
CXWindowsSecondaryScreen::updateKeycodeMap(Display* display)
{
	// there are up to 4 keysyms per keycode
	static const unsigned int maxKeysyms = 4;

	// table for counting 1 bits
	static const int s_numBits[maxKeysyms] = { 0, 1, 1, 2 };

	// get the number of keycodes
	int minKeycode, maxKeycode;
	XDisplayKeycodes(display, &minKeycode, &maxKeycode);
	const int numKeycodes = maxKeycode - minKeycode + 1;

	// get the keyboard mapping for all keys
	int keysymsPerKeycode;
	KeySym* keysyms = XGetKeyboardMapping(display,
								minKeycode, numKeycodes,
								&keysymsPerKeycode);

	// we only understand up to maxKeysyms keysyms per keycodes
	unsigned int numKeysyms = keysymsPerKeycode;
	if (numKeysyms > maxKeysyms) {
		numKeysyms = maxKeysyms;
	}

	// initialize
	m_keycodeMap.clear();

	// insert keys
	for (int i = 0; i < numKeycodes; ++i) {
		// compute mask over all mapped keysyms.  if a keycode has, say,
		// no shifted keysym then we can ignore the shift state when
		// synthesizing an event to generate it.
		unsigned int globalMask = 0;
		for (unsigned int j = 0; j < numKeysyms; ++j) {
			const KeySym keysym = keysyms[i * keysymsPerKeycode + j];
			if (keysym != NoSymbol) {
				globalMask |= j;
			}
		}

		// map each keysym to it's keycode/modifier mask
		for (unsigned int j = 0; j < numKeysyms; ++j) {
			// get keysym
			KeySym keysym = keysyms[i * keysymsPerKeycode + j];

			// get modifier mask required for this keysym.  note that
			// a keysym of NoSymbol means that a keysym using fewer
			// modifiers would be generated using these modifiers.
			// for example, given
			//  keycode 86 = KP_Add
			// then we'll generate KP_Add regardless of the modifiers.
			// we add an entry for that keysym for these modifiers.
			unsigned int index = j;
			if (keysym == NoSymbol && (index == 1 || index == 3)) {
				// shift doesn't matter
				index  = index - 1;
				keysym = keysyms[i * keysymsPerKeycode + index];
			}
			if (keysym == NoSymbol && index == 2) {
				// mode switch doesn't matter
				index  = 0;
				keysym = keysyms[i * keysymsPerKeycode + index];
			}
			if (keysym == NoSymbol && index == 0) {
				// no symbols at all for this keycode
				continue;
			}

			// look it up, creating a new entry if necessary
			KeyCodeMask& entry = m_keycodeMap[keysym];

			// save keycode for keysym and modifiers
			entry.m_keycode[j] = static_cast<KeyCode>(minKeycode + i);
		}
	}

	// clean up
	XFree(keysyms);
}

unsigned int
CXWindowsSecondaryScreen::indexToModifierMask(int index) const
{
	assert(index >= 0 && index <= 3);

	switch (index) {
	case 0:
		return 0;

	case 1:
		return ShiftMask | LockMask;

	case 2:
		return m_modeSwitchMask;

	case 3:
		return ShiftMask | LockMask | m_modeSwitchMask;
	}
}

void
CXWindowsSecondaryScreen::updateModifierMap(Display* display)
{
	// get modifier map from server
	XModifierKeymap* keymap = XGetModifierMapping(display);

	// initialize
	m_modifierMask       = 0;
	m_toggleModifierMask = 0;
	m_altMask            = 0;
	m_metaMask           = 0;
	m_superMask          = 0;
	m_modeSwitchMask     = 0;
	m_numLockMask        = 0;
	m_capsLockMask       = 0;
	m_scrollLockMask     = 0;
	m_keysPerModifier    = keymap->max_keypermod;
	m_modifierToKeycode.clear();
	m_modifierToKeycode.resize(8);
	m_modifierToKeycodes.clear();
	m_modifierToKeycodes.resize(8 * m_keysPerModifier);

	// set keycodes and masks
	for (unsigned int i = 0; i < 8; ++i) {
		const unsigned int bit = (1 << i);
		for (unsigned int j = 0; j < m_keysPerModifier; ++j) {
			KeyCode keycode = keymap->modifiermap[i * m_keysPerModifier + j];

			// save in modifier to keycode
			m_modifierToKeycodes[i * m_keysPerModifier + j] = keycode;

			// no further interest in unmapped modifier
			if (keycode == 0) {
				continue;
			}

			// save keycode for modifier if we don't have one yet
			if (m_modifierToKeycode[i] == 0) {
				m_modifierToKeycode[i] = keycode;
			}

			// save in keycode to modifier
			m_keycodeToModifier.insert(std::make_pair(keycode, i));

			// save bit in all-modifiers mask
			m_modifierMask |= bit;

			// modifier is a toggle if the keysym is a toggle modifier
			const KeySym keysym = XKeycodeToKeysym(display, keycode, 0);
			if (isToggleKeysym(keysym)) {
				m_toggleModifierMask |= bit;
			}

			// note mask for particular modifiers
			switch (keysym) {
			case XK_Alt_L:
			case XK_Alt_R:
				m_altMask        |= bit;
				break;

			case XK_Meta_L:
			case XK_Meta_R:
				m_metaMask       |= bit;
				break;

			case XK_Super_L:
			case XK_Super_R:
				m_superMask      |= bit;
				break;

			case XK_Mode_switch:
				m_modeSwitchMask |= bit;
				break;

			case XK_Num_Lock:
				m_numLockMask    |= bit;
				break;

			case XK_Caps_Lock:
				m_capsLockMask   |= bit;
				break;

			case XK_Scroll_Lock:
				m_scrollLockMask |= bit;
			}
		}
	}

	XFreeModifiermap(keymap);
}

void
CXWindowsSecondaryScreen::toggleKey(Display* display,
				KeySym keysym, unsigned int mask)
{
	// lookup the keycode
	KeyCodeMap::const_iterator index = m_keycodeMap.find(keysym);
	if (index == m_keycodeMap.end()) {
		return;
	}
	// FIXME -- which keycode?
	KeyCode keycode = index->second.m_keycode[0];

	// toggle the key
	if ((keysym == XK_Caps_Lock && m_capsLockHalfDuplex) ||
		(keysym == XK_Num_Lock && m_numLockHalfDuplex)) {
		// "half-duplex" toggle
		XTestFakeKeyEvent(display, keycode, (m_mask & mask) == 0, CurrentTime);
	}
	else {
		// normal toggle
		XTestFakeKeyEvent(display, keycode, True,  CurrentTime);
		XTestFakeKeyEvent(display, keycode, False, CurrentTime);
	}

	// toggle shadow state
	m_mask ^= mask;
}

bool
CXWindowsSecondaryScreen::isToggleKeysym(KeySym key)
{
	switch (key) {
	case XK_Caps_Lock:
	case XK_Shift_Lock:
	case XK_Num_Lock:
	case XK_Scroll_Lock:
		return true;

	default:
		return false;
	}
}

// map special KeyID keys to KeySyms
#if defined(HAVE_X11_XF86KEYSYM_H)
static const KeySym		g_mapE000[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa0 */ 0, 0, 0, 0,
	/* 0xa4 */ 0, 0,
	/* 0xa6 */ XF86XK_Back, XF86XK_Forward,
	/* 0xa8 */ XF86XK_Refresh, XF86XK_Stop,
	/* 0xaa */ XF86XK_Search, XF86XK_Favorites,
	/* 0xac */ XF86XK_HomePage, XF86XK_AudioMute,
	/* 0xae */ XF86XK_AudioLowerVolume, XF86XK_AudioRaiseVolume,
	/* 0xb0 */ XF86XK_AudioNext, XF86XK_AudioPrev,
	/* 0xb2 */ XF86XK_AudioStop, XF86XK_AudioPlay,
	/* 0xb4 */ XF86XK_Mail, XF86XK_AudioMedia,
	/* 0xb6 */ XF86XK_Launch0, XF86XK_Launch1,
	/* 0xb8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

CXWindowsSecondaryScreen::KeyCodeIndex
CXWindowsSecondaryScreen::findKey(KeyID id, KeyModifierMask& mask) const
{
	// convert id to keysym
	KeySym keysym = NoSymbol;
	if ((id & 0xfffff000) == 0xe000) {
		// special character
		switch (id & 0x0000ff00) {
#if defined(HAVE_X11_XF86KEYSYM_H)
		case 0xe000:
			keysym = g_mapE000[id & 0xff];
			break;
#endif

		case 0xee00:
			// ISO 9995 Function and Modifier Keys
			if (id == kKeyLeftTab) {
				keysym = XK_ISO_Left_Tab;
			}
			break;

		case 0xef00:
			// MISCELLANY
			keysym = static_cast<KeySym>(id - 0xef00 + 0xff00);
			break;
		}
	}
	else if ((id >= 0x0020 && id <= 0x007e) ||
			(id >= 0x00a0 && id <= 0x00ff)) {
		// Latin-1 maps directly
		keysym = static_cast<KeySym>(id);
	}
	else {
		// lookup keysym in table
		keysym = CXWindowsUtil::mapUCS4ToKeySym(id);
	}

	// fail if unknown key
	if (keysym == NoSymbol) {
		return m_keycodeMap.end();
	}

	// if kKeyTab is requested with shift active then try XK_ISO_Left_Tab
	// instead.  if that doesn't work, we'll fall back to XK_Tab with
	// shift active.  this is to handle primary screens that don't map
	// XK_ISO_Left_Tab sending events to secondary screens that do.
	if (keysym == XK_Tab && (mask & ShiftMask) != 0) {
		keysym = XK_ISO_Left_Tab;
		mask  &= ~ShiftMask;
	}

	// find the keycodes that generate the keysym
	KeyCodeIndex index = m_keycodeMap.find(keysym);
	if (index == noKey()) {
		// try upper/lower case (as some keymaps only include the
		// upper case, notably Sun Solaris).
		KeySym lower, upper;
		XConvertCase(keysym, &lower, &upper);
		if (lower != keysym)
			index = m_keycodeMap.find(lower);
		else if (upper != keysym)
			index = m_keycodeMap.find(upper);
	}
	if (index == noKey()) {
		// try backup keysym for certain keys (particularly the numpad
		// keys since most laptops don't have a separate numpad and the
		// numpad overlaying the main keyboard may not have movement
		// key bindings).
		switch (keysym) {
		case XK_KP_Home:
			keysym = XK_Home;
			break;

		case XK_KP_Left:
			keysym = XK_Left;
			break;

		case XK_KP_Up:
			keysym = XK_Up;
			break;

		case XK_KP_Right:
			keysym = XK_Right;
			break;

		case XK_KP_Down:
			keysym = XK_Down;
			break;

		case XK_KP_Prior:
			keysym = XK_Prior;
			break;

		case XK_KP_Next:
			keysym = XK_Next;
			break;

		case XK_KP_End:
			keysym = XK_End;
			break;

		case XK_KP_Insert:
			keysym = XK_Insert;
			break;

		case XK_KP_Delete:
			keysym = XK_Delete;
			break;

		case XK_ISO_Left_Tab:
			keysym = XK_Tab;
			mask  |= ShiftMask;
			break;

		default:
			return index;
		}

		index = m_keycodeMap.find(keysym);
	}

	return index;
}

CXWindowsSecondaryScreen::KeyCodeIndex
CXWindowsSecondaryScreen::noKey() const
{
	return m_keycodeMap.end();
}

bool
CXWindowsSecondaryScreen::adjustForNumLock(KeySym keysym) const
{
	if (IsKeypadKey(keysym) || IsPrivateKeypadKey(keysym)) {
		// it's NumLock sensitive
		LOG((CLOG_DEBUG2 "keypad key: NumLock %s", ((m_mask & m_numLockMask) != 0) ? "active" : "inactive"));
		return true;
	}
	return false;
}

bool
CXWindowsSecondaryScreen::adjustForCapsLock(KeySym keysym) const
{
	KeySym lKey, uKey;
	XConvertCase(keysym, &lKey, &uKey);
	if (lKey != uKey) {
		// it's CapsLock sensitive
		LOG((CLOG_DEBUG2 "case convertible: CapsLock %s", ((m_mask & m_capsLockMask) != 0) ? "active" : "inactive"));
		return true;
	}
	return false;
}


//
// CXWindowsSecondaryScreen::KeyCodeMask
//

CXWindowsSecondaryScreen::KeyCodeMask::KeyCodeMask()
{
	m_keycode[0] = 0;
	m_keycode[1] = 0;
	m_keycode[2] = 0;
	m_keycode[3] = 0;
}
