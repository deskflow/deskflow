#include "CXWindowsPrimaryScreen.h"
#include "CXWindowsClipboard.h"
#include "CXWindowsScreenSaver.h"
#include "CXWindowsUtil.h"
#include "CClipboard.h"
#include "IServer.h"
#include "ProtocolTypes.h"
#include "CThread.h"
#include "CLog.h"
#include "CStopwatch.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/X.h>
#	include <X11/Xutil.h>
#	define XK_MISCELLANY
#	include <X11/keysymdef.h>
#endif

//
// CXWindowsPrimaryScreen
//

CXWindowsPrimaryScreen::CXWindowsPrimaryScreen(IServer* server) :
	m_server(server),
	m_active(false),
	m_window(None)
{
	// do nothing
}

CXWindowsPrimaryScreen::~CXWindowsPrimaryScreen()
{
	assert(m_window == None);
}

void
CXWindowsPrimaryScreen::run()
{
	for (;;) {
		// wait for and get the next event
		XEvent xevent;
		if (!getEvent(&xevent)) {
			break;
		}

		// handle event
		switch (xevent.type) {
		case CreateNotify:
			{
				// select events on new window
				CDisplayLock display(this);
				selectEvents(display, xevent.xcreatewindow.window);
			}
			break;

		case MappingNotify:
			{
				// keyboard mapping changed
				CDisplayLock display(this);
				XRefreshKeyboardMapping(&xevent.xmapping);
				updateModifierMap(display);
			}
			break;

		case ClientMessage:
			if (xevent.xclient.message_type == m_atomScreenSaver ||
				xevent.xclient.format       == 32) {
				// screen saver activation/deactivation event
				m_server->onScreenSaver(xevent.xclient.data.l[0] != 0);
			}
			break;

		case KeyPress:
			{
				log((CLOG_DEBUG1 "event: KeyPress code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
				const KeyModifierMask mask = mapModifier(xevent.xkey.state);
				const KeyID key = mapKey(&xevent.xkey);
				if (key != kKeyNone) {
					m_server->onKeyDown(key, mask);
					if (key == XK_Caps_Lock && m_capsLockHalfDuplex) {
						m_server->onKeyUp(key, mask | KeyModifierCapsLock);
					}
					else if (key == XK_Num_Lock && m_numLockHalfDuplex) {
						m_server->onKeyUp(key, mask | KeyModifierNumLock);
					}
				}
			}
			break;

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
					XEvent xevent2;
					CDisplayLock display(this);
					if (XCheckIfEvent(display, &xevent2,
									&CXWindowsPrimaryScreen::findKeyEvent,
									(XPointer)&filter) != True) {
						// no press event follows so it's a plain release
						log((CLOG_DEBUG1 "event: KeyRelease code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
						if (key == XK_Caps_Lock && m_capsLockHalfDuplex) {
							m_server->onKeyDown(key, mask);
						}
						else if (key == XK_Num_Lock && m_numLockHalfDuplex) {
							m_server->onKeyDown(key, mask);
						}
						m_server->onKeyUp(key, mask);
					}
					else {
						// found a press event following so it's a repeat.
						// we could attempt to count the already queued
						// repeats but we'll just send a repeat of 1.
						// note that we discard the press event.
						log((CLOG_DEBUG1 "event: repeat code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
						m_server->onKeyRepeat(key, mask, 1);
					}
				}
			}
			break;

		case ButtonPress:
			{
				log((CLOG_DEBUG1 "event: ButtonPress button=%d", xevent.xbutton.button));
				const ButtonID button = mapButton(xevent.xbutton.button);
				if (button != kButtonNone) {
					m_server->onMouseDown(button);
				}
			}
			break;

		case ButtonRelease:
			{
				log((CLOG_DEBUG1 "event: ButtonRelease button=%d", xevent.xbutton.button));
				const ButtonID button = mapButton(xevent.xbutton.button);
				if (button != kButtonNone) {
					m_server->onMouseUp(button);
				}
				else if (xevent.xbutton.button == 4) {
					// wheel forward (away from user)
					m_server->onMouseWheel(120);
				}
				else if (xevent.xbutton.button == 5) {
					// wheel backward (toward user)
					m_server->onMouseWheel(-120);
				}
			}
			break;

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
					// warpCursorNoLockNoFlush() for where the events
					// are sent.  we discard the matching sent event
					// and can be sure we've skipped the warp event.
					CDisplayLock display(this);
					do {
						XMaskEvent(display, PointerMotionMask, &xevent);
					} while (!xevent.xmotion.send_event);
				}
				else if (!m_active) {
					// motion on primary screen
					m_server->onMouseMovePrimary(m_x, m_y);
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
						CDisplayLock display(this);
						warpCursorNoLockNoFlush(display, m_xCenter, m_yCenter);
					}

					// send event if mouse moved.  do this after warping
					// back to center in case the motion takes us onto
					// the primary screen.  if we sent the event first
					// in that case then the warp would happen after
					// warping to the primary screen's enter position,
					// effectively overriding it.
					if (x != 0 || y != 0) {
						m_server->onMouseMoveSecondary(x, y);
					}
				}
			}
			break;
		}
	}
}

void
CXWindowsPrimaryScreen::stop()
{
	CDisplayLock display(this);
	doStop();
}

void
CXWindowsPrimaryScreen::open()
{
	// open the display
	openDisplay();

	// check for peculiarities
	// FIXME -- may have to get these from some database
	m_numLockHalfDuplex  = false;
	m_capsLockHalfDuplex = false;
//	m_numLockHalfDuplex  = true;
//	m_capsLockHalfDuplex = true;

	// get screen shape
	SInt32 x, y, w, h;
	getScreenShape(x, y, w, h);

	{
		CDisplayLock display(this);

		// get notified of screen saver activation/deactivation
		m_atomScreenSaver = XInternAtom(display, "SCREENSAVER", False);
		getScreenSaver()->setNotify(m_window);

		// update key state
		updateModifierMap(display);

		// get mouse position
		Window root, window;
		int mx, my, xWindow, yWindow;
		unsigned int mask;
		if (!XQueryPointer(display, m_window, &root, &window,
								&mx, &my, &xWindow, &yWindow, &mask)) {
			mx = w >> 1;
			my = h >> 1;
		}

		// save mouse position
		m_x = mx;
		m_y = my;
	}

	// save position of center of screen
	m_xCenter = x + (w >> 1);
	m_yCenter = y + (h >> 1);

	// send screen info
	CClientInfo info;
	info.m_x        = x;
	info.m_y        = y;
	info.m_w        = w;
	info.m_h        = h;
	info.m_zoneSize = 1;
	info.m_mx       = m_x;
	info.m_my       = m_y;
	m_server->onInfoChanged("", info);
}

void
CXWindowsPrimaryScreen::close()
{
	// stop being notified of screen saver activation/deactivation
	getScreenSaver()->setNotify(None);
	m_atomScreenSaver = None;

	// close the display
	closeDisplay();
}

void
CXWindowsPrimaryScreen::enter(SInt32 x, SInt32 y, bool forScreenSaver)
{
	log((CLOG_INFO "entering primary at %d,%d%s", x, y, forScreenSaver ? " for screen saver" : ""));
	assert(m_active == true);
	assert(m_window != None);

	CDisplayLock display(this);

	// unmap the grab window.  this also ungrabs the mouse and keyboard.
	XUnmapWindow(display, m_window);

	// warp to requested location
	if (!forScreenSaver) {
		warpCursorNoLock(display, x, y);
	}

	// redirect input to root window.  do not warp the mouse because
	// that will deactivate the screen saver.
	else {
		XSetInputFocus(display, PointerRoot, PointerRoot, CurrentTime);
	}

	// not active anymore
	m_active = false;
}

bool
CXWindowsPrimaryScreen::leave()
{
	log((CLOG_INFO "leaving primary"));
	assert(m_active == false);
	assert(m_window != None);

	CDisplayLock display(this);

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

	// warp mouse to center
	warpCursorNoLock(display, m_xCenter, m_yCenter);

	// local client now active
	m_active = true;

	return true;
}

void
CXWindowsPrimaryScreen::reconfigure()
{
	// do nothing
}

void
CXWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	CDisplayLock display(this);
	warpCursorNoLock(display, x, y);
}

