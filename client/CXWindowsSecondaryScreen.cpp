#include "CXWindowsSecondaryScreen.h"
#include "CClient.h"
#include "CThread.h"
#include "CLog.h"
#include <assert.h>
#include <X11/X.h>
#include <X11/extensions/XTest.h>

//
// CXWindowsSecondaryScreen
//

CXWindowsSecondaryScreen::CXWindowsSecondaryScreen() :
								m_client(NULL),
								m_window(None)
{
	// do nothing
}

CXWindowsSecondaryScreen::~CXWindowsSecondaryScreen()
{
	assert(m_window  == None);
}

void					CXWindowsSecondaryScreen::open(CClient* client)
{
	assert(m_client == NULL);
	assert(client   != NULL);

	// set the client
	m_client = client;

	// open the display
	openDisplay();

	// verify the availability of the XTest extension
	CDisplayLock display(this);
	int majorOpcode, firstEvent, firstError;
	if (!XQueryExtension(display, XTestExtensionName,
								&majorOpcode, &firstEvent, &firstError))
		throw int(6);	// FIXME -- make exception for this
}

void					CXWindowsSecondaryScreen::close()
{
	assert(m_client != NULL);

	// close the display
	closeDisplay();

	// done with client
	m_client = NULL;
}

void					CXWindowsSecondaryScreen::enter(SInt32 x, SInt32 y)
{
	assert(m_window != None);

	CDisplayLock display(this);

	// warp to requested location
	XTestFakeMotionEvent(display, getScreen(), x, y, CurrentTime);
	XSync(display, False);

	// show cursor
	XUnmapWindow(display, m_window);
}

void					CXWindowsSecondaryScreen::leave()
{
	CDisplayLock display(this);
	leaveNoLock(display);
}

void					CXWindowsSecondaryScreen::keyDown(
								KeyID key, KeyModifierMask mask)
{
	CDisplayLock display(this);
	XTestFakeKeyEvent(display, mapKey(key, mask), True, CurrentTime);
	XSync(display, False);
}

void					CXWindowsSecondaryScreen::keyRepeat(
								KeyID, KeyModifierMask, SInt32)
{
	CDisplayLock display(this);
	// FIXME
}

void					CXWindowsSecondaryScreen::keyUp(
								KeyID key, KeyModifierMask mask)
{
	CDisplayLock display(this);
	XTestFakeKeyEvent(display, mapKey(key, mask), False, CurrentTime);
	XSync(display, False);
}

void					CXWindowsSecondaryScreen::mouseDown(ButtonID button)
{
	CDisplayLock display(this);
	XTestFakeButtonEvent(display, mapButton(button), True, CurrentTime);
	XSync(display, False);
}

void					CXWindowsSecondaryScreen::mouseUp(ButtonID button)
{
	CDisplayLock display(this);
	XTestFakeButtonEvent(display, mapButton(button), False, CurrentTime);
	XSync(display, False);
}

void					CXWindowsSecondaryScreen::mouseMove(SInt32 x, SInt32 y)
{
	CDisplayLock display(this);
	XTestFakeMotionEvent(display, getScreen(), x, y, CurrentTime);
	XSync(display, False);
}

void					CXWindowsSecondaryScreen::mouseWheel(SInt32)
{
	CDisplayLock display(this);
	// FIXME
}

void					CXWindowsSecondaryScreen::getSize(
								SInt32* width, SInt32* height) const
{
	getScreenSize(width, height);
}

SInt32					CXWindowsSecondaryScreen::getJumpZoneSize() const
{
	return 0;
}

void					CXWindowsSecondaryScreen::onOpenDisplay()
{
	assert(m_window == None);

	CDisplayLock display(this);

	// create the cursor hiding window.  this window is used to hide the
	// cursor when it's not on the screen.  the window is hidden as soon
	// as the cursor enters the screen or the display's real cursor is
	// moved.
	XSetWindowAttributes attr;
	attr.event_mask            = LeaveWindowMask;
	attr.do_not_propagate_mask = 0;
	attr.override_redirect     = True;
	attr.cursor                = createBlankCursor();
	m_window = XCreateWindow(display, getRoot(), 0, 0, 1, 1, 0, 0,
								InputOnly, CopyFromParent,
								CWDontPropagate | CWEventMask |
								CWOverrideRedirect | CWCursor,
								&attr);

	// become impervious to server grabs
	XTestGrabControl(display, True);

	// hide the cursor
	leaveNoLock(display);
}

void					CXWindowsSecondaryScreen::onCloseDisplay()
{
	assert(m_window != None);

	// no longer impervious to server grabs
	CDisplayLock display(this);
	XTestGrabControl(display, False);

	// destroy window
	XDestroyWindow(display, m_window);
	m_window = None;
}

void					CXWindowsSecondaryScreen::eventThread(void*)
{
	assert(m_window != None);

	for (;;) {
		// wait for and get the next event
		XEvent xevent;
		getEvent(&xevent);

		// handle event
		switch (xevent.type) {
		  case LeaveNotify: {
			// mouse moved out of hider window somehow.  hide the window.
			assert(m_window != None);
			CDisplayLock display(this);
			XUnmapWindow(display, m_window);
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

void					CXWindowsSecondaryScreen::leaveNoLock(Display* display)
{
	assert(display  != NULL);
	assert(m_window != None);

	// move hider window under the mouse (rather than moving the mouse
	// somewhere else on the screen)
	int x, y, dummy;
	unsigned int dummyMask;
	Window dummyWindow;
	XQueryPointer(display, getRoot(), &dummyWindow, &dummyWindow,
								&x, &y, &dummy, &dummy, &dummyMask);
	XMoveWindow(display, m_window, x, y);

	// raise and show the hider window
	XMapRaised(display, m_window);

	// hide cursor by moving it into the hider window
	XWarpPointer(display, None, m_window, 0, 0, 0, 0, 0, 0);
}

KeyCode					CXWindowsSecondaryScreen::mapKey(
								KeyID id, KeyModifierMask /*mask*/) const
{
	CDisplayLock display(this);
	// FIXME -- use mask
	return XKeysymToKeycode(display, static_cast<KeySym>(id));
}

unsigned int			CXWindowsSecondaryScreen::mapButton(
								ButtonID id) const
{
	// FIXME -- should use button mapping?
	return static_cast<unsigned int>(id);
}
