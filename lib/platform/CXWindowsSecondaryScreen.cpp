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
#	define XK_LATIN1
#	define XK_LATIN2
#	define XK_LATIN3
#	define XK_LATIN4
#	define XK_LATIN8
#	define XK_LATIN9
#	include <X11/keysymdef.h>
#	if defined(HAVE_X11_EXTENSIONS_XTEST_H)
#		include <X11/extensions/XTest.h>
#	else
#		error The XTest extension is required to build synergy
#	endif
#	if HAVE_X11_EXTENSIONS_XINERAMA_H
		// Xinerama.h may lack extern "C" for inclusion by C++
		extern "C" {
#		include <X11/extensions/Xinerama.h>
		}
#	endif
#	if defined(HAVE_X11_XF86KEYSYM_H)
#		include <X11/XF86keysym.h>
#	endif
#	if !defined(XF86XK_Launch0)
#		define XF86XK_Launch0 0x1008FF40
#	endif
#	if !defined(XF86XK_Launch1)
#		define XF86XK_Launch1 0x1008FF41
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

CXWindowsSecondaryScreen::KeySymsMap
						CXWindowsSecondaryScreen::s_decomposedKeySyms;

CXWindowsSecondaryScreen::CXWindowsSecondaryScreen(IScreenReceiver* receiver) :
	CSecondaryScreen(),
	m_window(None),
	m_xtestIsXineramaUnaware(true)
{
	m_screen = new CXWindowsScreen(receiver, this);

	// make sure decomposed keysym table is prepared
	getDecomposedKeySymTable();
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
	// check for ctrl+alt+del emulation
	if (key == kKeyDelete &&
		(mask & (KeyModifierControl | KeyModifierAlt)) ==
				(KeyModifierControl | KeyModifierAlt)) {
		LOG((CLOG_DEBUG "ctrl+alt+del emulation"));
		// just pass the key through
	}

	// get the sequence of keys to simulate key press and the final
	// modifier state.
	Keystrokes keys;
	KeyCode keycode;
	m_mask = mapKey(keys, keycode, key, mask, kPress);
	if (keys.empty()) {
		// do nothing if there are no associated keys (i.e. lookup failed)
		return;
	}

	// generate key events
	doKeystrokes(keys, 1);

	// do not record button down if button is 0 (invalid)
	if (button != 0) {
		// note that key is now down
		m_serverKeyMap[button] = keycode;
		m_keys[keycode]        = true;
		m_fakeKeys[keycode]    = true;
	}
}

