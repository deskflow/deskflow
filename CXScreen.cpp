#include "CXScreen.h"
#include "CEvent.h"
#include "CEventQueue.h"
#include <assert.h>
#include <unistd.h>
#include <X11/X.h>
#include <X11/extensions/XTest.h>

//
// CXScreen
//
class XClientOpen { }; // FIXME

CXScreen::CXScreen(const CString& name) :
								m_name(name),
								m_display(NULL),
								m_primary(false),
								m_w(0), m_h(0),
								m_window(None),
								m_active(false)
{
	// do nothing
}

CXScreen::~CXScreen()
{
	assert(m_display == NULL);
}

void					CXScreen::open(bool isPrimary)
{
	assert(m_display == NULL);

	m_primary = isPrimary;

	bool opened = false;
	try {
		// open the display
		m_display = ::XOpenDisplay(NULL);	// FIXME -- allow non-default
		if (m_display == NULL)
			throw XClientOpen();

		// hook up event handling
		onOpen(m_primary);
		opened = true;

		// get default screen
		m_screen = DefaultScreen(m_display);
		Screen* screen = ScreenOfDisplay(m_display, m_screen);

		// get screen size
		m_w = WidthOfScreen(screen);
		m_h = HeightOfScreen(screen);

		// type specific operations
		if (m_primary)
			openPrimary();
		else
			openSecondary();
	}
	catch (...) {
		if (opened)
			onClose();

		if (m_display != NULL) {
			::XCloseDisplay(m_display);
			m_display = NULL;
		}

		throw;
	}
}

void					CXScreen::close()
{
	assert(m_display != NULL);

	// type specific operations
	if (m_primary)
		closePrimary();
	else
		closeSecondary();

	// unhook event handling
	onClose();

	// close the display
	::XCloseDisplay(m_display);
	m_display = NULL;
}

void					CXScreen::enterScreen(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);

	if (m_primary)
		enterScreenPrimary(x, y);
	else
		enterScreenSecondary(x, y);
}

void					CXScreen::leaveScreen()
{
	assert(m_display != NULL);

	if (m_primary)
		leaveScreenPrimary();
	else
		leaveScreenSecondary();
}

void					CXScreen::warpCursor(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);

	// warp the mouse
	Window root = RootWindow(m_display, m_screen);
	::XWarpPointer(m_display, None, root, 0, 0, 0, 0, x, y);
	::XSync(m_display, False);

	// discard mouse events since we just added one we don't want
	XEvent xevent;
	while (::XCheckWindowEvent(m_display, m_window,
								PointerMotionMask, &xevent))
		; // do nothing
}

void					CXScreen::setClipboard(
								const IClipboard* clipboard)
{
	assert(m_display != NULL);

	if (m_primary)
		setClipboardPrimary(clipboard);
	else
		setClipboardSecondary(clipboard);
}

void					CXScreen::onScreenSaver(bool show)
{
	assert(m_display != NULL);

	if (m_primary)
		onScreenSaverPrimary(show);
	else
		onScreenSaverSecondary(show);
}

void					CXScreen::onKeyDown(KeyID key, KeyModifierMask)
{
	assert(m_display != NULL);
	assert(m_primary == false);

	// FIXME -- use mask
	::XTestFakeKeyEvent(m_display, mapKeyToX(key), True, CurrentTime);
	::XSync(m_display, False);
}

void					CXScreen::onKeyRepeat(KeyID, KeyModifierMask, SInt32)
{
	assert(m_display != NULL);
	assert(m_primary == false);

	// FIXME
}

void					CXScreen::onKeyUp(KeyID key, KeyModifierMask)
{
	assert(m_display != NULL);
	assert(m_primary == false);

	// FIXME -- use mask
	::XTestFakeKeyEvent(m_display, mapKeyToX(key), False, CurrentTime);
	::XSync(m_display, False);
}

void					CXScreen::onMouseDown(ButtonID button)
{
	assert(m_display != NULL);
	assert(m_primary == false);

	::XTestFakeButtonEvent(m_display, mapButtonToX(button), True, CurrentTime);
	::XSync(m_display, False);
}

