/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CXWindowsEventQueue.h"
#include "CEvent.h"
#include "CThread.h"
#if UNIX_LIKE
#	if HAVE_POLL
#		include <sys/poll.h>
#	else
#		if HAVE_SYS_SELECT_H
#			include <sys/select.h>
#		endif
#		if HAVE_SYS_TIME_H
#			include <sys/time.h>
#		endif
#		if HAVE_SYS_TYPES_H
#			include <sys/types.h>
#		endif
#		if HAVE_UNISTD_H
#			include <unistd.h>
#		endif
#	endif
#endif

//
// CEventQueueTimer
//

class CEventQueueTimer { };


//
// CXWindowsEventQueue
//

CXWindowsEventQueue::CXWindowsEventQueue(Display* display) :
	m_display(display)
{
	m_userEvent = XInternAtom(m_display, "SYNERGY_USER_EVENT", False);

	XSetWindowAttributes attr;
	m_window = XCreateWindow(m_display, DefaultRootWindow(m_display),
							0, 0, 1, 1, 0, 0, InputOnly, CopyFromParent,
							0, &attr);
}

CXWindowsEventQueue::~CXWindowsEventQueue()
{
	XDestroyWindow(m_display, m_window);
}

void
CXWindowsEventQueue::processSystemEvent(CEvent& event)
{
	event = CEvent(CEvent::kSystem, getSystemTarget(), &m_event);
}

void
CXWindowsEventQueue::processClientMessage(CEvent& event)
{
	assert(m_event.xany.type == ClientMessage);

	// handle user events specially
	if (m_event.xclient.message_type == m_userEvent) {
		// get event data
		CEventData data = removeEventData(m_event.xclient.data.l[1]);

		// create event
		event = CEvent(static_cast<size_t>(m_event.xclient.data.l[0]),
							data.first, data.second);
	}
	else {
		processSystemEvent(event);
	}
}

void
CXWindowsEventQueue::waitForEvent(double dtimeout)
{
	// use poll() to wait for a message from the X server or for timeout.
	// this is a good deal more efficient than polling and sleeping.
#if HAVE_POLL
	struct pollfd pfds[1];
	pfds[0].fd     = ConnectionNumber(m_display);
	pfds[0].events = POLLIN;
	int timeout    = (dtimeout < 0.0) ? -1 :
						static_cast<int>(1000.0 * dtimeout);
#else
	struct timeval timeout;
	struct timeval* timeoutPtr;
	if (dtimeout < 0.0) {
		timeoutPtr = NULL;
	}
	else {
		timeout.tv_sec  = static_cast<int>(dtimeout);
		timeout.tv_usec = static_cast<int>(1.0e+6 *
								(dtimeout - timeout.tv_sec));
		timeoutPtr      = &timeout;
	}

	// initialize file descriptor sets
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(ConnectionNumber(m_display), &rfds);
#endif

	// wait for message from X server or for timeout.  also check
	// if the thread has been cancelled.  poll() should return -1
	// with EINTR when the thread is cancelled.
	CThread::testCancel();
#if HAVE_POLL
	poll(pfds, 1, timeout);
#else
	select(ConnectionNumber(m_display) + 1,
						SELECT_TYPE_ARG234 &rfds,
						SELECT_TYPE_ARG234 NULL,
						SELECT_TYPE_ARG234 NULL,
						SELECT_TYPE_ARG5   timeoutPtr);
#endif
	CThread::testCancel();
}

bool
CXWindowsEventQueue::doGetEvent(CEvent& event)
{
	// get next event
	XNextEvent(m_display, &m_event);

	// process event
	if (m_event.xany.type == ClientMessage) {
		processClientMessage(event);
	}
	else {
		processSystemEvent(event);
	}

	return true;
}

bool
CXWindowsEventQueue::doAddEvent(CEvent::Type type, UInt32 dataID)
{
	// send ourself a message
	XEvent xevent;
	xevent.xclient.type         = ClientMessage;
	xevent.xclient.window       = m_window;
	xevent.xclient.message_type = m_userEvent;
	xevent.xclient.format       = 32;
	xevent.xclient.data.l[0]    = static_cast<long>(type);
	xevent.xclient.data.l[1]    = static_cast<long>(dataID);
	return (XSendEvent(m_display, m_window, False, 0, &xevent) != 0);
}

bool
CXWindowsEventQueue::doIsEmpty() const
{
	return (XPending(m_display) == 0);
}

CEventQueueTimer*
CXWindowsEventQueue::doNewTimer(double, bool) const
{
	return new CEventQueueTimer();
}

void
CXWindowsEventQueue::doDeleteTimer(CEventQueueTimer* timer) const
{
	delete timer;
}
