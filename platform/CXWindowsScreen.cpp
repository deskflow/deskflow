#include "CXWindowsScreen.h"
#include "CXWindowsClipboard.h"
#include "CXWindowsScreenSaver.h"
#include "CXWindowsUtil.h"
#include "CClipboard.h"
#include "XScreen.h"
#include "CLock.h"
#include "CThread.h"
#include "CLog.h"
#include "IJob.h"
#include "CString.h"
#include <cstdlib>
#include <cstring>

//
// CXWindowsScreen::CTimer
//

CXWindowsScreen::CTimer::CTimer(IJob* job, double timeout) :
	m_job(job),
	m_timeout(timeout)
{
	assert(m_job != NULL);
	assert(m_timeout > 0.0);

	reset();
}

CXWindowsScreen::CTimer::~CTimer()
{
	// do nothing
}

void
CXWindowsScreen::CTimer::run()
{
	m_job->run();
}

void
CXWindowsScreen::CTimer::reset()
{
	m_time = m_timeout;
}

CXWindowsScreen::CTimer::CTimer&
CXWindowsScreen::CTimer::operator-=(double dt)
{
	m_time -= dt;
	return *this;
}

CXWindowsScreen::CTimer::operator double() const
{
	return m_time;
}

bool
CXWindowsScreen::CTimer::operator<(const CTimer& t) const
{
	return m_time < t.m_time;
}


//
// CXWindowsScreen
//

CXWindowsScreen*		CXWindowsScreen::s_screen = NULL;

CXWindowsScreen::CXWindowsScreen() :
	m_display(NULL),
	m_root(None),
	m_x(0), m_y(0),
	m_w(0), m_h(0),
	m_stop(false),
	m_screenSaver(NULL)
{
	assert(s_screen == NULL);
	s_screen = this;
}

CXWindowsScreen::~CXWindowsScreen()
{
	assert(s_screen  != NULL);
	assert(m_display == NULL);

	s_screen = NULL;
}

void
CXWindowsScreen::addTimer(IJob* job, double timeout)
{
	CLock lock(&m_timersMutex);
	removeTimerNoLock(job);
	m_timers.push(CTimer(job, timeout));
}

void
CXWindowsScreen::removeTimer(IJob* job)
{
	CLock lock(&m_timersMutex);
	removeTimerNoLock(job);
}

void
CXWindowsScreen::removeTimerNoLock(IJob* job)
{
	// do it the hard way.  first collect all jobs that are not
	// the removed job.
	CTimerPriorityQueue::container_type tmp;
	for (CTimerPriorityQueue::iterator index = m_timers.begin();
								index != m_timers.end(); ++index) {
		if (index->getJob() != job) {
			tmp.push_back(*index);
		}
	}

	// now swap in the new list
	m_timers.swap(tmp);
}

void
CXWindowsScreen::openDisplay()
{
	assert(m_display == NULL);

	// set the X I/O error handler so we catch the display disconnecting
	XSetIOErrorHandler(&CXWindowsScreen::ioErrorHandler);

	// get the DISPLAY
	const char* display = getenv("DISPLAY");
	if (display == NULL) {
		display = ":0.0";
	}

	// open the display
	log((CLOG_DEBUG "XOpenDisplay(\"%s\")", display));
	m_display = XOpenDisplay(display);
	if (m_display == NULL) {
		throw XScreenOpenFailure();
	}

	// get default screen and root window
	m_screen = DefaultScreen(m_display);
	m_root   = RootWindow(m_display, m_screen);

	// create the transparent cursor
	createBlankCursor();

	// get screen shape
	updateScreenShape();

	// initialize the screen saver
	m_screenSaver = new CXWindowsScreenSaver(this, m_display);
}

void
CXWindowsScreen::closeDisplay()
{
	CLock lock(&m_mutex);

	// done with screen saver
	delete m_screenSaver;
	m_screenSaver = NULL;

	// destroy clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		delete m_clipboard[id];
		m_clipboard[id] = NULL;
	}

	// close the display
	if (m_display != NULL) {
		XCloseDisplay(m_display);
		m_display = NULL;
		log((CLOG_DEBUG "closed display"));
	}
	XSetIOErrorHandler(NULL);
}

int
CXWindowsScreen::getScreen() const
{
	assert(m_display != NULL);
	return m_screen;
}

Window
CXWindowsScreen::getRoot() const
{
	assert(m_display != NULL);
	return m_root;
}

void
CXWindowsScreen::initClipboards(Window window)
{
	assert(m_display != NULL);
	assert(window    != None);

	// initialize clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		m_clipboard[id] = new CXWindowsClipboard(m_display, window, id);
	}
}