void
CXWindowsPrimaryScreen::warpCursorNoLock(Display* display, SInt32 x, SInt32 y)
{
	// warp mouse
	warpCursorNoLockNoFlush(display, x, y);

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

void
CXWindowsPrimaryScreen::warpCursorNoLockNoFlush(
				Display* display, SInt32 x, SInt32 y)
{
	assert(display  != NULL);
	assert(m_window != None);

	// send an event that we can recognize before the mouse warp
	XEvent eventBefore;
	eventBefore.type                = MotionNotify;
	eventBefore.xmotion.display     = display;
	eventBefore.xmotion.window      = m_window;
	eventBefore.xmotion.root        = getRoot();
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
	XWarpPointer(display, None, getRoot(), 0, 0, 0, 0, x, y);

	// send an event that we can recognize after the mouse warp
	XSendEvent(display, m_window, False, 0, &eventAfter);
	XSync(display, False);

	log((CLOG_DEBUG2 "warped to %d,%d", x, y));
}

void
CXWindowsPrimaryScreen::setClipboard(ClipboardID id,
				const IClipboard* clipboard)
{
	setDisplayClipboard(id, clipboard);
}

void
CXWindowsPrimaryScreen::grabClipboard(ClipboardID id)
{
	setDisplayClipboard(id, NULL);
}

void
CXWindowsPrimaryScreen::getClipboard(ClipboardID id,
				IClipboard* clipboard) const
{
	getDisplayClipboard(id, clipboard);
}

KeyModifierMask
CXWindowsPrimaryScreen::getToggleMask() const
{
	CDisplayLock display(this);

	// query the pointer to get the keyboard state
	// FIXME -- is there a better way to do this?
	Window root, window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
	if (!XQueryPointer(display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state)) {
		return 0;
	}

	// convert to KeyModifierMask
	KeyModifierMask mask = 0;
	if (state & m_numLockMask)
		mask |= KeyModifierNumLock;
	if (state & m_capsLockMask)
		mask |= KeyModifierCapsLock;
	if (state & m_scrollLockMask)
		mask |= KeyModifierScrollLock;

	return mask;
}

bool
CXWindowsPrimaryScreen::isLockedToScreen() const
{
	CDisplayLock display(this);

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

void
CXWindowsPrimaryScreen::onOpenDisplay(Display* display)
{
	assert(m_window == None);

	// get size of screen
	SInt32 x, y, w, h;
	getScreenShape(x, y, w, h);

	// create the grab window.  this window is used to capture user
	// input when the user is focussed on another client.  don't let
	// the window manager mess with it.
	XSetWindowAttributes attr;
	attr.event_mask            = PointerMotionMask |// PointerMotionHintMask |
								 ButtonPressMask | ButtonReleaseMask |
								 KeyPressMask | KeyReleaseMask |
								 KeymapStateMask | PropertyChangeMask;
	attr.do_not_propagate_mask = 0;
	attr.override_redirect     = True;
	attr.cursor                = createBlankCursor();
	m_window = XCreateWindow(display, getRoot(), x, y, w, h, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);
	log((CLOG_DEBUG "window is 0x%08x", m_window));

	// start watching for events on other windows
	selectEvents(display, getRoot());
}

CXWindowsClipboard*
CXWindowsPrimaryScreen::createClipboard(ClipboardID id)
{
	CDisplayLock display(this);
	return new CXWindowsClipboard(display, m_window, id);
}

void
CXWindowsPrimaryScreen::onCloseDisplay(Display* display)
{
	assert(m_window != None);

	// destroy window
	if (display != NULL) {
		XDestroyWindow(display, m_window);
	}
	m_window = None;
}

void
CXWindowsPrimaryScreen::onUnexpectedClose()
{
	// tell server to shutdown
	m_server->onError();
}

void
CXWindowsPrimaryScreen::onLostClipboard(ClipboardID id)
{
	// tell server that the clipboard was grabbed locally
	m_server->onGrabClipboard("", id, 0);
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
	KeySym keysym;
	char dummy[1];

	CDisplayLock display(this);
	XLookupString(event, dummy, 0, &keysym, NULL);
	return static_cast<KeyID>(keysym);
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
CXWindowsPrimaryScreen::updateModifierMap(Display* display)
{
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
