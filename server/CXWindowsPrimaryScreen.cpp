#include "CXWindowsPrimaryScreen.h"
#include "CServer.h"
#include "CThread.h"
#include "CLog.h"
#include <assert.h>
#include <X11/X.h>

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

		  case KeyPress: {
			log((CLOG_DEBUG "event: KeyPress code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
			const KeyModifierMask mask = mapModifier(xevent.xkey.state);
			const KeyID key = mapKey(xevent.xkey.keycode, mask);
			if (key != kKeyNone) {
				m_server->onKeyDown(key, mask);
			}
			break;
		  }

		  // FIXME -- simulate key repeat.  X sends press/release for
		  // repeat.  must detect auto repeat and use kKeyRepeat.
		  case KeyRelease: {
			log((CLOG_DEBUG "event: KeyRelease code=%d, state=0x%04x", xevent.xkey.keycode, xevent.xkey.state));
			const KeyModifierMask mask = mapModifier(xevent.xkey.state);
			const KeyID key = mapKey(xevent.xkey.keycode, mask);
			if (key != kKeyNone) {
				m_server->onKeyUp(key, mask);
			}
			break;
		  }

		  case ButtonPress: {
			log((CLOG_DEBUG "event: ButtonPress button=%d", xevent.xbutton.button));
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_server->onMouseDown(button);
			}
			break;
		  }

		  case ButtonRelease: {
			log((CLOG_DEBUG "event: ButtonRelease button=%d", xevent.xbutton.button));
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_server->onMouseUp(button);
			}
			break;
		  }

		  case MotionNotify: {
			log((CLOG_DEBUG "event: MotionNotify %d,%d", xevent.xmotion.x_root, xevent.xmotion.y_root));
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

/*
		  case SelectionClear:
			target->XXX(xevent.xselectionclear.);
			break;

		  case SelectionNotify:
			target->XXX(xevent.xselection.);
			break;

		  case SelectionRequest:
			target->XXX(xevent.xselectionrequest.);
			break;
*/
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
				log((CLOG_DEBUG "waiting to grab pointer"));
				CThread::sleep(0.25);
			}
		} while (result != GrabSuccess);
		log((CLOG_DEBUG "grabbed pointer"));

		// now the keyboard
		result = XGrabKeyboard(display, m_window, True,
								GrabModeAsync, GrabModeAsync, CurrentTime);
		assert(result != GrabNotViewable);
		if (result != GrabSuccess) {
			// back off to avoid grab deadlock
			XUngrabPointer(display, CurrentTime);
			log((CLOG_DEBUG "ungrabbed pointer, waiting to grab keyboard"));
			CThread::sleep(0.25);
		}
	} while (result != GrabSuccess);
	log((CLOG_DEBUG "grabbed keyboard"));

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
	log((CLOG_DEBUG "warped to %d,%d", x, y));

	// discard mouse events since we just added one we don't want
	XEvent xevent;
	while (XCheckWindowEvent(display, m_window,
								PointerMotionMask, &xevent)) {
		// do nothing
	}
}

#include <X11/Xatom.h> // FIXME
void					CXWindowsPrimaryScreen::setClipboard(
								const IClipboard* /*clipboard*/)
{
	// FIXME -- put this in superclass?
	// FIXME -- don't use CurrentTime
	CDisplayLock display(this);
	XSetSelectionOwner(display, XA_PRIMARY, m_window, CurrentTime);
	if (XGetSelectionOwner(display, XA_PRIMARY) == m_window) {
		// we got the selection
		log((CLOG_DEBUG "grabbed clipboard"));
	}
	// FIXME -- need to copy or adopt the clipboard to serve future requests
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
								IClipboard* clipboard) const
{
	// FIXME -- put this in superclass?
	// FIXME -- don't use CurrentTime
	getDisplayClipboard(clipboard, m_window, CurrentTime);
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
								 KeymapStateMask;
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
	XSelectInput(display, w, PointerMotionMask | SubstructureNotifyMask |
								PropertyChangeMask);

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
	if (state & 1)
		mask |= KeyModifierShift;
	if (state & 2)
		mask |= KeyModifierCapsLock;
	if (state & 4)
		mask |= KeyModifierControl;
	if (state & 8)
		mask |= KeyModifierAlt;
	if (state & 16)
		mask |= KeyModifierNumLock;
	if (state & 32)
		mask |= KeyModifierMeta;
	if (state & 128)
		mask |= KeyModifierScrollLock;
	return mask;
}

KeyID					CXWindowsPrimaryScreen::mapKey(
								KeyCode keycode, KeyModifierMask mask) const
{
	int index;
	if (mask & KeyModifierShift)
		index = 1;
	else
		index = 0;
	CDisplayLock display(this);
	return static_cast<KeyID>(XKeycodeToKeysym(display, keycode, index));
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
