#include "CXWindowsPrimaryScreen.h"
#include "CServer.h"
#include "CThread.h"
#include "CLog.h"
#include <assert.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

//
// CXWindowsPrimaryScreen
//

CXWindowsPrimaryScreen::CXWindowsPrimaryScreen() :
								m_server(NULL),
								m_active(false),
								m_window(None)
{
	// do nothing
}

CXWindowsPrimaryScreen::~CXWindowsPrimaryScreen()
{
	assert(m_window == None);
}

void					CXWindowsPrimaryScreen::run()
{
	for (;;) {
		// wait for and get the next event
		XEvent xevent;
		if (!getEvent(&xevent)) {
			break;
		}

		// handle event
		switch (xevent.type) {
		case CreateNotify: {
			// select events on new window
			CDisplayLock display(this);
			selectEvents(display, xevent.xcreatewindow.window);
			break;
		}

		case MappingNotify: {
			// keyboard mapping changed
			CDisplayLock display(this);
			XRefreshKeyboardMapping(&xevent.xmapping);
			updateModifierMap(display);
			break;
		}

		case KeyPress: {
			log((CLOG_DEBUG1 "event: KeyPress code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
			const KeyModifierMask mask = mapModifier(xevent.xkey.state);
			const KeyID key = mapKey(&xevent.xkey);
			if (key != kKeyNone) {
				m_server->onKeyDown(key, mask);
				if (key == XK_Caps_Lock && m_capsLockHalfDuplex)
					m_server->onKeyUp(key, mask | KeyModifierCapsLock);
			}
			break;
		}

		// FIXME -- simulate key repeat.  X sends press/release for
		// repeat.  must detect auto repeat and use kKeyRepeat.
		case KeyRelease: {
			log((CLOG_DEBUG1 "event: KeyRelease code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
			const KeyModifierMask mask = mapModifier(xevent.xkey.state);
			const KeyID key = mapKey(&xevent.xkey);
			if (key != kKeyNone) {
				if (key == XK_Caps_Lock && m_capsLockHalfDuplex)
					m_server->onKeyDown(key, mask);
				m_server->onKeyUp(key, mask);
			}
			break;
		}

		case ButtonPress: {
			log((CLOG_DEBUG1 "event: ButtonPress button=%d", xevent.xbutton.button));
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_server->onMouseDown(button);
			}
			break;
		}

		case ButtonRelease: {
			log((CLOG_DEBUG1 "event: ButtonRelease button=%d", xevent.xbutton.button));
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_server->onMouseUp(button);
			}
			break;
		}

		case MotionNotify: {
			log((CLOG_DEBUG2 "event: MotionNotify %d,%d", xevent.xmotion.x_root, xevent.xmotion.y_root));
			SInt32 x, y;
			if (!m_active) {
				x = xevent.xmotion.x_root;
				y = xevent.xmotion.y_root;
				m_server->onMouseMovePrimary(x, y);
			}
			else {
				// FIXME -- slurp up all remaining motion events?
				// probably not since key strokes may go to wrong place.

				// get mouse deltas
				{
					CDisplayLock display(this);
					Window root, window;
					int xRoot, yRoot, xWindow, yWindow;
					unsigned int mask;
					if (!XQueryPointer(display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &mask))
						break;

					// compute position of center of window
					SInt32 w, h;
					getScreenSize(&w, &h);
					x = xRoot - (w >> 1);
					y = yRoot - (h >> 1);

					// warp mouse back to center
					warpCursorNoLock(display, w >> 1, h >> 1);
				}

				m_server->onMouseMoveSecondary(x, y);
			}
			break;
		}

		case SelectionClear:
			// we just lost the selection.  that means someone else
			// grabbed the selection so this screen is now the
			// selection owner.  report that to the server.
			if (lostClipboard(xevent.xselectionclear.selection,
								xevent.xselectionclear.time)) {
				m_server->grabClipboard(getClipboardID(
								xevent.xselectionclear.selection));
			}
			break;

		case SelectionNotify:
			// notification of selection transferred.  we shouldn't
			// get this here because we handle them in the selection
			// retrieval methods.  we'll just delete the property
			// with the data (satisfying the usual ICCCM protocol).
			if (xevent.xselection.property != None) {
				CDisplayLock display(this);
				XDeleteProperty(display, m_window, xevent.xselection.property);
			}
			break;

		case SelectionRequest:
			// somebody is asking for clipboard data
			if (xevent.xselectionrequest.owner == m_window) {
				addClipboardRequest(m_window,
								xevent.xselectionrequest.requestor,
								xevent.xselectionrequest.selection,
								xevent.xselectionrequest.target,
								xevent.xselectionrequest.property,
								xevent.xselectionrequest.time);
			}
			else {
				// unknown window.  return failure.
				CDisplayLock display(this);
				XEvent event;
				event.xselection.type      = SelectionNotify;
				event.xselection.display   = display;
				event.xselection.requestor = xevent.xselectionrequest.requestor;
				event.xselection.selection = xevent.xselectionrequest.selection;
				event.xselection.target    = xevent.xselectionrequest.target;
				event.xselection.property  = None;
				event.xselection.time      = xevent.xselectionrequest.time;
				XSendEvent(display, xevent.xselectionrequest.requestor,
								False, 0, &event);
			}
			break;

		case PropertyNotify:
			// clipboard transfers involve property changes so forward
			// the event to the superclass.  we only care about the
			// deletion of properties.
			if (xevent.xproperty.state == PropertyDelete) {
				processClipboardRequest(xevent.xproperty.window,
								xevent.xproperty.atom,
								xevent.xproperty.time);
			}
			break;

		case DestroyNotify:
			// looks like one of the windows that requested a clipboard
			// transfer has gone bye-bye.
			destroyClipboardRequest(xevent.xdestroywindow.window);
			break;
		}
	}
}

void					CXWindowsPrimaryScreen::stop()
{
	doStop();
}

void					CXWindowsPrimaryScreen::open(CServer* server)
{
	assert(m_server == NULL);
	assert(server   != NULL);

	// set the server
	m_server = server;

	// open the display
	openDisplay();

	// check for peculiarities
	// FIXME -- may have to get these from some database
	m_capsLockHalfDuplex = false;
//	m_capsLockHalfDuplex = true;

	// update key state
	{
		CDisplayLock display(this);
		updateModifierMap(display);
	}
}

void					CXWindowsPrimaryScreen::close()
{
	assert(m_server != NULL);

	// close the display
	closeDisplay();

	// done with server
	m_server = NULL;
}

void					CXWindowsPrimaryScreen::enter(SInt32 x, SInt32 y)
{
	log((CLOG_INFO "entering primary at %d,%d", x, y));
	assert(m_active == true);
	assert(m_window != None);

	CDisplayLock display(this);

	// warp to requested location
	XWarpPointer(display, None, m_window, 0, 0, 0, 0, x, y);

	// unmap the grab window.  this also ungrabs the mouse and keyboard.
	XUnmapWindow(display, m_window);

	// remove all input events for grab window
	XEvent event;
	while (XCheckWindowEvent(display, m_window,
								PointerMotionMask |
								ButtonPressMask | ButtonReleaseMask |
								KeyPressMask | KeyReleaseMask |
								KeymapStateMask,
								&event)) {
		// do nothing
	}

	// not active anymore
	m_active = false;
}

void					CXWindowsPrimaryScreen::leave()
{
	log((CLOG_INFO "leaving primary"));
	assert(m_active == false);
	assert(m_window != None);

	CDisplayLock display(this);

	// raise and show the input window
	XMapRaised(display, m_window);

	// grab the mouse and keyboard.  keep trying until we get them.
	// if we can't grab one after grabbing the other then ungrab
	// and wait before retrying.
	int result;
	do {
		// mouse first
		do {
			result = XGrabPointer(display, m_window, True, 0,
								GrabModeAsync, GrabModeAsync,
								m_window, None, CurrentTime);
			assert(result != GrabNotViewable);
			if (result != GrabSuccess) {
				log((CLOG_DEBUG2 "waiting to grab pointer"));
				CThread::sleep(0.1);
			}
		} while (result != GrabSuccess);
		log((CLOG_DEBUG2 "grabbed pointer"));

		// now the keyboard
		result = XGrabKeyboard(display, m_window, True,
								GrabModeAsync, GrabModeAsync, CurrentTime);
		assert(result != GrabNotViewable);
		if (result != GrabSuccess) {
			// back off to avoid grab deadlock
			XUngrabPointer(display, CurrentTime);
			log((CLOG_DEBUG2 "ungrabbed pointer, waiting to grab keyboard"));
			CThread::sleep(0.1);
		}
	} while (result != GrabSuccess);
	log((CLOG_DEBUG1 "grabbed pointer and keyboard"));

	// move the mouse to the center of grab window
	SInt32 w, h;
	getScreenSize(&w, &h);
	warpCursorNoLock(display, w >> 1, h >> 1);

	// local client now active
	m_active = true;
}

void					CXWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	CDisplayLock display(this);
	warpCursorNoLock(display, x, y);
}

void					CXWindowsPrimaryScreen::warpCursorNoLock(
								Display* display, SInt32 x, SInt32 y)
{
	assert(display  != NULL);
	assert(m_window != None);

	// warp the mouse
	XWarpPointer(display, None, getRoot(), 0, 0, 0, 0, x, y);
	XSync(display, False);
	log((CLOG_DEBUG1 "warped to %d,%d", x, y));

	// discard mouse events since we just added one we don't want
	XEvent xevent;
	while (XCheckWindowEvent(display, m_window,
								PointerMotionMask, &xevent)) {
		// do nothing
	}
}

void					CXWindowsPrimaryScreen::setClipboard(
								ClipboardID id, const IClipboard* clipboard)
{
	setDisplayClipboard(id, clipboard, m_window, getCurrentTime(m_window));
}

void					CXWindowsPrimaryScreen::grabClipboard(ClipboardID id)
{
	setDisplayClipboard(id, NULL, m_window, getCurrentTime(m_window));
}

void					CXWindowsPrimaryScreen::getSize(
								SInt32* width, SInt32* height) const
{
	getScreenSize(width, height);
}

SInt32					CXWindowsPrimaryScreen::getJumpZoneSize() const
{
	return 1;
}

void					CXWindowsPrimaryScreen::getClipboard(
								ClipboardID id, IClipboard* clipboard) const
{
	getDisplayClipboard(id, clipboard, m_window, getCurrentTime(m_window));
}

KeyModifierMask			CXWindowsPrimaryScreen::getToggleMask() const
{
	CDisplayLock display(this);

	// query the pointer to get the keyboard state
	// FIXME -- is there a better way to do this?
	Window root, window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
	if (!XQueryPointer(display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state))
		return 0;

	// convert to KeyModifierMask
	KeyModifierMask mask;
	if (state & m_numLockMask)
		mask |= KeyModifierNumLock;
	if (state & m_capsLockMask)
		mask |= KeyModifierCapsLock;
	if (state & m_scrollLockMask)
		mask |= KeyModifierScrollLock;

	return mask;
}

void					CXWindowsPrimaryScreen::onOpenDisplay()
{
	assert(m_window == None);

	CDisplayLock display(this);

	// get size of screen
	SInt32 w, h;
	getScreenSize(&w, &h);

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
	m_window = XCreateWindow(display, getRoot(), 0, 0, w, h, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);

	// start watching for events on other windows
	selectEvents(display, getRoot());
}

void					CXWindowsPrimaryScreen::onCloseDisplay()
{
	assert(m_window != None);

	// destroy window
	CDisplayLock display(this);
	XDestroyWindow(display, m_window);
	m_window = None;
}

long					CXWindowsPrimaryScreen::getEventMask(Window w) const
{
	if (w == m_window)
		return PointerMotionMask |// PointerMotionHintMask |
								 ButtonPressMask | ButtonReleaseMask |
								 KeyPressMask | KeyReleaseMask |
								 KeymapStateMask;
	else
		return PointerMotionMask | SubstructureNotifyMask;
}

void					CXWindowsPrimaryScreen::selectEvents(
								Display* display, Window w) const
{
	// we want to track the mouse everywhere on the display.  to achieve
	// that we select PointerMotionMask on every window.  we also select
	// SubstructureNotifyMask in order to get CreateNotify events so we
	// select events on new windows too.

	// we don't want to adjust our grab window
	if (w == m_window)
		return;

	// select events of interest
	XSelectInput(display, w, PointerMotionMask | SubstructureNotifyMask);

	// recurse on child windows
	Window rw, pw, *cw;
	unsigned int nc;
	if (XQueryTree(display, w, &rw, &pw, &cw, &nc)) {
		for (unsigned int i = 0; i < nc; ++i)
			selectEvents(display, cw[i]);
		XFree(cw);
	}
}

KeyModifierMask			CXWindowsPrimaryScreen::mapModifier(
								unsigned int state) const
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

KeyID					CXWindowsPrimaryScreen::mapKey(XKeyEvent* event) const
{
	KeySym keysym;
	char dummy[1];

	CDisplayLock display(this);
	XLookupString(event, dummy, 0, &keysym, NULL);
	return static_cast<KeyID>(keysym);
}

ButtonID				CXWindowsPrimaryScreen::mapButton(
								unsigned int button) const
{
	// FIXME -- should use button mapping?
	if (button >= 1 && button <= 3)
		return static_cast<ButtonID>(button);
	else
		return kButtonNone;
}

void					CXWindowsPrimaryScreen::updateModifierMap(
								Display* display)
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
