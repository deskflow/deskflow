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

#include "CXWindowsPrimaryScreen.h"
#include "CXWindowsScreen.h"
#include "CXWindowsUtil.h"
#include "IPrimaryScreenReceiver.h"
#include "XScreen.h"
#include "CThread.h"
#include "CLog.h"
#include "CStopwatch.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/X.h>
#	include <X11/Xutil.h>
#	define XK_MISCELLANY
#	define XK_XKB_KEYS
#	include <X11/keysymdef.h>
#endif

//
// CXWindowsPrimaryScreen
//

CXWindowsPrimaryScreen::CXWindowsPrimaryScreen(
				IScreenReceiver* receiver,
				IPrimaryScreenReceiver* primaryReceiver) :
	CPrimaryScreen(receiver),
	m_receiver(primaryReceiver),
	m_window(None)
{
	m_screen = new CXWindowsScreen(receiver, this);
}

CXWindowsPrimaryScreen::~CXWindowsPrimaryScreen()
{
	assert(m_window == None);
	delete m_screen;
}

void
CXWindowsPrimaryScreen::reconfigure(UInt32)
{
	// do nothing
}

void
CXWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	CDisplayLock display(m_screen);

	// warp mouse
	warpCursorNoFlush(display, x, y);

	// remove all input events before and including warp
	XEvent event;
	while (XCheckMaskEvent(display, PointerMotionMask |
								ButtonPressMask | ButtonReleaseMask |
								KeyPressMask | KeyReleaseMask |
								KeymapStateMask,
								&event)) {
		// do nothing
	}

	// save position as last position
	m_x = x;
	m_y = y;
}

KeyModifierMask
CXWindowsPrimaryScreen::getToggleMask() const
{
	CDisplayLock display(m_screen);

	// query the pointer to get the keyboard state
	Window root, window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
	if (!XQueryPointer(display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state)) {
		return 0;
	}

	// convert to KeyModifierMask
	KeyModifierMask mask = 0;
	if (state & m_numLockMask) {
		mask |= KeyModifierNumLock;
	}
	if (state & m_capsLockMask) {
		mask |= KeyModifierCapsLock;
	}
	if (state & m_scrollLockMask) {
		mask |= KeyModifierScrollLock;
	}

	return mask;
}

bool
CXWindowsPrimaryScreen::isLockedToScreen() const
{
	CDisplayLock display(m_screen);

	// query the pointer to get the button state
	Window root, window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
	if (XQueryPointer(display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state)) {
		if ((state & (Button1Mask | Button2Mask | Button3Mask |
								Button4Mask | Button5Mask)) != 0) {
			return true;
		}
	}

	// get logical keyboard state
	char keyMap[32];
	memset(keyMap, 0, sizeof(keyMap));
	XQueryKeymap(display, keyMap);

	// locked if any key is down
	for (unsigned int i = 0; i < sizeof(keyMap); ++i) {
		if (keyMap[i] != 0) {
			return true;
		}
	}

	// not locked
	return false;
}

IScreen*
CXWindowsPrimaryScreen::getScreen() const
{
	return m_screen;
}

void
CXWindowsPrimaryScreen::onScreensaver(bool activated)
{
	m_receiver->onScreensaver(activated);
}

bool
CXWindowsPrimaryScreen::onPreDispatch(const CEvent*)
{
	return false;
}