void
CXWindowsScreen::updateScreenShape()
{
	m_x = 0;
	m_y = 0;
	m_w = WidthOfScreen(ScreenOfDisplay(m_display, m_screen));
	m_h = HeightOfScreen(ScreenOfDisplay(m_display, m_screen));
	log((CLOG_INFO "screen shape: %d,%d %dx%d", m_x, m_y, m_w, m_h));
}

void
CXWindowsScreen::getScreenShape(SInt32& x, SInt32& y,
				SInt32& w, SInt32& h) const
{
	assert(m_display != NULL);

	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

void
CXWindowsScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	assert(m_display != NULL);

	Window root, window;
	int mx, my, xWindow, yWindow;
	unsigned int mask;
	if (XQueryPointer(m_display, getRoot(), &root, &window,
								&mx, &my, &xWindow, &yWindow, &mask)) {
		x = mx;
		y = my;
	}
	else {
		getCursorCenter(x, y);
	}
}

void
CXWindowsScreen::getCursorCenter(SInt32& x, SInt32& y) const
{
	assert(m_display != NULL);

	x = m_x + (m_w >> 1);
	y = m_y + (m_h >> 1);
}

void
CXWindowsScreen::createBlankCursor()
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
	m_cursor = XCreatePixmapCursor(m_display, bitmap, bitmap,
								&color, &color, 0, 0);

	// don't need bitmap or the data anymore
	delete[] data;
	XFreePixmap(m_display, bitmap);
}

Cursor
CXWindowsScreen::getBlankCursor() const
{
	return m_cursor;
}

bool
CXWindowsScreen::getEvent(XEvent* xevent) const
{
	// wait for an event in a cancellable way and don't lock the
	// display while we're waiting.
	m_mutex.lock();
	for (;;) {
		while (!m_stop && XPending(m_display) == 0) {
			// check timers
			if (const_cast<CXWindowsScreen*>(this)->processTimers()) {
				continue;
			}

			// wait
			m_mutex.unlock();
			CThread::sleep(0.01);
			m_mutex.lock();
		}
		if (m_stop) {
			m_mutex.unlock();
			return false;
		}
		else {
			// get the event
			XNextEvent(m_display, xevent);

			// process the event.  return the event if unhandled.
			m_mutex.unlock();
			if (!const_cast<CXWindowsScreen*>(this)->processEvent(xevent)) {
				return true;
			}
			m_mutex.lock();
		}
	}
}

void
CXWindowsScreen::doStop()
{
	// caller must have locked display
	m_stop = true;
}

ClipboardID
CXWindowsScreen::getClipboardID(Atom selection) const
{
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		if (m_clipboard[id] != NULL &&
			m_clipboard[id]->getSelection() == selection) {
			return id;
		}
	}
	return kClipboardEnd;
}

void
CXWindowsScreen::onUnexpectedClose()
{
	// do nothing
}

bool
CXWindowsScreen::processEvent(XEvent* xevent)
{
	switch (xevent->type) {
	case SelectionClear:
		{
			// we just lost the selection.  that means someone else
			// grabbed the selection so this screen is now the
			// selection owner.  report that to the subclass.
			ClipboardID id = getClipboardID(xevent->xselectionclear.selection);
			if (id != kClipboardEnd) {
				log((CLOG_DEBUG "lost clipboard %d ownership at time %d", id, xevent->xselectionclear.time));
				m_clipboard[id]->lost(xevent->xselectionclear.time);
				onLostClipboard(id);
				return true;
			}
		}
		break;

	case SelectionNotify:
		// notification of selection transferred.  we shouldn't
		// get this here because we handle them in the selection
		// retrieval methods.  we'll just delete the property
		// with the data (satisfying the usual ICCCM protocol).
		if (xevent->xselection.property != None) {
			CLock lock(&m_mutex);
			XDeleteProperty(m_display,
								xevent->xselection.requestor,
								xevent->xselection.property);
		}
		return true;

	case SelectionRequest:
		{
			// somebody is asking for clipboard data
			ClipboardID id = getClipboardID(
								xevent->xselectionrequest.selection);
			if (id != kClipboardEnd) {
				CLock lock(&m_mutex);
				m_clipboard[id]->addRequest(
								xevent->xselectionrequest.owner,
								xevent->xselectionrequest.requestor,
								xevent->xselectionrequest.target,
								xevent->xselectionrequest.time,
								xevent->xselectionrequest.property);
				return true;
			}
		}
		break;

	case PropertyNotify:
		// property delete may be part of a selection conversion
		if (xevent->xproperty.state == PropertyDelete) {
			processClipboardRequest(xevent->xproperty.window,
								xevent->xproperty.time,
								xevent->xproperty.atom);
			return true;
		}
		break;

	case DestroyNotify:
		// looks like one of the windows that requested a clipboard
		// transfer has gone bye-bye.
		destroyClipboardRequest(xevent->xdestroywindow.window);

		// we don't know if the event was handled or not so continue
		break;
	}

	// let screen saver have a go
	{
		CLock lock(&m_mutex);
		m_screenSaver->processEvent(xevent);
	}

	return false;
}

