#include "CXWindowsPrimaryScreen.h"
#include "CServer.h"
#include "CThread.h"
#include "TMethodJob.h"
#include <assert.h>
#include <X11/X.h>
#include <X11/extensions/XTest.h>

//
// CXWindowsPrimaryScreen
//

CXWindowsPrimaryScreen::CXWindowsPrimaryScreen() :
								m_server(NULL),
								m_display(NULL),
								m_w(0), m_h(0),
								m_window(None),
								m_active(false)
{
	// do nothing
}

CXWindowsPrimaryScreen::~CXWindowsPrimaryScreen()
{
	assert(m_display == NULL);
}

void					CXWindowsPrimaryScreen::open(CServer* server)
{
	assert(m_server == NULL);
	assert(server   != NULL);

	// set the server
	m_server = server;

	// open the display
	m_display = ::XOpenDisplay(NULL);	// FIXME -- allow non-default
	if (m_display == NULL)
		throw int(5);	// FIXME -- make exception for this

	// get default screen
	m_screen = DefaultScreen(m_display);
	Screen* screen = ScreenOfDisplay(m_display, m_screen);

	// get screen size
	m_w = WidthOfScreen(screen);
	m_h = HeightOfScreen(screen);

	// get the root window
	Window root = RootWindow(m_display, m_screen);

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
	attr.cursor                = None;
	m_window = ::XCreateWindow(m_display, root, 0, 0, m_w, m_h, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);

	// start watching for events on other windows
	selectEvents(root);

	// start processing events
	m_eventThread = new CThread(new TMethodJob<CXWindowsPrimaryScreen>(
								this, &CXWindowsPrimaryScreen::eventThread));
}

void					CXWindowsPrimaryScreen::close()
{
	assert(m_server != NULL);
	assert(m_window != None);
	assert(m_eventThread != NULL);

	// stop event thread
	m_eventThread->cancel();
	m_eventThread->wait();
	delete m_eventThread;
	m_eventThread = NULL;

	// destroy window
	::XDestroyWindow(m_display, m_window);
	m_window = None;

	// close the display
	::XCloseDisplay(m_display);
	m_display = NULL;
}

void					CXWindowsPrimaryScreen::enter(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);
	assert(m_window  != None);
	assert(m_active  == true);

	// warp to requested location
	::XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, x, y);

	// unmap the grab window.  this also ungrabs the mouse and keyboard.
	::XUnmapWindow(m_display, m_window);

	// remove all input events for grab window
	XEvent event;
	while (::XCheckWindowEvent(m_display, m_window,
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
	assert(m_display != NULL);
	assert(m_window  != None);
	assert(m_active  == false);

	// raise and show the input window
	::XMapRaised(m_display, m_window);

	// grab the mouse and keyboard.  keep trying until we get them.
	// if we can't grab one after grabbing the other then ungrab
	// and wait before retrying.
	int result;
	do {
		// mouse first
		do {
			result = ::XGrabPointer(m_display, m_window, True, 0,
								GrabModeAsync, GrabModeAsync,
								m_window, None, CurrentTime);
			assert(result != GrabNotViewable);
			if (result != GrabSuccess)
				CThread::sleep(0.25);
		} while (result != GrabSuccess);

		// now the keyboard
		result = ::XGrabKeyboard(m_display, m_window, True,
								GrabModeAsync, GrabModeAsync, CurrentTime);
		assert(result != GrabNotViewable);
		if (result != GrabSuccess) {
			::XUngrabPointer(m_display, CurrentTime);
			CThread::sleep(0.25);
		}
	} while (result != GrabSuccess);

	// move the mouse to the center of grab window
	warpCursor(m_w >> 1, m_h >> 1);

	// local client now active
	m_active = true;
}

void					CXWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{

	// warp the mouse
	Window root = RootWindow(m_display, m_screen);
	::XWarpPointer(m_display, None, root, 0, 0, 0, 0, x, y);
	::XSync(m_display, False);

	// discard mouse events since we just added one we don't want
	XEvent xevent;
	while (::XCheckWindowEvent(m_display, m_window,
								PointerMotionMask, &xevent)) {
		// do nothing
	}
}

void					CXWindowsPrimaryScreen::getSize(
								SInt32* width, SInt32* height) const
{
	assert(m_display != NULL);
	assert(width != NULL && height != NULL);

	*width  = m_w;
	*height = m_h;
}

SInt32					CXWindowsPrimaryScreen::getJumpZoneSize() const
{
	assert(m_display != NULL);

	return 1;
}

void					CXWindowsPrimaryScreen::selectEvents(Window w) const
{
	// we want to track the mouse everywhere on the display.  to achieve
	// that we select PointerMotionMask on every window.  we also select
	// SubstructureNotifyMask in order to get CreateNotify events so we
	// select events on new windows too.

	// we don't want to adjust our grab window
	if (w == m_window)
		return;

	// select events of interest
	::XSelectInput(m_display, w, PointerMotionMask | SubstructureNotifyMask);

	// recurse on child windows
	Window rw, pw, *cw;
	unsigned int nc;
	if (::XQueryTree(m_display, w, &rw, &pw, &cw, &nc)) {
		for (unsigned int i = 0; i < nc; ++i)
			selectEvents(cw[i]);
		::XFree(cw);
	}
}

void					CXWindowsPrimaryScreen::eventThread(void*)
{
	for (;;) {
		// wait for and then get the next event
		while (XPending(m_display) == 0) {
			CThread::sleep(0.05);
		}
		XEvent xevent;
		XNextEvent(m_display, &xevent);

		// handle event
		switch (xevent.type) {
		  case CreateNotify:
			// select events on new window
			selectEvents(xevent.xcreatewindow.window);
			break;

		  case KeyPress: {
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
			const KeyModifierMask mask = mapModifier(xevent.xkey.state);
			const KeyID key = mapKey(xevent.xkey.keycode, mask);
			if (key != kKeyNone) {
				m_server->onKeyUp(key, mask);
			}
			break;
		  }

		  case ButtonPress: {
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_server->onMouseDown(button);
			}
			break;
		  }

		  case ButtonRelease: {
			const ButtonID button = mapButton(xevent.xbutton.button);
			if (button != kButtonNone) {
				m_server->onMouseUp(button);
			}
			break;
		  }

		  case MotionNotify: {
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
				Window root, window;
				int xRoot, yRoot, xWindow, yWindow;
				unsigned int mask;
				if (!::XQueryPointer(m_display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &mask))
					break;
				x = xRoot - (m_w >> 1);
				y = yRoot - (m_h >> 1);

				// warp mouse back to center
				warpCursor(m_w >> 1, m_h >> 1);

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
	return static_cast<KeyID>(::XKeycodeToKeysym(m_display, keycode, index));
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
