#include "CXWindowsScreen.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include <string.h>
#include <assert.h>
#include <X11/X.h>

//
// CXWindowsScreen
//

CXWindowsScreen::CXWindowsScreen() :
								m_display(NULL),
								m_root(None),
								m_w(0), m_h(0)
{
	// do nothing
}

CXWindowsScreen::~CXWindowsScreen()
{
	assert(m_display == NULL);
}

void					CXWindowsScreen::openDisplay()
{
	assert(m_display == NULL);

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
	log((CLOG_INFO "display size: %dx%d", m_w, m_h));

	// get the root window
	m_root = RootWindow(m_display, m_screen);

	// let subclass prep display
	onOpenDisplay();

	// start processing events
	m_eventThread = new CThread(new TMethodJob<CXWindowsScreen>(
								this, &CXWindowsScreen::eventThread));
}

void					CXWindowsScreen::closeDisplay()
{
	assert(m_display != NULL);
	assert(m_eventThread != NULL);

	// stop event thread
	log((CLOG_DEBUG "stopping event thread"));
	m_eventThread->cancel();
	m_eventThread->wait();
	delete m_eventThread;
	m_eventThread = NULL;
	log((CLOG_DEBUG "stopped event thread"));

	// let subclass close down display
	onCloseDisplay();

	// close the display
	XCloseDisplay(m_display);
	m_display = NULL;
	log((CLOG_DEBUG "closed display"));
}

int						CXWindowsScreen::getScreen() const
{
	assert(m_display != NULL);
	return m_screen;
}

Window					CXWindowsScreen::getRoot() const
{
	assert(m_display != NULL);
	return m_root;
}

void					CXWindowsScreen::getScreenSize(
								SInt32* w, SInt32* h) const
{
	assert(m_display != NULL);
	assert(w != NULL && h != NULL);

	*w = m_w;
	*h = m_h;
}

Cursor					CXWindowsScreen::createBlankCursor() const
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

void					CXWindowsScreen::getEvent(XEvent* xevent) const
{
	// wait for an event in a cancellable way and don't lock the
	// display while we're waiting.
	m_mutex.lock();
	while (XPending(m_display) == 0) {
		m_mutex.unlock();
		CThread::sleep(0.05);
		m_mutex.lock();
	}
	XNextEvent(m_display, xevent);
	m_mutex.unlock();
}


//
// CXWindowsScreen::CDisplayLock
//

CXWindowsScreen::CDisplayLock::CDisplayLock(const CXWindowsScreen* screen) :
								m_mutex(&screen->m_mutex),
								m_display(screen->m_display)
{
	assert(m_display != NULL);

	m_mutex->lock();
}

CXWindowsScreen::CDisplayLock::~CDisplayLock()
{
	m_mutex->unlock();
}

CXWindowsScreen::CDisplayLock::operator Display*() const
{
	return m_display;
}
