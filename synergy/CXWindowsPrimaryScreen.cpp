#include "CXWindowsPrimaryScreen.h"
#include "CServer.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include <assert.h>
#include <X11/X.h>

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
	log((CLOG_DEBUG "XOpenDisplay(%s)", "NULL"));
	m_display = XOpenDisplay(NULL);	// FIXME -- allow non-default
	if (m_display == NULL)
		throw int(5);	// FIXME -- make exception for this

	// get default screen
	m_screen = DefaultScreen(m_display);
	Screen* screen = ScreenOfDisplay(m_display, m_screen);

	// get screen size
	m_w = WidthOfScreen(screen);
	m_h = HeightOfScreen(screen);
	log((CLOG_INFO "primary display size: %dx%d", m_w, m_h));

	// get the root window
	m_root = RootWindow(m_display, m_screen);

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
	m_window = XCreateWindow(m_display, m_root, 0, 0, m_w, m_h, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);

	// start watching for events on other windows
	selectEvents(m_root);

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
	log((CLOG_DEBUG "stopping event thread"));
	m_eventThread->cancel();
	m_eventThread->wait();
	delete m_eventThread;
	m_eventThread = NULL;
	log((CLOG_DEBUG "stopped event thread"));

	// destroy window
	XDestroyWindow(m_display, m_window);
	m_window = None;

	// close the display
	XCloseDisplay(m_display);
	m_display = NULL;
	log((CLOG_DEBUG "closed display"));
}

void					CXWindowsPrimaryScreen::enter(SInt32 x, SInt32 y)
{
	log((CLOG_INFO "entering primary at %d,%d", x, y));
	assert(m_display != NULL);
	assert(m_window  != None);
	assert(m_active  == true);

	CLock lock(&m_mutex);

	// warp to requested location
	XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, x, y);

	// unmap the grab window.  this also ungrabs the mouse and keyboard.
	XUnmapWindow(m_display, m_window);

	// remove all input events for grab window
	XEvent event;
	while (XCheckWindowEvent(m_display, m_window,
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
	assert(m_display != NULL);
	assert(m_window  != None);
	assert(m_active  == false);

	CLock lock(&m_mutex);

	// raise and show the input window
	XMapRaised(m_display, m_window);

	// grab the mouse and keyboard.  keep trying until we get them.
	// if we can't grab one after grabbing the other then ungrab
	// and wait before retrying.
	int result;
	do {
		// mouse first
		do {
			result = XGrabPointer(m_display, m_window, True, 0,
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
		result = XGrabKeyboard(m_display, m_window, True,
								GrabModeAsync, GrabModeAsync, CurrentTime);
		assert(result != GrabNotViewable);
		if (result != GrabSuccess) {
			XUngrabPointer(m_display, CurrentTime);
			log((CLOG_DEBUG "ungrabbed pointer, waiting to grab keyboard"));
			CThread::sleep(0.25);
		}
	} while (result != GrabSuccess);
	log((CLOG_DEBUG "grabbed keyboard"));

	// move the mouse to the center of grab window
	warpCursorNoLock(m_w >> 1, m_h >> 1);

	// local client now active
	m_active = true;
}

void					CXWindowsPrimaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	CLock lock(&m_mutex);
	warpCursorNoLock(x, y);
}

void					CXWindowsPrimaryScreen::warpCursorNoLock(
								SInt32 x, SInt32 y)
{
	// warp the mouse
	XWarpPointer(m_display, None, m_root, 0, 0, 0, 0, x, y);
	XSync(m_display, False);
	log((CLOG_DEBUG "warped to %d,%d", x, y));

	// discard mouse events since we just added one we don't want
	XEvent xevent;
	while (XCheckWindowEvent(m_display, m_window,
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
	XSelectInput(m_display, w, PointerMotionMask | SubstructureNotifyMask);

	// recurse on child windows
	Window rw, pw, *cw;
	unsigned int nc;
	if (XQueryTree(m_display, w, &rw, &pw, &cw, &nc)) {
		for (unsigned int i = 0; i < nc; ++i)
			selectEvents(cw[i]);
		XFree(cw);
	}
}

Cursor					CXWindowsPrimaryScreen::createBlankCursor()
{
	// this seems just a bit more complicated than really necessary

	// get the closet cursor size to 1x1
	unsigned int w, h;
	XQueryBestCursor(m_display, m_root, 1, 1, &w, &h);

	// make bitmap data for cursor of closet size.  since the cursor
	// is blank we can use the same bitmap for shape and mask:  all
	// zeros.
	const int size = ((w + 7) >> 3) * h;
	char* data = new char[size];
	memset(data, 0, size);

	// make bitmap
	Pixmap bitmap = XCreateBitmapFromData(m_display, m_root, data, w, h);

	// need an arbitrary color for the cursor
	XColor color;
	color.pixel = 0;
	color.red   = color.green = color.blue = 0;
	color.flags = DoRed | DoGreen | DoBlue;

	// make cursor from bitmap
	Cursor cursor = XCreatePixmapCursor(m_display, bitmap, bitmap,
								&color, &color, 0, 0);

	// don't need bitmap or the data anymore
	delete[] data;
	XFreePixmap(m_display, bitmap);

	return cursor;
}

void					CXWindowsPrimaryScreen::eventThread(void*)
{
	for (;;) {
		// wait for and then get the next event
		m_mutex.lock();
		while (XPending(m_display) == 0) {
			m_mutex.unlock();
			CThread::sleep(0.05);
			m_mutex.lock();
		}
		XEvent xevent;
		XNextEvent(m_display, &xevent);
		m_mutex.unlock();

		// handle event
		switch (xevent.type) {
		  case CreateNotify: {
			// select events on new window
			CLock lock(&m_mutex);
			selectEvents(xevent.xcreatewindow.window);
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
					CLock lock(&m_mutex);
					Window root, window;
					int xRoot, yRoot, xWindow, yWindow;
					unsigned int mask;
					if (!XQueryPointer(m_display, m_window, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &mask))
						break;

					x = xRoot - (m_w >> 1);
					y = yRoot - (m_h >> 1);

					// warp mouse back to center
					warpCursorNoLock(m_w >> 1, m_h >> 1);
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
	return static_cast<KeyID>(XKeycodeToKeysym(m_display, keycode, index));
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
