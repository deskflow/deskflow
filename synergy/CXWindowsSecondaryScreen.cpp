#include "CXWindowsSecondaryScreen.h"
#include "CClient.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include <assert.h>
#include <X11/X.h>
#include <X11/extensions/XTest.h>

//
// CXWindowsSecondaryScreen
//

CXWindowsSecondaryScreen::CXWindowsSecondaryScreen() :
								m_client(NULL),
								m_display(NULL),
								m_window(None),
								m_w(0), m_h(0)
{
	// do nothing
}

CXWindowsSecondaryScreen::~CXWindowsSecondaryScreen()
{
	assert(m_display == NULL);
}

void					CXWindowsSecondaryScreen::open(CClient* client)
{
	assert(m_client == NULL);
	assert(client   != NULL);

	// set the client
	m_client = client;

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
	log((CLOG_INFO "secondary display size: %dx%d", m_w, m_h));

	// verify the availability of the XTest extension
	int majorOpcode, firstEvent, firstError;
	if (!XQueryExtension(m_display, XTestExtensionName,
								&majorOpcode, &firstEvent, &firstError))
		throw int(6);	// FIXME -- make exception for this

	// get the root window
	m_root = RootWindow(m_display, m_screen);

	// create the cursor hiding window.  this window is used to hide the
	// cursor when it's not on the screen.  the window is hidden as soon
	// as the cursor enters the screen or the display's real cursor is
	// moved.
	XSetWindowAttributes attr;
	attr.event_mask            = LeaveWindowMask;
	attr.do_not_propagate_mask = 0;
	attr.override_redirect     = True;
	attr.cursor                = createBlankCursor();
	m_window = XCreateWindow(m_display, m_root, 0, 0, 1, 1, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);

	// become impervious to server grabs
	XTestGrabControl(m_display, True);

	// hide the cursor
	leave();

	// start processing events
	m_eventThread = new CThread(new TMethodJob<CXWindowsSecondaryScreen>(
								this, &CXWindowsSecondaryScreen::eventThread));
}

void					CXWindowsSecondaryScreen::close()
{
	assert(m_client != NULL);
	assert(m_eventThread != NULL);

	// stop event thread
	m_eventThread->cancel();
	m_eventThread->wait();
	delete m_eventThread;
	m_eventThread = NULL;

	// no longer impervious to server grabs
	XTestGrabControl(m_display, False);

	// destroy window
	XDestroyWindow(m_display, m_window);
	m_window = None;

	// close the display
	XCloseDisplay(m_display);
	m_display = NULL;
}

void					CXWindowsSecondaryScreen::enter(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);
	assert(m_window != None);

	CLock lock(&m_mutex);

	// warp to requested location
	XTestFakeMotionEvent(m_display, m_screen, x, y, CurrentTime);
	XSync(m_display, False);

	// show cursor
	XUnmapWindow(m_display, m_window);
}

void					CXWindowsSecondaryScreen::leave()
{
	assert(m_display != NULL);
	assert(m_window != None);

	CLock lock(&m_mutex);

	// move hider window under the mouse (rather than moving the mouse
	// somewhere else on the screen)
	int x, y, dummy;
	unsigned int dummyMask;
	Window dummyWindow;
	XQueryPointer(m_display, m_root, &dummyWindow, &dummyWindow,
								&x, &y, &dummy, &dummy, &dummyMask);
	XMoveWindow(m_display, m_window, x, y);

	// raise and show the hider window
	XMapRaised(m_display, m_window);

	// hide cursor by moving it into the hider window
	XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, 0, 0);
}

void					CXWindowsSecondaryScreen::keyDown(
								KeyID key, KeyModifierMask mask)
{
	assert(m_display != NULL);

	CLock lock(&m_mutex);

	XTestFakeKeyEvent(m_display, mapKey(key, mask), True, CurrentTime);
	XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::keyRepeat(
								KeyID, KeyModifierMask, SInt32)
{
	assert(m_display != NULL);

	CLock lock(&m_mutex);

	// FIXME
}

void					CXWindowsSecondaryScreen::keyUp(
								KeyID key, KeyModifierMask mask)
{
	assert(m_display != NULL);

	CLock lock(&m_mutex);

	XTestFakeKeyEvent(m_display, mapKey(key, mask), False, CurrentTime);
	XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::mouseDown(ButtonID button)
{
	assert(m_display != NULL);

	CLock lock(&m_mutex);

	XTestFakeButtonEvent(m_display, mapButton(button), True, CurrentTime);
	XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::mouseUp(ButtonID button)
{
	assert(m_display != NULL);

	CLock lock(&m_mutex);

	XTestFakeButtonEvent(m_display, mapButton(button), False, CurrentTime);
	XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::mouseMove(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);

	CLock lock(&m_mutex);

	XTestFakeMotionEvent(m_display, m_screen, x, y, CurrentTime);
	XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::mouseWheel(SInt32)
{
	assert(m_display != NULL);

	CLock lock(&m_mutex);

	// FIXME
}

void					CXWindowsSecondaryScreen::getSize(
								SInt32* width, SInt32* height) const
{
	assert(m_display != NULL);
	assert(width != NULL && height != NULL);

	*width  = m_w;
	*height = m_h;
}

SInt32					CXWindowsSecondaryScreen::getJumpZoneSize() const
{
	assert(m_display != NULL);

	return 0;
}

Cursor					CXWindowsSecondaryScreen::createBlankCursor()
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

void					CXWindowsSecondaryScreen::eventThread(void*)
{
	assert(m_display != NULL);
	assert(m_window != None);

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
		  case LeaveNotify: {
			// mouse moved out of hider window somehow.  hide the window.
			CLock lock(&m_mutex);
			XUnmapWindow(m_display, m_window);
			break;
		  }

/*
		  // FIXME -- handle screen resolution changes

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

KeyCode					CXWindowsSecondaryScreen::mapKey(
								KeyID id, KeyModifierMask /*mask*/) const
{
	// FIXME -- use mask
	return XKeysymToKeycode(m_display, static_cast<KeySym>(id));
}

unsigned int			CXWindowsSecondaryScreen::mapButton(
								ButtonID id) const
{
	// FIXME -- should use button mapping?
	return static_cast<unsigned int>(id);
}
