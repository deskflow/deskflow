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
	assert(m_window == None);
}

void					CXWindowsSecondaryScreen::run()
{
	assert(m_window != None);

	for (;;) {
		// wait for and get the next event
		XEvent xevent;
		if (!getEvent(&xevent)) {
			break;
		}

		// handle event
		switch (xevent.type) {
		  case LeaveNotify: {
			// mouse moved out of hider window somehow.  hide the window.
			assert(m_window != None);
			CDisplayLock display(this);
			XUnmapWindow(display, m_window);
			break;
		  }

		  case SelectionClear:
			// we just lost the selection.  that means someone else
			// grabbed the selection so this screen is now the
			// selection owner.  report that to the server.
			m_client->onClipboardChanged();
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
		}
	}
}

void					CXWindowsSecondaryScreen::stop()
{
	doStop();
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

void					CXWindowsSecondaryScreen::setClipboard(
								const IClipboard* clipboard)
{
	// FIXME -- don't use CurrentTime
	setDisplayClipboard(clipboard, m_window, CurrentTime);
}

void					CXWindowsSecondaryScreen::grabClipboard()
{
	// FIXME -- don't use CurrentTime
	setDisplayClipboard(NULL, m_window, CurrentTime);
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

void					CXWindowsSecondaryScreen::getClipboard(
								IClipboard* clipboard) const
{
	// FIXME -- don't use CurrentTime
	getDisplayClipboard(clipboard, m_window, CurrentTime);
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