void					CXScreen::onMouseUp(ButtonID button)
{
	assert(m_display != NULL);
	assert(m_primary == false);

	::XTestFakeButtonEvent(m_display, mapButtonToX(button), False, CurrentTime);
	::XSync(m_display, False);
}

void					CXScreen::onMouseMove(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);
	assert(m_primary == false);

	::XTestFakeMotionEvent(m_display, m_screen, x, y, CurrentTime);
	::XSync(m_display, False);
}

void					CXScreen::onMouseWheel(SInt32)
{
	assert(m_display != NULL);
	assert(m_primary == false);

	// FIXME
}

void					CXScreen::onClipboardChanged()
{
	assert(m_display != NULL);
	assert(m_primary == false);

	// FIXME
}

CString					CXScreen::getName() const
{
	return m_name;
}

void					CXScreen::getSize(
								SInt32* width, SInt32* height) const
{
	assert(m_display != NULL);
	assert(width != NULL && height != NULL);

	*width  = m_w;
	*height = m_h;
}

void					CXScreen::getClipboard(
								IClipboard* /*clipboard*/) const
{
	assert(m_display != NULL);

	// FIXME
}

void					CXScreen::openPrimary()
{
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
}

void					CXScreen::closePrimary()
{
	assert(m_window != None);

	// destroy window
	::XDestroyWindow(m_display, m_window);
	m_window = None;
}

void					CXScreen::enterScreenPrimary(SInt32 x, SInt32 y)
{
	assert(m_window != None);
	assert(m_active == true);

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
								&event))
		; // do nothing

	// not active anymore
	m_active = false;
}

void					CXScreen::leaveScreenPrimary()
{
	assert(m_window != None);
	assert(m_active == false);

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
				::sleep(1);
		} while (result != GrabSuccess);

		// now the keyboard
		result = ::XGrabKeyboard(m_display, m_window, True,
								GrabModeAsync, GrabModeAsync, CurrentTime);
		assert(result != GrabNotViewable);
		if (result != GrabSuccess) {
			::XUngrabPointer(m_display, CurrentTime);
			::sleep(1);
		}
	} while (result != GrabSuccess);

	// move the mouse to the center of grab window
	warpCursor(m_w >> 1, m_h >> 1);

	// local client now active
	m_active = true;
}

void					CXScreen::setClipboardPrimary(
								const IClipboard* /*clipboard*/)
{
	// FIXME
}

void					CXScreen::onScreenSaverPrimary(bool /*show*/)
{
	// FIXME
}

void					CXScreen::openSecondary()
{
	// verify the availability of the XTest extension
	int majorOpcode, firstEvent, firstError;
	if (!::XQueryExtension(m_display, XTestExtensionName,
								&majorOpcode, &firstEvent, &firstError))
		throw XClientOpen();

	// become impervious to server grabs
	XTestGrabControl(m_display, True);
}

void					CXScreen::closeSecondary()
{
	// no longer impervious to server grabs
	XTestGrabControl(m_display, False);
}

void					CXScreen::enterScreenSecondary(
								SInt32 x, SInt32 y)
{
	// FIXME
}

void					CXScreen::leaveScreenSecondary()
{
	// FIXME
}

void					CXScreen::setClipboardSecondary(
								const IClipboard* /*clipboard*/)
{
	// FIXME
}

void					CXScreen::onScreenSaverSecondary(bool /*show*/)
{
	// FIXME
}

Display*				CXScreen::getDisplay() const
{
	return m_display;
}

void					CXScreen::onEvents()
{
	if (m_primary)
		onPrimaryEvents();
	else
		onSecondaryEvents();
}

void					CXScreen::selectEvents(Window w) const
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

KeyModifierMask			CXScreen::mapModifierFromX(unsigned int state) const
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

unsigned int			CXScreen::mapModifierToX(KeyModifierMask mask) const
{
	// FIXME -- should be configurable
	unsigned int state = 0;
	if (mask & KeyModifierShift)
		state |= 1;
	if (mask & KeyModifierControl)
		state |= 4;
	if (mask & KeyModifierAlt)
		state |= 8;
	if (mask & KeyModifierMeta)
		state |= 32;
	if (mask & KeyModifierCapsLock)
		state |= 2;
	if (mask & KeyModifierNumLock)
		state |= 16;
	if (mask & KeyModifierScrollLock)
		state |= 128;
	return state;
}