bool
CXWindowsScreen::processTimers()
{
	std::vector<IJob*> jobs;
	{
		CLock lock(&m_timersMutex);

		// get current time
		const double time = m_time.getTime();

		// done if no timers have expired
		if (m_timers.empty() || m_timers.top() > time) {
			return false;
		}

		// subtract current time from all timers.  note that this won't
		// change the order of elements in the priority queue (except
		// for floating point round off which we'll ignore).
		for (CTimerPriorityQueue::iterator index = m_timers.begin();
								index != m_timers.end(); ++index) {
			(*index) -= time;
		}

		// process all timers at or below zero, saving the jobs
		while (m_timers.top() <= 0.0) {
			CTimer timer = m_timers.top();
			jobs.push_back(timer.getJob());
			timer.reset();
			m_timers.pop();
			m_timers.push(timer);
		}

		// reset the clock
		m_time.reset();
	}

	// now run the jobs.  note that if one of these jobs removes
	// a timer later in the jobs list and deletes that job pointer
	// then this will crash when it tries to run that job.
	for (std::vector<IJob*>::iterator index = jobs.begin();
								index != jobs.end(); ++index) {
		(*index)->run();
	}
}

CXWindowsScreenSaver*
CXWindowsScreen::getScreenSaver() const
{
	return m_screenSaver;
}

bool
CXWindowsScreen::setDisplayClipboard(ClipboardID id,
				const IClipboard* clipboard)
{
	CLock lock(&m_mutex);

	// fail if we don't have the requested clipboard
	if (m_clipboard[id] == NULL) {
		return false;
	}

	// get the actual time.  ICCCM does not allow CurrentTime.
	Time timestamp = CXWindowsUtil::getCurrentTime(
								m_display, m_clipboard[id]->getWindow());

	if (clipboard != NULL) {
		// save clipboard data
		return CClipboard::copy(m_clipboard[id], clipboard, timestamp);
	}
	else {
		// assert clipboard ownership
		if (!m_clipboard[id]->open(timestamp)) {
			return false;
		}
		m_clipboard[id]->empty();
		m_clipboard[id]->close();
		return true;
	}
}

bool
CXWindowsScreen::getDisplayClipboard(ClipboardID id,
				IClipboard* clipboard) const
{
	assert(clipboard != NULL);

	// block others from using the display while we get the clipboard
	CLock lock(&m_mutex);

	// fail if we don't have the requested clipboard
	if (m_clipboard[id] == NULL) {
		return false;
	}

	// get the actual time.  ICCCM does not allow CurrentTime.
	Time timestamp = CXWindowsUtil::getCurrentTime(
								m_display, m_clipboard[id]->getWindow());

	// copy the clipboard
	return CClipboard::copy(clipboard, m_clipboard[id], timestamp);
}

void
CXWindowsScreen::processClipboardRequest(Window requestor,
				Time time, Atom property)
{
	CLock lock(&m_mutex);

	// check every clipboard until one returns success
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		if (m_clipboard[id] != NULL &&
			m_clipboard[id]->processRequest(requestor, time, property)) {
			break;
		}
	}
}

void
CXWindowsScreen::destroyClipboardRequest(Window requestor)
{
	CLock lock(&m_mutex);

	// check every clipboard until one returns success
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		if (m_clipboard[id] != NULL &&
			m_clipboard[id]->destroyRequest(requestor)) {
			break;
		}
	}
}

int
CXWindowsScreen::ioErrorHandler(Display*)
{
	// the display has disconnected, probably because X is shutting
	// down.  X forces us to exit at this point.  that's arguably
	// a flaw in X but, realistically, it's difficult to gracefully
	// handle not having a Display* anymore.  we'll simply log the
	// error, notify the subclass (which must not use the display
	// so we set it to NULL), and exit.
	log((CLOG_WARN "X display has unexpectedly disconnected"));
	s_screen->m_display = NULL;
	s_screen->onUnexpectedClose();
	log((CLOG_CRIT "quiting due to X display disconnection"));
	exit(17);
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