bool
CXWindowsPrimaryScreen::onEvent(CEvent* event)
{
	assert(event != NULL);
	XEvent& xevent = event->m_event;

	// handle event
	switch (xevent.type) {
	case CreateNotify:
		{
			// select events on new window
			CDisplayLock display(m_screen);
			selectEvents(display, xevent.xcreatewindow.window);
		}
		return true;

	case MappingNotify:
		// keyboard mapping changed
		updateKeys();
		return true;

	case KeyPress:
		{
			log((CLOG_DEBUG1 "event: KeyPress code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
			const KeyModifierMask mask = mapModifier(xevent.xkey.state);
			const KeyID key = mapKey(&xevent.xkey);
			if (key != kKeyNone) {
				m_receiver->onKeyDown(key, mask);
				if (key == kKeyCapsLock && m_capsLockHalfDuplex) {
					m_receiver->onKeyUp(key, mask | KeyModifierCapsLock);
				}
				else if (key == kKeyNumLock && m_numLockHalfDuplex) {
					m_receiver->onKeyUp(key, mask | KeyModifierNumLock);
				}
			}
		}
		return true;

	case KeyRelease:
		{
			const KeyModifierMask mask = mapModifier(xevent.xkey.state);
			const KeyID key = mapKey(&xevent.xkey);
			if (key != kKeyNone) {
				// check if this is a key repeat by getting the next
				// KeyPress event that has the same key and time as
				// this release event, if any.  first prepare the
				// filter info.
				CKeyEventInfo filter;
				filter.m_event   = KeyPress;
				filter.m_window  = xevent.xkey.window;
				filter.m_time    = xevent.xkey.time;
				filter.m_keycode = xevent.xkey.keycode;

				// now check for event
				bool hasPress;
				{
					XEvent xevent2;
					CDisplayLock display(m_screen);
					hasPress = (XCheckIfEvent(display, &xevent2,
									&CXWindowsPrimaryScreen::findKeyEvent,
									(XPointer)&filter) == True);
				}
				if (!hasPress) {
					// no press event follows so it's a plain release
					log((CLOG_DEBUG1 "event: KeyRelease code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
					if (key == kKeyCapsLock && m_capsLockHalfDuplex) {
						m_receiver->onKeyDown(key, mask);
					}
					else if (key == kKeyNumLock && m_numLockHalfDuplex) {
						m_receiver->onKeyDown(key, mask);
					}
					m_receiver->onKeyUp(key, mask);
				}
				else {
					// found a press event following so it's a repeat.
					// we could attempt to count the already queued
					// repeats but we'll just send a repeat of 1.
					// note that we discard the press event.
					log((CLOG_DEBUG1 "event: repeat code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
					m_receiver->onKeyRepeat(key, mask, 1);
				}
			}
		}
		return true;

	case ButtonPress:
		{
			log((CLOG_DEBUG1 "event: ButtonPress button=%d", xevent.xbutton.button));
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_receiver->onMouseDown(button);
			}
		}
		return true;

	case ButtonRelease:
		{
			log((CLOG_DEBUG1 "event: ButtonRelease button=%d", xevent.xbutton.button));
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_receiver->onMouseUp(button);
			}
			else if (xevent.xbutton.button == 4) {
				// wheel forward (away from user)
				m_receiver->onMouseWheel(120);
			}
			else if (xevent.xbutton.button == 5) {
				// wheel backward (toward user)
				m_receiver->onMouseWheel(-120);
			}
		}
		return true;

	case MotionNotify:
		{
			log((CLOG_DEBUG2 "event: MotionNotify %d,%d", xevent.xmotion.x_root, xevent.xmotion.y_root));

			// compute motion delta (relative to the last known
			// mouse position)
			SInt32 x = xevent.xmotion.x_root - m_x;
			SInt32 y = xevent.xmotion.y_root - m_y;

			// save position to compute delta of next motion
			m_x = xevent.xmotion.x_root;
			m_y = xevent.xmotion.y_root;

			if (xevent.xmotion.send_event) {
				// we warped the mouse.  discard events until we
				// find the matching sent event.  see
				// warpCursorNoFlush() for where the events are
				// sent.  we discard the matching sent event and
				// can be sure we've skipped the warp event.
				CDisplayLock display(m_screen);
				do {
					XMaskEvent(display, PointerMotionMask, &xevent);
				} while (!xevent.xmotion.send_event);
			}
			else if (!isActive()) {
				// motion on primary screen
				m_receiver->onMouseMovePrimary(m_x, m_y);
			}
			else {
				// motion on secondary screen.  warp mouse back to
				// center.
				//
				// my lombard (powerbook g3) running linux and
				// using the adbmouse driver has two problems:
				// first, the driver only sends motions of +/-2
				// pixels and, second, it seems to discard some
				// physical input after a warp.  the former isn't a
				// big deal (we're just limited to every other
				// pixel) but the latter is a PITA.  to work around
				// it we only warp when the mouse has moved more
				// than s_size pixels from the center.
				static const SInt32 s_size = 32;
				if (xevent.xmotion.x_root - m_xCenter < -s_size ||
					xevent.xmotion.x_root - m_xCenter >  s_size ||
					xevent.xmotion.y_root - m_yCenter < -s_size ||
					xevent.xmotion.y_root - m_yCenter >  s_size) {
					CDisplayLock display(m_screen);
					warpCursorNoFlush(display, m_xCenter, m_yCenter);
				}

				// send event if mouse moved.  do this after warping
				// back to center in case the motion takes us onto
				// the primary screen.  if we sent the event first
				// in that case then the warp would happen after
				// warping to the primary screen's enter position,
				// effectively overriding it.
				if (x != 0 || y != 0) {
					m_receiver->onMouseMoveSecondary(x, y);
				}
			}
		}
		return true;
	}

	return false;
}

SInt32
CXWindowsPrimaryScreen::getJumpZoneSize() const
{
	return 1;
}

void
CXWindowsPrimaryScreen::onPreMainLoop()
{
	assert(m_window != None);
}

void
CXWindowsPrimaryScreen::onPreOpen()
{
	assert(m_window == None);
}

void
CXWindowsPrimaryScreen::onPostOpen()
{
	// get cursor info
	m_screen->getCursorPos(m_x, m_y);
	m_screen->getCursorCenter(m_xCenter, m_yCenter);

	// check for peculiarities
	// FIXME -- may have to get these from some database
	m_numLockHalfDuplex  = false;
	m_capsLockHalfDuplex = false;
//	m_numLockHalfDuplex  = true;
//	m_capsLockHalfDuplex = true;
}

void
CXWindowsPrimaryScreen::onPreEnter()
{
	assert(m_window != None);
}

void
CXWindowsPrimaryScreen::onPreLeave()
{
	assert(m_window != None);
}

void
CXWindowsPrimaryScreen::onEnterScreenSaver()
{
	CDisplayLock display(m_screen);

	// set keyboard focus to root window.  the screensaver should then
	// pick up key events for when the user enters a password to unlock. 
	XSetInputFocus(display, PointerRoot, PointerRoot, CurrentTime);
}

void
CXWindowsPrimaryScreen::createWindow()
{
	assert(m_window == None);

	// get size of screen
	SInt32 x, y, w, h;
	m_screen->getShape(x, y, w, h);

	// grab window attributes.  this window is used to capture user
	// input when the user is focused on another client.  don't let
	// the window manager mess with it.
	XSetWindowAttributes attr;
	attr.event_mask            = PointerMotionMask |
								 ButtonPressMask | ButtonReleaseMask |
								 KeyPressMask | KeyReleaseMask |
								 KeymapStateMask | PropertyChangeMask;
	attr.do_not_propagate_mask = 0;
	attr.override_redirect     = True;
	attr.cursor                = m_screen->getBlankCursor();

	{
		// create the grab window
		CDisplayLock display(m_screen);
		m_window = XCreateWindow(display, m_screen->getRoot(),
								x, y, w, h, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);
		if (m_window == None) {
			throw XScreenOpenFailure();
		}
		log((CLOG_DEBUG "window is 0x%08x", m_window));

		// start watching for events on other windows
		selectEvents(display, m_screen->getRoot());
	}

	// tell generic screen about the window
	m_screen->setWindow(m_window);
}

void
CXWindowsPrimaryScreen::destroyWindow()
{
	// display can be NULL if the server unexpectedly disconnected
	if (m_window != None) {
		m_screen->setWindow(None);
		CDisplayLock display(m_screen);
		if (display != NULL) {
			XDestroyWindow(display, m_window);
		}
		m_window = None;
	}
}

bool
CXWindowsPrimaryScreen::showWindow()
{
	assert(m_window != None);

	CDisplayLock display(m_screen);

	// raise and show the input window
	XMapRaised(display, m_window);

	// grab the mouse and keyboard.  keep trying until we get them.
	// if we can't grab one after grabbing the other then ungrab
	// and wait before retrying.  give up after s_timeout seconds.
	static const double s_timeout = 1.0;
	int result;
	CStopwatch timer;
	do {
		// keyboard first
		do {
			result = XGrabKeyboard(display, m_window, True,
								GrabModeAsync, GrabModeAsync, CurrentTime);
			assert(result != GrabNotViewable);
			if (result != GrabSuccess) {
				log((CLOG_DEBUG2 "waiting to grab keyboard"));
				CThread::sleep(0.05);
				if (timer.getTime() >= s_timeout) {
					log((CLOG_DEBUG2 "grab keyboard timed out"));
					XUnmapWindow(display, m_window);
					return false;
				}
			}
		} while (result != GrabSuccess);
		log((CLOG_DEBUG2 "grabbed keyboard"));

		// now the mouse
		result = XGrabPointer(display, m_window, True, 0,
								GrabModeAsync, GrabModeAsync,
								m_window, None, CurrentTime);
		assert(result != GrabNotViewable);
		if (result != GrabSuccess) {
			// back off to avoid grab deadlock
			XUngrabKeyboard(display, CurrentTime);
			log((CLOG_DEBUG2 "ungrabbed keyboard, waiting to grab pointer"));
			CThread::sleep(0.05);
			if (timer.getTime() >= s_timeout) {
				log((CLOG_DEBUG2 "grab pointer timed out"));
				XUnmapWindow(display, m_window);
				return false;
			}
		}
	} while (result != GrabSuccess);
	log((CLOG_DEBUG1 "grabbed pointer and keyboard"));

	return true;
}

void
CXWindowsPrimaryScreen::hideWindow()
{
	CDisplayLock display(m_screen);

	// unmap the grab window.  this also ungrabs the mouse and keyboard.
	XUnmapWindow(display, m_window);
}

void
CXWindowsPrimaryScreen::warpCursorToCenter()
{
	warpCursor(m_xCenter, m_yCenter);
}

void
CXWindowsPrimaryScreen::warpCursorNoFlush(
				Display* display, SInt32 x, SInt32 y)
{
	assert(display  != NULL);
	assert(m_window != None);

	// send an event that we can recognize before the mouse warp
	XEvent eventBefore;
	eventBefore.type                = MotionNotify;
	eventBefore.xmotion.display     = display;
	eventBefore.xmotion.window      = m_window;
	eventBefore.xmotion.root        = m_screen->getRoot();
	eventBefore.xmotion.subwindow   = m_window;
	eventBefore.xmotion.time        = CurrentTime;
	eventBefore.xmotion.x           = x;
	eventBefore.xmotion.y           = y;
	eventBefore.xmotion.x_root      = x;
	eventBefore.xmotion.y_root      = y;
	eventBefore.xmotion.state       = 0;
	eventBefore.xmotion.is_hint     = False;
	eventBefore.xmotion.same_screen = True;
	XEvent eventAfter               = eventBefore;
	XSendEvent(display, m_window, False, 0, &eventBefore);

	// warp mouse
	XWarpPointer(display, None, m_screen->getRoot(), 0, 0, 0, 0, x, y);

	// send an event that we can recognize after the mouse warp
	XSendEvent(display, m_window, False, 0, &eventAfter);
	XSync(display, False);

	log((CLOG_DEBUG2 "warped to %d,%d", x, y));
}

void
CXWindowsPrimaryScreen::selectEvents(Display* display, Window w) const
{
	// ignore errors while we adjust event masks.  windows could be
	// destroyed at any time after the XQueryTree() in doSelectEvents()
	// so we must ignore BadWindow errors.
	CXWindowsUtil::CErrorLock lock(display);

	// adjust event masks
	doSelectEvents(display, w);
}

void
CXWindowsPrimaryScreen::doSelectEvents(Display* display, Window w) const
{
	// we want to track the mouse everywhere on the display.  to achieve
	// that we select PointerMotionMask on every window.  we also select
	// SubstructureNotifyMask in order to get CreateNotify events so we
	// select events on new windows too.
	//
	// note that this can break certain clients due a design flaw of X.
	// X will deliver a PointerMotion event to the deepest window in the
	// hierarchy that contains the pointer and has PointerMotionMask
	// selected by *any* client.  if another client doesn't select
	// motion events in a subwindow so the parent window will get them
	// then by selecting for motion events on the subwindow we break
	// that client because the parent will no longer get the events.

	// FIXME -- should provide some workaround for event selection
	// design flaw.  perhaps only select for motion events on windows
	// that already do or are top-level windows or don't propagate
	// pointer events.  or maybe an option to simply poll the mouse.

	// we don't want to adjust our grab window
	if (w == m_window) {
		return;
	}

	// select events of interest.  do this before querying the tree so
	// we'll get notifications of children created after the XQueryTree()
	// so we won't miss them.
	XSelectInput(display, w, PointerMotionMask | SubstructureNotifyMask);

	// recurse on child windows
	Window rw, pw, *cw;
	unsigned int nc;
	if (XQueryTree(display, w, &rw, &pw, &cw, &nc)) {
		for (unsigned int i = 0; i < nc; ++i) {
			doSelectEvents(display, cw[i]);
		}
		XFree(cw);
	}
}

KeyModifierMask
CXWindowsPrimaryScreen::mapModifier(unsigned int state) const
{
	// FIXME -- should be configurable
	KeyModifierMask mask = 0;
	if (state & ShiftMask)
		mask |= KeyModifierShift;
	if (state & LockMask)
		mask |= KeyModifierCapsLock;
	if (state & ControlMask)
		mask |= KeyModifierControl;
	if (state & Mod1Mask)
		mask |= KeyModifierAlt;
	if (state & Mod2Mask)
		mask |= KeyModifierNumLock;
	if (state & Mod4Mask)
		mask |= KeyModifierMeta;
	if (state & Mod5Mask)
		mask |= KeyModifierScrollLock;
	return mask;
}

KeyID
CXWindowsPrimaryScreen::mapKey(XKeyEvent* event) const
{
	CDisplayLock display(m_screen);

	// convert to a keysym
	// FIXME -- we're not properly handling unicode
	KeySym keysym;
	char dummy[1];
	XLookupString(event, dummy, 0, &keysym, NULL);

	// convert key
	switch (keysym & 0xffffff00) {
	case 0x0000:
		// Latin-1
		return static_cast<KeyID>(keysym);

	case 0xfe00:
		// ISO 9995 Function and Modifier Keys
		if (keysym == XK_ISO_Left_Tab) {
			return kKeyLeftTab;
		}
		return kKeyNone;

	case 0xff00:
		// MISCELLANY
		return static_cast<KeyID>(keysym - 0xff00 + 0xef00);

	default:
		// FIXME -- support unicode characters
		return kKeyNone;
	}
}

ButtonID
CXWindowsPrimaryScreen::mapButton(unsigned int button) const
{
	// FIXME -- should use button mapping?
	if (button >= 1 && button <= 3) {
		return static_cast<ButtonID>(button);
	}
	else {
		return kButtonNone;
	}
}

void
CXWindowsPrimaryScreen::updateKeys()
{
	CDisplayLock display(m_screen);

	// get modifier map from server
	XModifierKeymap* keymap = XGetModifierMapping(display);

	// initialize
	m_numLockMask    = 0;
	m_capsLockMask   = 0;
	m_scrollLockMask = 0;

	// set keycodes and masks
	for (unsigned int i = 0; i < 8; ++i) {
		const unsigned int bit = (1 << i);
		for (int j = 0; j < keymap->max_keypermod; ++j) {
			KeyCode keycode = keymap->modifiermap[i *
								keymap->max_keypermod + j];

			// note toggle modifier bits
			const KeySym keysym = XKeycodeToKeysym(display, keycode, 0);
			if (keysym == XK_Num_Lock) {
				m_numLockMask |= bit;
			}
			else if (keysym == XK_Caps_Lock) {
				m_capsLockMask |= bit;
			}
			else if (keysym == XK_Scroll_Lock) {
				m_scrollLockMask |= bit;
			}
		}
	}

	XFreeModifiermap(keymap);
}

Bool
CXWindowsPrimaryScreen::findKeyEvent(Display*, XEvent* xevent, XPointer arg)
{
	CKeyEventInfo* filter = reinterpret_cast<CKeyEventInfo*>(arg);
	return (xevent->type         == filter->m_event &&
			xevent->xkey.window  == filter->m_window &&
			xevent->xkey.time    == filter->m_time &&
			xevent->xkey.keycode == filter->m_keycode) ? True : False;
}