KeyID					CXScreen::mapKeyFromX(
								KeyCode keycode, KeyModifierMask mask) const
{
	int index;
	if (mask & KeyModifierShift)
		index = 1;
	else
		index = 0;
	return static_cast<KeyID>(::XKeycodeToKeysym(m_display, keycode, index));
}

KeyCode					CXScreen::mapKeyToX(KeyID keyID) const
{
	return ::XKeysymToKeycode(m_display, static_cast<KeySym>(keyID));
}

ButtonID				CXScreen::mapButtonFromX(unsigned int button) const
{
	// FIXME -- should use button mapping?
	if (button >= 1 && button <= 3)
		return static_cast<ButtonID>(button);
	else
		return kButtonNone;
}

unsigned int			CXScreen::mapButtonToX(ButtonID buttonID) const
{
	// FIXME -- should use button mapping?
	return static_cast<unsigned int>(buttonID);
}

void					CXScreen::onPrimaryEvents()
{
	while (XPending(m_display) > 0) {
		XEvent xevent;
		XNextEvent(m_display, &xevent);

		switch (xevent.type) {
		  case KeyPress: {
			const KeyModifierMask mask = mapModifierFromX(xevent.xkey.state);
			const KeyID key = mapKeyFromX(xevent.xkey.keycode, mask);
			if (key != kKeyNone) {
				CEvent event;
				event.m_key.m_type  = CEventBase::kKeyDown;
				event.m_key.m_key   = key;
				event.m_key.m_mask  = mask;
				event.m_key.m_count = 0;
				CEQ->push(&event);
			}
			break;
		  }

		  // FIXME -- simulate key repeat.  X sends press/release for
		  // repeat.  must detect auto repeat and use kKeyRepeat.
		  case KeyRelease: {
			const KeyModifierMask mask = mapModifierFromX(xevent.xkey.state);
			const KeyID key = mapKeyFromX(xevent.xkey.keycode, mask);
			if (key != kKeyNone) {
				CEvent event;
				event.m_key.m_type  = CEventBase::kKeyUp;
				event.m_key.m_key   = key;
				event.m_key.m_mask  = mask;
				event.m_key.m_count = 0;
				CEQ->push(&event);
			}
			break;
		  }

		  case ButtonPress: {
			const ButtonID button = mapButtonFromX(xevent.xbutton.button);
			if (button != kButtonNone) {
				CEvent event;
				event.m_mouse.m_type   = CEventBase::kMouseDown;
				event.m_mouse.m_button = button;
				event.m_mouse.m_x      = 0;
				event.m_mouse.m_y      = 0;
				CEQ->push(&event);
			}
			break;
		  }

		  case ButtonRelease: {
			const ButtonID button = mapButtonFromX(xevent.xbutton.button);
			if (button != kButtonNone) {
				CEvent event;
				event.m_mouse.m_type   = CEventBase::kMouseUp;
				event.m_mouse.m_button = button;
				event.m_mouse.m_x      = 0;
				event.m_mouse.m_y      = 0;
				CEQ->push(&event);
			}
			break;
		  }

		  case MotionNotify: {
			CEvent event;
			event.m_mouse.m_type   = CEventBase::kMouseMove;
			event.m_mouse.m_button = kButtonNone;
			if (!m_active) {
				event.m_mouse.m_x = xevent.xmotion.x_root;
				event.m_mouse.m_y = xevent.xmotion.y_root;
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
				event.m_mouse.m_x = xRoot - (m_w >> 1);
				event.m_mouse.m_y = yRoot - (m_h >> 1);

				// warp mouse back to center
				warpCursor(m_w >> 1, m_h >> 1);
			}
			CEQ->push(&event);
			break;
		  }

		  case CreateNotify:
			// select events on new window
			if (m_primary)
				selectEvents(xevent.xcreatewindow.window);
			break;

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

void					CXScreen::onSecondaryEvents()
{
	while (XPending(m_display) > 0) {
		XEvent xevent;
		XNextEvent(m_display, &xevent);
		// FIXME
	}
}
