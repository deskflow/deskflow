#include "CXWindowsSecondaryScreen.h"
#include "CClient.h"
#include "CThread.h"
#include "TMethodJob.h"
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
	m_display = ::XOpenDisplay(NULL);	// FIXME -- allow non-default
	if (m_display == NULL)
		throw int(5);	// FIXME -- make exception for this

	// get default screen
	m_screen = DefaultScreen(m_display);
	Screen* screen = ScreenOfDisplay(m_display, m_screen);

	// get screen size
	m_w = WidthOfScreen(screen);
	m_h = HeightOfScreen(screen);

	// verify the availability of the XTest extension
	int majorOpcode, firstEvent, firstError;
	if (!::XQueryExtension(m_display, XTestExtensionName,
								&majorOpcode, &firstEvent, &firstError))
		throw int(6);	// FIXME -- make exception for this

	// become impervious to server grabs
	::XTestGrabControl(m_display, True);

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
	::XTestGrabControl(m_display, False);

	// close the display
	::XCloseDisplay(m_display);
	m_display = NULL;
}

void					CXWindowsSecondaryScreen::enter(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);

	// warp to requested location
	warpCursor(x, y);

	// show cursor
	// FIXME
}

void					CXWindowsSecondaryScreen::leave()
{
	assert(m_display != NULL);

	// hide cursor
	// FIXME
}

void					CXWindowsSecondaryScreen::warpCursor(SInt32 x, SInt32 y)
{
	assert(m_display != NULL);

	::XTestFakeMotionEvent(m_display, m_screen, x, y, CurrentTime);
	::XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::onKeyDown(
								KeyID key, KeyModifierMask mask)
{
	assert(m_display != NULL);

	::XTestFakeKeyEvent(m_display, mapKey(key, mask), True, CurrentTime);
	::XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::onKeyRepeat(
								KeyID, KeyModifierMask, SInt32)
{
	assert(m_display != NULL);

	// FIXME
}

void					CXWindowsSecondaryScreen::onKeyUp(
								KeyID key, KeyModifierMask mask)
{
	assert(m_display != NULL);

	::XTestFakeKeyEvent(m_display, mapKey(key, mask), False, CurrentTime);
	::XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::onMouseDown(ButtonID button)
{
	assert(m_display != NULL);

	::XTestFakeButtonEvent(m_display, mapButton(button), True, CurrentTime);
	::XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::onMouseUp(ButtonID button)
{
	assert(m_display != NULL);

	::XTestFakeButtonEvent(m_display, mapButton(button), False, CurrentTime);
	::XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::onMouseMove(
								SInt32 x, SInt32 y)
{
	assert(m_display != NULL);

	::XTestFakeMotionEvent(m_display, m_screen, x, y, CurrentTime);
	::XSync(m_display, False);
}

void					CXWindowsSecondaryScreen::onMouseWheel(SInt32)
{
	assert(m_display != NULL);

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

void					CXWindowsSecondaryScreen::eventThread(void*)
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
		  case LeaveNotify:
			// mouse moved out of window somehow.  hide the window.
//			::XUnmapWindow(m_display, m_window);
			break;

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
	return ::XKeysymToKeycode(m_display, static_cast<KeySym>(id));
}

unsigned int			CXWindowsSecondaryScreen::mapButton(
								ButtonID id) const
{
	// FIXME -- should use button mapping?
	return static_cast<unsigned int>(id);
}