void
CXWindowsSecondaryScreen::keyRepeat(KeyID key,
				KeyModifierMask mask, SInt32 count, KeyButton button)
{
	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}

	// get the sequence of keys to simulate key repeat and the final
	// modifier state.
	Keystrokes keys;
	KeyCode keycode;
	m_mask = mapKey(keys, keycode, key, mask, kRepeat);
	if (keys.empty()) {
		return;
	}

	// if this keycode shouldn't auto-repeat then ignore
	if ((m_keyControl.auto_repeats[keycode >> 3] & (1 << (keycode & 7))) == 0) {
		return;
	}

	// if the keycode for the auto-repeat is not the same as for the
	// initial press then mark the initial key as released and the new
	// key as pressed.  this can happen when we auto-repeat after a
	// dead key.  for example, a dead accent followed by 'a' will
	// generate an 'a with accent' followed by a repeating 'a'.  the
	// keycodes for the two keysyms might be different.
	if (keycode != index->second) {
		// replace key up with previous keycode but leave key down
		// alone so it uses the new keycode and store that keycode
		// in the server key map.  the key up is the first keystroke
		// with the keycode returned by mapKey().
		for (Keystrokes::iterator index2 = keys.begin();
								index2 != keys.end(); ++index2) {
			if (index2->m_keycode == keycode) {
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
	// if we haven't seen this button go down then ignore it
	ServerKeyMap::iterator index = m_serverKeyMap.find(button);
	if (index == m_serverKeyMap.end()) {
		return;
	}
	KeyCode keycode = index->second;

	// check for ctrl+alt+del emulation
	if (key == kKeyDelete &&
		(mask & (KeyModifierControl | KeyModifierAlt)) ==
				(KeyModifierControl | KeyModifierAlt)) {
		LOG((CLOG_DEBUG "ctrl+alt+del emulation"));
		// just pass the key through
	}

	// get the sequence of keys to simulate key release and the final
	// modifier state.
	Keystrokes keys;
	if (!((key == kKeyCapsLock && m_capsLockHalfDuplex) ||
		(key == kKeyNumLock && m_numLockHalfDuplex))) {
		m_mask = mapKeyRelease(keys, keycode);
	}

	// generate key events
	doKeystrokes(keys, 1);

	// note that key is now up
	m_serverKeyMap.erase(index);
	m_keys[keycode]     = false;
	m_fakeKeys[keycode] = false;
}

void
CXWindowsSecondaryScreen::flush(Display* display) const
{
	XFlush(display);
}

void
CXWindowsSecondaryScreen::mouseDown(ButtonID button)
{
	const unsigned int xButton = mapButton(button);
	if (xButton != 0) {
		CDisplayLock display(m_screen);
		XTestFakeButtonEvent(display, xButton, True, CurrentTime);
		flush(display);
	}
}

void
CXWindowsSecondaryScreen::mouseUp(ButtonID button)
{
	const unsigned int xButton = mapButton(button);
	if (xButton != 0) {
		CDisplayLock display(m_screen);
		XTestFakeButtonEvent(display, xButton, False, CurrentTime);
		flush(display);
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
	flush(display);
}

void
CXWindowsSecondaryScreen::resetOptions()
{
	m_numLockHalfDuplex      = false;
	m_capsLockHalfDuplex     = false;
	m_xtestIsXineramaUnaware = true;
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
		else if (options[i] == kOptionXTestXineramaUnaware) {
			m_xtestIsXineramaUnaware = (options[i + 1] != 0);
			LOG((CLOG_DEBUG1 "XTest is Xinerama unaware %s", m_xtestIsXineramaUnaware ? "true" : "false"));
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

	// check if xinerama is enabled and there is more than one screen
	m_xinerama = false;
#if HAVE_X11_EXTENSIONS_XINERAMA_H
	int eventBase, errorBase;
	if (XineramaQueryExtension(display, &eventBase, &errorBase)) {
		if (XineramaIsActive(display)) {
			int numScreens;
			XineramaScreenInfo* screens;
			screens = XineramaQueryScreens(display, &numScreens);
			if (screens != NULL) {
				m_xinerama = (numScreens > 1);
				XFree(screens);
			}
		}
	}
#endif
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
			flush(display);
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

	if (m_xinerama && m_xtestIsXineramaUnaware) {
		XWarpPointer(display, None, m_screen->getRoot(), 0, 0, 0, 0, x, y);
	}
	else {
		XTestFakeMotionEvent(display, DefaultScreen(pDisplay),
							x, y, CurrentTime);
	}
	flush(display);
}

void
CXWindowsSecondaryScreen::setToggleState(KeyModifierMask mask)
{
	CDisplayLock display(m_screen);

	// toggle modifiers that don't match the desired state
	ModifierMask xMask = maskToX(mask);
	if ((xMask & m_capsLockMask)   != (m_mask & m_capsLockMask)) {
		toggleKey(display, m_capsLockKeysym, m_capsLockMask);
	}
	if ((xMask & m_numLockMask)    != (m_mask & m_numLockMask)) {
		toggleKey(display, m_numLockKeysym, m_numLockMask);
	}
	if ((xMask & m_scrollLockMask) != (m_mask & m_scrollLockMask)) {
		toggleKey(display, m_scrollLockKeysym, m_scrollLockMask);
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

CXWindowsSecondaryScreen::ModifierMask
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

	// ignore releases and repeats for half-duplex keys
	const bool isHalfDuplex = ((id == kKeyCapsLock && m_capsLockHalfDuplex) ||
								(id == kKeyNumLock && m_numLockHalfDuplex));
	if (isHalfDuplex && action != kPress) {
		return m_mask;
	}

	// requested notes the modifiers requested by the server.
	ModifierMask requested = maskToX(mask);

	// convert KeyID to a KeySym
	KeySym keysym = keyIDToKeySym(id, requested);
	if (keysym == NoSymbol) {
		// unknown key
		LOG((CLOG_DEBUG2 "no keysym for id 0x%08x", id));
		return m_mask;
	}

	// get the mapping for this keysym
	KeySymIndex keyIndex = m_keysymMap.find(keysym);

	// if the mapping isn't found and keysym is caps lock sensitive
	// then convert the case of the keysym and try again.
	if (keyIndex == m_keysymMap.end()) {
		KeySym lKey, uKey;
		XConvertCase(keysym, &lKey, &uKey);
		if (lKey != uKey) {
			if (lKey == keysym) {
				keyIndex = m_keysymMap.find(uKey);
			}
			else {
				keyIndex = m_keysymMap.find(lKey);
			}
		}
	}

	if (keyIndex != m_keysymMap.end()) {
		// the keysym is mapped to some keycode.  if it's a modifier
		// and that modifier is already in the desired state then
		// ignore the request since there's nothing to do.  never
		// ignore a toggle modifier on press or release, though.
		const KeyMapping& keyMapping   = keyIndex->second;
		const ModifierMask modifierBit = keyMapping.m_modifierMask;
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

		// create the keystrokes for this keysym
		ModifierMask mask;
		if (!mapToKeystrokes(keys, keycode, mask, keyIndex, m_mask, action)) {
			// failed to generate keystrokes
			keys.clear();
			return m_mask;
		}
		else {
			// success
			LOG((CLOG_DEBUG2 "new mask: 0x%04x", mask));
			return mask;
		}
	}

	// we can't find the keysym mapped to any keycode.  this doesn't
	// necessarily mean we can't generate the keysym, though.  if the
	// keysym can be created by combining keysyms then we may still
	// be okay.
	KeySyms decomposition;
	if (decomposeKeySym(keysym, decomposition)) {
		LOG((CLOG_DEBUG2 "decomposed keysym 0x%08x into %d keysyms", keysym, decomposition.size()));

		// map each decomposed keysym to keystrokes.  we want the mask
		// and the keycode from the last keysym (which should be the
		// only non-dead key).  the dead keys are not sensitive to
		// anything but shift and mode switch.
		ModifierMask mask;
		for (KeySyms::const_iterator i  = decomposition.begin();
									 i != decomposition.end();) {
			// increment the iterator
			KeySyms::const_iterator next = i;
			++next;

			// lookup the key
			keysym   = *i;
			keyIndex = m_keysymMap.find(keysym);
			if (keyIndex == m_keysymMap.end()) {
				// missing a required keysym
				LOG((CLOG_DEBUG2 "no keycode for decomposed keysym 0x%08x", keysym));
				keys.clear();
				return m_mask;
			}

			// the keysym is mapped to some keycode
			if (!mapToKeystrokes(keys, keycode, mask,
								keyIndex, m_mask, action)) {
				// failed to generate keystrokes
				keys.clear();
				return m_mask;
			}

			// on to the next keysym
			i = next;
		}
		LOG((CLOG_DEBUG2 "new mask: 0x%04x", mask));
		return mask;
	}

	LOG((CLOG_DEBUG2 "no keycode for keysym"));
	return m_mask;
}

CXWindowsSecondaryScreen::ModifierMask
CXWindowsSecondaryScreen::mapKeyRelease(Keystrokes& keys, KeyCode keycode) const
{
	// add key release
	Keystroke keystroke;
	keystroke.m_keycode = keycode;
	keystroke.m_press   = False;
	keystroke.m_repeat  = false;
	keys.push_back(keystroke);

	// if this is a modifier keycode then update the current modifier mask
	KeyCodeToModifierMap::const_iterator i = m_keycodeToModifier.find(keycode);
	if (i != m_keycodeToModifier.end()) {
		ModifierMask bit = (1 << i->second);
		if (getBits(m_toggleModifierMask, bit) != 0) {
			// toggle keys modify the state on release
			return flipBits(m_mask, bit);
		}
		else {
			// can't reset bit until all keys that set it are released.
			// scan those keys to see if any (except keycode) are pressed.
			KeyCodes::const_iterator j;
			const KeyCodes& keycodes = m_modifierKeycodes[i->second];
			for (j = keycodes.begin(); j != keycodes.end(); ++j) {
				KeyCode modKeycode = *j;
				if (modKeycode != keycode && m_keys[modKeycode]) {
					break;
				}
			}
			if (j == keycodes.end()) {
				return clearBits(m_mask, bit);
			}
		}
	}

	return m_mask;
}

unsigned int
CXWindowsSecondaryScreen::findBestKeyIndex(KeySymIndex keyIndex,
				ModifierMask /*currentMask*/) const
{
	// there are up to 4 keycodes per keysym to choose from.  the
	// best choice is the one that requires the fewest adjustments
	// to the modifier state.  for example, the letter A normally
	// requires shift + a.  if shift isn't already down we'd have
	// to synthesize a shift press before the a press.  however,
	// if A could also be created with some other keycode without
	// shift then we'd prefer that when shift wasn't down.
	//
	// if the action is kRepeat or kRelease then we don't call this
	// method since we just need to synthesize a key repeat/release
	// on the same keycode that we pressed.
	// XXX -- do this right
	for (unsigned int i = 0; i < 4; ++i) {
		if (keyIndex->second.m_keycode[i] != 0) {
			return i;
		}
	}

	assert(0 && "no keycode found for keysym");
	return 0;
}

bool
CXWindowsSecondaryScreen::isShiftInverted(KeySymIndex keyIndex,
				ModifierMask currentMask) const
{
	// each keycode has up to 4 keysym associated with it, one each for:
	// no modifiers, shift, mode switch, and shift and mode switch.  if
	// a keysym is modified by num lock and num lock is active then you
	// get the shifted keysym when shift is not down and the unshifted
	// keysym when it is.  that is, num lock inverts the sense of the
	// shift modifier when active.  similarly for caps lock.  this
	// method returns true iff the sense of shift should be inverted
	// for this key given a modifier state.
	if (keyIndex->second.m_numLockSensitive) {
		if (getBits(currentMask, m_numLockMask) != 0) {
			return true;
		}
	}

	// if a keysym is num lock sensitive it is never caps lock
	// sensitive, thus the else here.
	else if (keyIndex->second.m_capsLockSensitive) {
		if (getBits(currentMask, m_capsLockMask) != 0) {
			return true;
		}
	}

	return false;
}

CXWindowsSecondaryScreen::ModifierMask
CXWindowsSecondaryScreen::getModifierMask(KeySym keysym) const
{
	// find the keysym mapping.  if it exists and there's a keycode
	// for index 0 (the index we use for modifiers) then return the
	// modifierMask, which might be 0.  otherwise return 0.
	KeySymIndex keyIndex = m_keysymMap.find(keysym);
	if (keyIndex != m_keysymMap.end() && keyIndex->second.m_keycode[0] != 0) {
		return keyIndex->second.m_modifierMask;
	}
	else {
		return 0;
	}
}

bool
CXWindowsSecondaryScreen::mapToKeystrokes(Keystrokes& keys,
				KeyCode& keycode,
				ModifierMask& finalMask,
				KeySymIndex keyIndex,
				ModifierMask currentMask,
				EKeyAction action) const
{
	// keyIndex must be valid
	assert(keyIndex != m_keysymMap.end());

	// get the keysym we're trying to generate and possible keycodes
	const KeySym keysym       = keyIndex->first;
	const KeyMapping& mapping = keyIndex->second;
	LOG((CLOG_DEBUG2 "keysym = 0x%08x", keysym));

	// get the best keycode index for the keysym and modifiers.  note
	// that (bestIndex & 1) == 0 if the keycode is a shift modifier
	// and (bestIndex & 2) == 0 if the keycode is a mode switch
	// modifier.  this is important later because we don't want
	// adjustModifiers() to adjust a modifier if that's the key we're
	// mapping.
	unsigned int bestIndex = findBestKeyIndex(keyIndex, currentMask);

	// get the keycode
	keycode = mapping.m_keycode[bestIndex];

	// flip low bit of bestIndex if shift is inverted.  if there's a
	// keycode for this new index then use it.  otherwise use the old
	// keycode.  you'd think we should fail if there isn't a keycode
	// for the new index but some keymaps only include the upper case
	// keysyms (notably those on Sun Solaris) so to handle the missing
	// lower case keysyms we just use the old keycode.  note that
	// isShiftInverted() will always return false for a shift modifier.
	if (isShiftInverted(keyIndex, currentMask)) {
		LOG((CLOG_DEBUG2 "shift is inverted"));
		bestIndex ^= 1;
		if (mapping.m_keycode[bestIndex] != 0) {
			keycode = mapping.m_keycode[bestIndex];
		}
	}
	LOG((CLOG_DEBUG2 "bestIndex = %d, keycode = %d", bestIndex, keycode));

	// compute desired mask.  the desired mask is the one that matches
	// bestIndex, except if the key being synthesized is a shift key
	// where we desire what we already have or if it's the mode switch
	// key where we only desire to adjust shift.  also, if the keycode
	// is not sensitive to shift then don't adjust it, otherwise
	// something like shift+home would become just home.  similiarly
	// for mode switch.
	ModifierMask desiredMask = currentMask;
	if (keyIndex->second.m_modifierMask != m_shiftMask) {
		if (keyIndex->second.m_shiftSensitive[bestIndex]) {
			if ((bestIndex & 1) != 0) {
				desiredMask = setBits(desiredMask, m_shiftMask);
			}
			else {
				desiredMask = clearBits(desiredMask, m_shiftMask);
			}
		}
		if (keyIndex->second.m_modifierMask != m_modeSwitchMask) {
			if (keyIndex->second.m_modeSwitchSensitive[bestIndex]) {
				if ((bestIndex & 2) != 0) {
					desiredMask = setBits(desiredMask, m_modeSwitchMask);
				}
				else {
					desiredMask = clearBits(desiredMask, m_modeSwitchMask);
				}
			}
		}
	}

	// adjust the modifiers to match the desired modifiers
	Keystrokes undo;
	ModifierMask tmpMask = currentMask;
	if (!adjustModifiers(keys, undo, tmpMask, desiredMask)) {
		LOG((CLOG_DEBUG2 "failed to adjust modifiers"));
		return false;
	}

	// note if the press of a half-duplex key should be treated as a release
	const bool isHalfDuplex =
				((keysym == m_capsLockKeysym && m_capsLockHalfDuplex) ||
				 (keysym == m_numLockKeysym  && m_numLockHalfDuplex));
	if (isHalfDuplex && getBits(currentMask, mapping.m_modifierMask) != 0) {
		action = kRelease;
	}

	// add the key event
	Keystroke keystroke;
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

	// put undo keystrokes at end of keystrokes in reverse order
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	// if the key is a modifier key then compute the modifier map after
	// this key is pressed or released.
	finalMask = currentMask;
	if (mapping.m_modifierMask != 0) {
		// can't be repeating if we've gotten here
		assert(action != kRepeat);

		// toggle keys modify the state on release.  other keys set the
		// bit on press and clear the bit on release.  if half-duplex
		// then toggle each time we get here.
		if (getBits(m_toggleModifierMask, mapping.m_modifierMask) != 0) {
			if (isHalfDuplex) {
				finalMask = flipBits(finalMask, mapping.m_modifierMask);
			}
		}
		else if (action == kPress) {
			finalMask = setBits(finalMask, mapping.m_modifierMask);
		}
	}

	return true;
}

bool
CXWindowsSecondaryScreen::adjustModifiers(Keystrokes& keys,
				Keystrokes& undo,
				ModifierMask& inOutMask,
				ModifierMask desiredMask) const
{
	// get mode switch set correctly.  do this before shift because
	// mode switch may be sensitive to the shift modifier and will
	// set/reset it as necessary.
	const bool wantModeSwitch = ((desiredMask & m_modeSwitchMask) != 0);
	const bool haveModeSwitch = ((inOutMask   & m_modeSwitchMask) != 0);
	if (wantModeSwitch != haveModeSwitch) {
		LOG((CLOG_DEBUG2 "fix mode switch"));

		// adjust shift if necessary
		KeySymIndex modeSwitchIndex = m_keysymMap.find(m_modeSwitchKeysym);
		assert(modeSwitchIndex != m_keysymMap.end());
		if (modeSwitchIndex->second.m_shiftSensitive[0]) {
			const bool wantShift = false;
			const bool haveShift = ((inOutMask & m_shiftMask) != 0);
			if (wantShift != haveShift) {
				// add shift keystrokes
				LOG((CLOG_DEBUG2 "fix shift for mode switch"));
				if (!adjustModifier(keys, undo, m_shiftKeysym, wantShift)) {
					return false;
				}
				inOutMask ^= m_shiftMask;
			}
		}

		// add mode switch keystrokes
		if (!adjustModifier(keys, undo, m_modeSwitchKeysym, wantModeSwitch)) {
			return false;
		}
		inOutMask ^= m_modeSwitchMask;
	}

	// get shift set correctly
	const bool wantShift = ((desiredMask & m_shiftMask) != 0);
	const bool haveShift = ((inOutMask   & m_shiftMask) != 0);
	if (wantShift != haveShift) {
		// add shift keystrokes
		LOG((CLOG_DEBUG2 "fix shift"));
		if (!adjustModifier(keys, undo, m_shiftKeysym, wantShift)) {
			return false;
		}
		inOutMask ^= m_shiftMask;
	}

	return true;
}

bool
CXWindowsSecondaryScreen::adjustModifier(Keystrokes& keys,
				Keystrokes& undo, KeySym keysym, bool desireActive) const
{
	// this method generates keystrokes to change a modifier into the
	// desired state.  under X11, we only expect to adjust the shift
	// and mode switch states.  other modifiers don't affect keysym
	// generation, except num lock and caps lock and we don't change
	// those but instead just invert the handling of the shift key.
	// we don't check here if the modifier is already in the desired
	// state;  the caller should do that.

	// get the key mapping for keysym
	KeySymIndex keyIndex = m_keysymMap.find(keysym);
	if (keyIndex == m_keysymMap.end() || keyIndex->second.m_keycode[0] == 0) {
		// no keycode for keysym or keycode is not a modifier
		LOG((CLOG_DEBUG2 "no modifier for 0x%08x", keysym));
		return false;
	}

	// this had better be a modifier
	assert(keyIndex->second.m_modifierMask != 0);

	// we do not handle toggle modifiers here.  they never need to be
	// adjusted
	assert((keyIndex->second.m_modifierMask & m_toggleModifierMask) == 0);

	// initialize keystroke
	Keystroke keystroke;
	keystroke.m_repeat  = false;

	// releasing a modifier is quite different from pressing one.
	// when we release a modifier we have to release every keycode that
	// is assigned to the modifier since the modifier is active if any
	// one of them is down.  when we press a modifier we just have to
	// press one of those keycodes.
	if (desireActive) {
		// press
		keystroke.m_keycode = keyIndex->second.m_keycode[0];
		keystroke.m_press   = True;
		keys.push_back(keystroke);
		keystroke.m_press   = False;
		undo.push_back(keystroke);
	}
	else {
		// release
		KeyCodeToModifierMap::const_iterator index =
			m_keycodeToModifier.find(keyIndex->second.m_keycode[0]);
		if (index != m_keycodeToModifier.end()) {
			const KeyCodes& keycodes = m_modifierKeycodes[index->second];
			for (KeyCodes::const_iterator j = keycodes.begin();
									j != keycodes.end(); ++j) {
				if (m_keys[*j]) {
					keystroke.m_keycode = *j;
					keystroke.m_press   = False;
					keys.push_back(keystroke);
					keystroke.m_press   = True;
					undo.push_back(keystroke);
				}
			}
		}
	}

	return true;
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
			LOG((CLOG_DEBUG2 "keystrokes:"));
			LOG((CLOG_DEBUG2 "  %d %s", k->m_keycode, k->m_press ? "down" : "up"));
			XTestFakeKeyEvent(display, k->m_keycode, k->m_press, CurrentTime);

			// next key
			++k;
		}
	}

	// update
	flush(display);
}

CXWindowsSecondaryScreen::ModifierMask
CXWindowsSecondaryScreen::maskToX(KeyModifierMask inMask) const
{
	ModifierMask outMask = 0;
	if (inMask & KeyModifierShift) {
		outMask |= m_shiftMask;
	}
	if (inMask & KeyModifierControl) {
		outMask |= m_ctrlMask;
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
	updateKeysymMap(display);
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
CXWindowsSecondaryScreen::updateKeysymMap(Display* display)
{
	// there are up to 4 keysyms per keycode
	static const unsigned int maxKeysyms = 4;

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

	// get modifier map from server
	XModifierKeymap* modifiers = XGetModifierMapping(display);

	// determine shift and mode switch sensitivity.  a keysym is shift
	// or mode switch sensitive if its keycode is.  a keycode is mode
	// mode switch sensitive if it has keysyms for indices 2 or 3.
	// it's shift sensitive if the keysym for index 1 (if any) is
	// different from the keysym for index 0 and, if the keysym for
	// for index 3 (if any) is different from the keysym for index 2.
	// that is, if shift changes the generated keysym for the keycode.
	std::vector<bool> usesShift(numKeycodes);
	std::vector<bool> usesModeSwitch(numKeycodes);
	for (int i = 0; i < numKeycodes; ++i) {
		// check mode switch first
		if (numKeysyms > 2 &&
			keysyms[i * keysymsPerKeycode + 2] != NoSymbol ||
			keysyms[i * keysymsPerKeycode + 3] != NoSymbol) {
			usesModeSwitch[i] = true;
		}

		// check index 0 with index 1 keysyms
		if (keysyms[i * keysymsPerKeycode + 0] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 1] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 1] !=
				keysyms[i * keysymsPerKeycode + 0]) {
			usesShift[i] = true;
		}

		else if (numKeysyms >= 4 &&
			keysyms[i * keysymsPerKeycode + 2] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 3] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 3] !=
				keysyms[i * keysymsPerKeycode + 2]) {
			usesShift[i] = true;
		}
	}

	// initialize
	m_keysymMap.clear();
	int keysPerModifier = modifiers->max_keypermod;

	// for each modifier keycode, get the index 0 keycode and add it to
	// the keysym map.  also collect all keycodes for each modifier.
	m_keycodeToModifier.clear();
	for (ModifierIndex i = 0; i < 8; ++i) {
		// start with no keycodes for this modifier
		m_modifierKeycodes[i].clear();

		// add each keycode for modifier
		for (unsigned int j = 0; j < keysPerModifier; ++j) {
			// get keycode and ignore unset keycodes
			KeyCode keycode = modifiers->modifiermap[i * keysPerModifier + j];
			if (keycode == 0) {
				continue;
			}

			// save keycode for modifier and modifier for keycode
			m_modifierKeycodes[i].push_back(keycode);
			m_keycodeToModifier[keycode] = i;

			// get keysym and get/create key mapping
			const int keycodeIndex = keycode - minKeycode;
			const KeySym keysym    = keysyms[keycodeIndex *
											keysymsPerKeycode + 0];
			KeyMapping& mapping    = m_keysymMap[keysym];

			// skip if we already have a keycode for this index
			if (mapping.m_keycode[0] != 0) {
				continue;
			}

			// fill in keysym info
			mapping.m_keycode[0]             = keycode;
			mapping.m_shiftSensitive[0]      = usesShift[keycodeIndex];
			mapping.m_modeSwitchSensitive[0] = usesModeSwitch[keycodeIndex];
			mapping.m_modifierMask           = (1 << i);
			mapping.m_capsLockSensitive      = false;
			mapping.m_numLockSensitive       = false;
		}
	}

	// create a convenient NoSymbol entry (if it doesn't exist yet).
	// sometimes it's useful to handle NoSymbol like a normal keysym.
	// remove any entry for NoSymbol.  that keysym doesn't count.
	{
		KeyMapping& mapping = m_keysymMap[NoSymbol];
		for (unsigned int i = 0; i < numKeysyms; ++i) {
			mapping.m_keycode[i]             = 0;
			mapping.m_shiftSensitive[i]      = false;
			mapping.m_modeSwitchSensitive[i] = false;
		}
		mapping.m_modifierMask      = 0;
		mapping.m_capsLockSensitive = false;
		mapping.m_numLockSensitive  = false;
	}

	// add each keysym to the map, unless we've already inserted a key
	// for that keysym index.
	for (int i = 0; i < numKeycodes; ++i) {
		for (unsigned int j = 0; j < numKeysyms; ++j) {
			// lookup keysym
			const KeySym keysym = keysyms[i * keysymsPerKeycode + j];
			if (keysym == NoSymbol) {
				continue;
			}
			KeyMapping& mapping = m_keysymMap[keysym];

			// skip if we already have a keycode for this index
			if (mapping.m_keycode[j] != 0) {
				continue;
			}

			// fill in keysym info
			if (mapping.m_keycode[0] == 0) {
				mapping.m_modifierMask       = 0;
			}
			mapping.m_keycode[j]             = static_cast<KeyCode>(
												minKeycode + i);
			mapping.m_shiftSensitive[j]      = usesShift[i];
			mapping.m_modeSwitchSensitive[j] = usesModeSwitch[i];
			mapping.m_numLockSensitive       = adjustForNumLock(keysym);
			mapping.m_capsLockSensitive      = adjustForCapsLock(keysym);
		}
	}

	// choose the keysym to use for each modifier.  if the modifier
	// isn't mapped then use NoSymbol.  if a modifier has both left
	// and right versions then (arbitrarily) prefer the left.  also
	// collect the available modifier bits.
	struct CModifierBitInfo {
	public:
		KeySym CXWindowsSecondaryScreen::*m_keysym;
		KeySym m_left;
		KeySym m_right;
	};
	static const CModifierBitInfo s_modifierBitTable[] = {
	{ &CXWindowsSecondaryScreen::m_shiftKeysym,   XK_Shift_L,   XK_Shift_R    },
	{ &CXWindowsSecondaryScreen::m_ctrlKeysym,    XK_Control_L, XK_Control_R  },
	{ &CXWindowsSecondaryScreen::m_altKeysym,     XK_Alt_L,     XK_Alt_R      },
	{ &CXWindowsSecondaryScreen::m_metaKeysym,    XK_Meta_L,    XK_Meta_R     },
	{ &CXWindowsSecondaryScreen::m_superKeysym,   XK_Super_L,   XK_Super_R    },
	{ &CXWindowsSecondaryScreen::m_modeSwitchKeysym, XK_Mode_switch, NoSymbol },
	{ &CXWindowsSecondaryScreen::m_numLockKeysym,    XK_Num_Lock,    NoSymbol },
	{ &CXWindowsSecondaryScreen::m_capsLockKeysym,   XK_Caps_Lock,   NoSymbol },
	{ &CXWindowsSecondaryScreen::m_scrollLockKeysym, XK_Scroll_Lock, NoSymbol }
	};
	m_modifierMask       = 0;
	m_toggleModifierMask = 0;
	for (size_t i = 0; i < sizeof(s_modifierBitTable) /
							sizeof(s_modifierBitTable[0]); ++i) {
		const CModifierBitInfo& info = s_modifierBitTable[i];

		// find available keysym
		KeySymIndex keyIndex = m_keysymMap.find(info.m_left);
		if (keyIndex == m_keysymMap.end() && info.m_right != NoSymbol) {
			keyIndex = m_keysymMap.find(info.m_right);
		}
		if (keyIndex                        != m_keysymMap.end() &&
			keyIndex->second.m_modifierMask != 0) {
			this->*(info.m_keysym) = keyIndex->first;
		}
		else {
			this->*(info.m_keysym) = NoSymbol;
			continue;
		}

		// add modifier bit
		m_modifierMask |= keyIndex->second.m_modifierMask;
		if (isToggleKeysym(this->*(info.m_keysym))) {
			m_toggleModifierMask |= keyIndex->second.m_modifierMask;
		}
	}

	// if there's no mode switch key mapped then remove all keycodes
	// that depend on it and no keycode can be mode switch sensitive.
	if (m_modeSwitchKeysym == NoSymbol) {
		LOG((CLOG_DEBUG2 "no mode switch in keymap"));
		for (KeySymMap::iterator i = m_keysymMap.begin();
								i != m_keysymMap.end(); ) {
			i->second.m_keycode[2]             = 0;
			i->second.m_keycode[3]             = 0;
			i->second.m_modeSwitchSensitive[0] = false;
			i->second.m_modeSwitchSensitive[1] = false;
			i->second.m_modeSwitchSensitive[2] = false;
			i->second.m_modeSwitchSensitive[3] = false;

			// if this keysym no has no keycodes then remove it
			// except for the NoSymbol keysym mapping.
			if (i->second.m_keycode[0] == 0 && i->second.m_keycode[1] == 0) {
				m_keysymMap.erase(i++);
			}
			else {
				++i;
			}
		}
	}

	// cache the bits for the modifier
	m_shiftMask      = getModifierMask(m_shiftKeysym);
	m_ctrlMask       = getModifierMask(m_ctrlKeysym);
	m_altMask        = getModifierMask(m_altKeysym);
	m_metaMask       = getModifierMask(m_metaKeysym);
	m_superMask      = getModifierMask(m_superKeysym);
	m_capsLockMask   = getModifierMask(m_capsLockKeysym);
	m_numLockMask    = getModifierMask(m_numLockKeysym);
	m_modeSwitchMask = getModifierMask(m_modeSwitchKeysym);
	m_scrollLockMask = getModifierMask(m_scrollLockKeysym);

	// clean up
	XFree(keysyms);
	XFreeModifiermap(modifiers);
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
	for (ModifierIndex i = 0; i < 8; ++i) {
		const ModifierMask bit = (1 << i);
		if ((bit & m_toggleModifierMask) == 0) {
			for (KeyCodes::const_iterator j = m_modifierKeycodes[i].begin();
								j != m_modifierKeycodes[i].end(); ++j) {
				if (m_keys[*j]) {
					m_mask |= bit;
					break;
				}
			}
		}
		else if ((bit & state) != 0) {
			// toggle is on
			m_mask |= bit;
		}
	}
}

void
CXWindowsSecondaryScreen::toggleKey(Display* display,
				KeySym keysym, ModifierMask mask)
{
	// lookup the key mapping
	KeySymIndex index = m_keysymMap.find(keysym);
	if (index == m_keysymMap.end()) {
		return;
	}
	KeyCode keycode = index->second.m_keycode[0];

	// toggle the key
	if ((keysym == m_capsLockKeysym && m_capsLockHalfDuplex) ||
		(keysym == m_numLockKeysym  && m_numLockHalfDuplex)) {
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

KeySym
CXWindowsSecondaryScreen::keyIDToKeySym(KeyID id, ModifierMask mask) const
{
	// convert id to keysym
	KeySym keysym = NoSymbol;
	if ((id & 0xfffff000) == 0xe000) {
		// special character
		switch (id & 0x0000ff00) {
#if defined(HAVE_X11_XF86KEYSYM_H)
		case 0xe000:
			return g_mapE000[id & 0xff];
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
		return static_cast<KeySym>(id);
	}
	else {
		// lookup keysym in table
		return CXWindowsUtil::mapUCS4ToKeySym(id);
	}

	// fail if unknown key
	if (keysym == NoSymbol) {
		return keysym;
	}

	// if kKeyTab is requested with shift active then try XK_ISO_Left_Tab
	// instead.  if that doesn't work, we'll fall back to XK_Tab with
	// shift active.  this is to handle primary screens that don't map
	// XK_ISO_Left_Tab sending events to secondary screens that do.
	if (keysym == XK_Tab && (mask & ShiftMask) != 0) {
		keysym = XK_ISO_Left_Tab;
	}

	// some keysyms have emergency backups (particularly the numpad
	// keys since most laptops don't have a separate numpad and the
	// numpad overlaying the main keyboard may not have movement
	// key bindings).  figure out the emergency backup.
	KeySym backupKeysym;
	switch (keysym) {
	case XK_KP_Home:
		backupKeysym = XK_Home;
		break;

	case XK_KP_Left:
		backupKeysym = XK_Left;
		break;

	case XK_KP_Up:
		backupKeysym = XK_Up;
		break;

	case XK_KP_Right:
		backupKeysym = XK_Right;
		break;

	case XK_KP_Down:
		backupKeysym = XK_Down;
		break;

	case XK_KP_Prior:
		backupKeysym = XK_Prior;
		break;

	case XK_KP_Next:
		backupKeysym = XK_Next;
		break;

	case XK_KP_End:
		backupKeysym = XK_End;
		break;

	case XK_KP_Insert:
		backupKeysym = XK_Insert;
		break;

	case XK_KP_Delete:
		backupKeysym = XK_Delete;
		break;

	case XK_ISO_Left_Tab:
		backupKeysym = XK_Tab;
		break;

	default:
		backupKeysym = keysym;
		break;
	}

	// see if the keysym is assigned to any keycode.  if not and the
	// backup keysym is then use the backup keysym.
	if (backupKeysym != keysym &&
		m_keysymMap.find(keysym)       == m_keysymMap.end() &&
		m_keysymMap.find(backupKeysym) != m_keysymMap.end()) {
		keysym = backupKeysym;
	}

	return keysym;
}

bool
CXWindowsSecondaryScreen::decomposeKeySym(KeySym keysym,
				KeySyms& decomposed) const
{
	// unfortunately, X11 doesn't appear to have any way of
	// decomposing a keysym into its component keysyms.  we'll
	// use a lookup table for certain character sets.
	const KeySymsMap& table      = getDecomposedKeySymTable();
	KeySymsMap::const_iterator i = table.find(keysym);
	if (i == table.end()) {
		return false;
	}
	decomposed = i->second;
	return true;
}

bool
CXWindowsSecondaryScreen::adjustForNumLock(KeySym keysym) const
{
	return (IsKeypadKey(keysym) || IsPrivateKeypadKey(keysym));
}

bool
CXWindowsSecondaryScreen::adjustForCapsLock(KeySym keysym) const
{
	KeySym lKey, uKey;
	XConvertCase(keysym, &lKey, &uKey);
	return (lKey != uKey);
}

const CXWindowsSecondaryScreen::KeySymsMap&
CXWindowsSecondaryScreen::getDecomposedKeySymTable()
{
	static const KeySym s_rawTable[] = {
		// non-dead version of dead keys
		XK_grave,        XK_dead_grave,       XK_space, 0,
		XK_acute,        XK_dead_acute,       XK_space, 0,
		XK_asciicircum,  XK_dead_circumflex,  XK_space, 0,
		XK_asciitilde,   XK_dead_tilde,       XK_space, 0,
		XK_cedilla,      XK_dead_cedilla,     XK_space, 0,
		XK_ogonek,       XK_dead_ogonek,      XK_space, 0,
		XK_caron,        XK_dead_caron,       XK_space, 0,
		XK_abovedot,     XK_dead_abovedot,    XK_space, 0,
		XK_doubleacute,  XK_dead_doubleacute, XK_space, 0,
		XK_breve,        XK_dead_breve,       XK_space, 0,
		XK_macron,       XK_dead_macron,      XK_space, 0,

		// Latin-1 (ISO 8859-1)
		XK_Agrave,       XK_dead_grave,       XK_A, 0,
		XK_Aacute,       XK_dead_acute,       XK_A, 0,
		XK_Acircumflex,  XK_dead_circumflex,  XK_A, 0,
		XK_Atilde,       XK_dead_tilde,       XK_A, 0,
		XK_Adiaeresis,   XK_dead_diaeresis,   XK_A, 0,
		XK_Aring,        XK_dead_abovering,   XK_A, 0,
		XK_Ccedilla,     XK_dead_cedilla,     XK_C, 0,
		XK_Egrave,       XK_dead_grave,       XK_E, 0,
		XK_Eacute,       XK_dead_acute,       XK_E, 0,
		XK_Ecircumflex,  XK_dead_circumflex,  XK_E, 0,
		XK_Ediaeresis,   XK_dead_diaeresis,   XK_E, 0,
		XK_Igrave,       XK_dead_grave,       XK_I, 0,
		XK_Iacute,       XK_dead_acute,       XK_I, 0,
		XK_Icircumflex,  XK_dead_circumflex,  XK_I, 0,
		XK_Idiaeresis,   XK_dead_diaeresis,   XK_I, 0,
		XK_Ntilde,       XK_dead_tilde,       XK_N, 0,
		XK_Ograve,       XK_dead_grave,       XK_O, 0,
		XK_Oacute,       XK_dead_acute,       XK_O, 0,
		XK_Ocircumflex,  XK_dead_circumflex,  XK_O, 0,
		XK_Otilde,       XK_dead_tilde,       XK_O, 0,
		XK_Odiaeresis,   XK_dead_diaeresis,   XK_O, 0,
		XK_Ugrave,       XK_dead_grave,       XK_U, 0,
		XK_Uacute,       XK_dead_acute,       XK_U, 0,
		XK_Ucircumflex,  XK_dead_circumflex,  XK_U, 0,
		XK_Udiaeresis,   XK_dead_diaeresis,   XK_U, 0,
		XK_Yacute,       XK_dead_acute,       XK_Y, 0,
		XK_agrave,       XK_dead_grave,       XK_a, 0,
		XK_aacute,       XK_dead_acute,       XK_a, 0,
		XK_acircumflex,  XK_dead_circumflex,  XK_a, 0,
		XK_atilde,       XK_dead_tilde,       XK_a, 0,
		XK_adiaeresis,   XK_dead_diaeresis,   XK_a, 0,
		XK_aring,        XK_dead_abovering,   XK_a, 0,
		XK_ccedilla,     XK_dead_cedilla,     XK_c, 0,
		XK_egrave,       XK_dead_grave,       XK_e, 0,
		XK_eacute,       XK_dead_acute,       XK_e, 0,
		XK_ecircumflex,  XK_dead_circumflex,  XK_e, 0,
		XK_ediaeresis,   XK_dead_diaeresis,   XK_e, 0,
		XK_igrave,       XK_dead_grave,       XK_i, 0,
		XK_iacute,       XK_dead_acute,       XK_i, 0,
		XK_icircumflex,  XK_dead_circumflex,  XK_i, 0,
		XK_idiaeresis,   XK_dead_diaeresis,   XK_i, 0,
		XK_ntilde,       XK_dead_tilde,       XK_n, 0,
		XK_ograve,       XK_dead_grave,       XK_o, 0,
		XK_oacute,       XK_dead_acute,       XK_o, 0,
		XK_ocircumflex,  XK_dead_circumflex,  XK_o, 0,
		XK_otilde,       XK_dead_tilde,       XK_o, 0,
		XK_odiaeresis,   XK_dead_diaeresis,   XK_o, 0,
		XK_ugrave,       XK_dead_grave,       XK_u, 0,
		XK_uacute,       XK_dead_acute,       XK_u, 0,
		XK_ucircumflex,  XK_dead_circumflex,  XK_u, 0,
		XK_udiaeresis,   XK_dead_diaeresis,   XK_u, 0,
		XK_yacute,       XK_dead_acute,       XK_y, 0,
		XK_ydiaeresis,   XK_dead_diaeresis,   XK_y, 0,

		// Latin-2 (ISO 8859-2)
		XK_Aogonek,      XK_dead_ogonek,      XK_A, 0,
		XK_Lcaron,       XK_dead_caron,       XK_L, 0,
		XK_Sacute,       XK_dead_acute,       XK_S, 0,
		XK_Scaron,       XK_dead_caron,       XK_S, 0,
		XK_Scedilla,     XK_dead_cedilla,     XK_S, 0,
		XK_Tcaron,       XK_dead_caron,       XK_T, 0,
		XK_Zacute,       XK_dead_acute,       XK_Z, 0,
		XK_Zcaron,       XK_dead_caron,       XK_Z, 0,
		XK_Zabovedot,    XK_dead_abovedot,    XK_Z, 0,
		XK_aogonek,      XK_dead_ogonek,      XK_a, 0,
		XK_lcaron,       XK_dead_caron,       XK_l, 0,
		XK_sacute,       XK_dead_acute,       XK_s, 0,
		XK_scaron,       XK_dead_caron,       XK_s, 0,
		XK_scedilla,     XK_dead_cedilla,     XK_s, 0,
		XK_tcaron,       XK_dead_caron,       XK_t, 0,
		XK_zacute,       XK_dead_acute,       XK_z, 0,
		XK_zcaron,       XK_dead_caron,       XK_z, 0,
		XK_zabovedot,    XK_dead_abovedot,    XK_z, 0,
		XK_Racute,       XK_dead_acute,       XK_R, 0,
		XK_Abreve,       XK_dead_breve,       XK_A, 0,
		XK_Lacute,       XK_dead_acute,       XK_L, 0,
		XK_Cacute,       XK_dead_acute,       XK_C, 0,
		XK_Ccaron,       XK_dead_caron,       XK_C, 0,
		XK_Eogonek,      XK_dead_ogonek,      XK_E, 0,
		XK_Ecaron,       XK_dead_caron,       XK_E, 0,
		XK_Dcaron,       XK_dead_caron,       XK_D, 0,
		XK_Nacute,       XK_dead_acute,       XK_N, 0,
		XK_Ncaron,       XK_dead_caron,       XK_N, 0,
		XK_Odoubleacute, XK_dead_doubleacute, XK_O, 0,
		XK_Rcaron,       XK_dead_caron,       XK_R, 0,
		XK_Uring,        XK_dead_abovering,   XK_U, 0,
		XK_Udoubleacute, XK_dead_doubleacute, XK_U, 0,
		XK_Tcedilla,     XK_dead_cedilla,     XK_T, 0,
		XK_racute,       XK_dead_acute,       XK_r, 0,
		XK_abreve,       XK_dead_breve,       XK_a, 0,
		XK_lacute,       XK_dead_acute,       XK_l, 0,
		XK_cacute,       XK_dead_acute,       XK_c, 0,
		XK_ccaron,       XK_dead_caron,       XK_c, 0,
		XK_eogonek,      XK_dead_ogonek,      XK_e, 0,
		XK_ecaron,       XK_dead_caron,       XK_e, 0,
		XK_dcaron,       XK_dead_caron,       XK_d, 0,
		XK_nacute,       XK_dead_acute,       XK_n, 0,
		XK_ncaron,       XK_dead_caron,       XK_n, 0,
		XK_odoubleacute, XK_dead_doubleacute, XK_o, 0,
		XK_rcaron,       XK_dead_caron,       XK_r, 0,
		XK_uring,        XK_dead_abovering,   XK_u, 0,
		XK_udoubleacute, XK_dead_doubleacute, XK_u, 0,
		XK_tcedilla,     XK_dead_cedilla,     XK_t, 0,

		// Latin-3 (ISO 8859-3)
		XK_Hcircumflex,  XK_dead_circumflex,  XK_H, 0,
		XK_Iabovedot,    XK_dead_abovedot,    XK_I, 0,
		XK_Gbreve,       XK_dead_breve,       XK_G, 0,
		XK_Jcircumflex,  XK_dead_circumflex,  XK_J, 0,
		XK_hcircumflex,  XK_dead_circumflex,  XK_h, 0,
		XK_gbreve,       XK_dead_breve,       XK_g, 0,
		XK_jcircumflex,  XK_dead_circumflex,  XK_j, 0,
		XK_Cabovedot,    XK_dead_abovedot,    XK_C, 0,
		XK_Ccircumflex,  XK_dead_circumflex,  XK_C, 0,
		XK_Gabovedot,    XK_dead_abovedot,    XK_G, 0,
		XK_Gcircumflex,  XK_dead_circumflex,  XK_G, 0,
		XK_Ubreve,       XK_dead_breve,       XK_U, 0,
		XK_Scircumflex,  XK_dead_circumflex,  XK_S, 0,
		XK_cabovedot,    XK_dead_abovedot,    XK_c, 0,
		XK_ccircumflex,  XK_dead_circumflex,  XK_c, 0,
		XK_gabovedot,    XK_dead_abovedot,    XK_g, 0,
		XK_gcircumflex,  XK_dead_circumflex,  XK_g, 0,
		XK_ubreve,       XK_dead_breve,       XK_u, 0,
		XK_scircumflex,  XK_dead_circumflex,  XK_s, 0,

		// Latin-4 (ISO 8859-4)
		XK_scircumflex,  XK_dead_circumflex,  XK_s, 0,
		XK_Rcedilla,     XK_dead_cedilla,     XK_R, 0,
		XK_Itilde,       XK_dead_tilde,       XK_I, 0,
		XK_Lcedilla,     XK_dead_cedilla,     XK_L, 0,
		XK_Emacron,      XK_dead_macron,      XK_E, 0,
		XK_Gcedilla,     XK_dead_cedilla,     XK_G, 0,
		XK_rcedilla,     XK_dead_cedilla,     XK_r, 0,
		XK_itilde,       XK_dead_tilde,       XK_i, 0,
		XK_lcedilla,     XK_dead_cedilla,     XK_l, 0,
		XK_emacron,      XK_dead_macron,      XK_e, 0,
		XK_gcedilla,     XK_dead_cedilla,     XK_g, 0,
		XK_Amacron,      XK_dead_macron,      XK_A, 0,
		XK_Iogonek,      XK_dead_ogonek,      XK_I, 0,
		XK_Eabovedot,    XK_dead_abovedot,    XK_E, 0,
		XK_Imacron,      XK_dead_macron,      XK_I, 0,
		XK_Ncedilla,     XK_dead_cedilla,     XK_N, 0,
		XK_Omacron,      XK_dead_macron,      XK_O, 0,
		XK_Kcedilla,     XK_dead_cedilla,     XK_K, 0,
		XK_Uogonek,      XK_dead_ogonek,      XK_U, 0,
		XK_Utilde,       XK_dead_tilde,       XK_U, 0,
		XK_Umacron,      XK_dead_macron,      XK_U, 0,
		XK_amacron,      XK_dead_macron,      XK_a, 0,
		XK_iogonek,      XK_dead_ogonek,      XK_i, 0,
		XK_eabovedot,    XK_dead_abovedot,    XK_e, 0,
		XK_imacron,      XK_dead_macron,      XK_i, 0,
		XK_ncedilla,     XK_dead_cedilla,     XK_n, 0,
		XK_omacron,      XK_dead_macron,      XK_o, 0,
		XK_kcedilla,     XK_dead_cedilla,     XK_k, 0,
		XK_uogonek,      XK_dead_ogonek,      XK_u, 0,
		XK_utilde,       XK_dead_tilde,       XK_u, 0,
		XK_umacron,      XK_dead_macron,      XK_u, 0,

		// Latin-8 (ISO 8859-14)
		XK_Babovedot,    XK_dead_abovedot,    XK_B, 0,
		XK_babovedot,    XK_dead_abovedot,    XK_b, 0,
		XK_Dabovedot,    XK_dead_abovedot,    XK_D, 0,
		XK_Wgrave,       XK_dead_grave,       XK_W, 0,
		XK_Wacute,       XK_dead_acute,       XK_W, 0,
		XK_dabovedot,    XK_dead_abovedot,    XK_d, 0,
		XK_Ygrave,       XK_dead_grave,       XK_Y, 0,
		XK_Fabovedot,    XK_dead_abovedot,    XK_F, 0,
		XK_fabovedot,    XK_dead_abovedot,    XK_f, 0,
		XK_Mabovedot,    XK_dead_abovedot,    XK_M, 0,
		XK_mabovedot,    XK_dead_abovedot,    XK_m, 0,
		XK_Pabovedot,    XK_dead_abovedot,    XK_P, 0,
		XK_wgrave,       XK_dead_grave,       XK_w, 0,
		XK_pabovedot,    XK_dead_abovedot,    XK_p, 0,
		XK_wacute,       XK_dead_acute,       XK_w, 0,
		XK_Sabovedot,    XK_dead_abovedot,    XK_S, 0,
		XK_ygrave,       XK_dead_grave,       XK_y, 0,
		XK_Wdiaeresis,   XK_dead_diaeresis,   XK_W, 0,
		XK_wdiaeresis,   XK_dead_diaeresis,   XK_w, 0,
		XK_sabovedot,    XK_dead_abovedot,    XK_s, 0,
		XK_Wcircumflex,  XK_dead_circumflex,  XK_W, 0,
		XK_Tabovedot,    XK_dead_abovedot,    XK_T, 0,
		XK_Ycircumflex,  XK_dead_circumflex,  XK_Y, 0,
		XK_wcircumflex,  XK_dead_circumflex,  XK_w, 0,
		XK_tabovedot,    XK_dead_abovedot,    XK_t, 0,
		XK_ycircumflex,  XK_dead_circumflex,  XK_y, 0,

		// Latin-9 (ISO 8859-15)
		XK_Ydiaeresis,   XK_dead_diaeresis,   XK_Y, 0,

		// end of table
		0
	};

	// fill table if not yet initialized
	if (s_decomposedKeySyms.empty()) {
		const KeySym* scan = s_rawTable;
		while (*scan != 0) {
			// add an entry for this keysym
			KeySyms& entry = s_decomposedKeySyms[*scan];

			// add the decomposed keysyms for the keysym
			while (*++scan != 0) {
				entry.push_back(*scan);
			}

			// skip end of entry marker
			++scan;
		}
	}

	return s_decomposedKeySyms;
}


//
// CXWindowsSecondaryScreen::KeyMapping
//

CXWindowsSecondaryScreen::KeyMapping::KeyMapping()
{
	m_keycode[0] = 0;
	m_keycode[1] = 0;
	m_keycode[2] = 0;
	m_keycode[3] = 0;
}
