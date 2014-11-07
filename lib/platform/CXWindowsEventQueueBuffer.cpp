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

#include "CXWindowsEventQueueBuffer.h"
#include "CLock.h"
#include "CThread.h"
#include "CEvent.h"
#include "IEventQueue.h"
#if HAVE_POLL
#	include <poll.h>
#else
#	if HAVE_SYS_SELECT_H
#		include <sys/select.h>
#	endif
#	if HAVE_SYS_TIME_H
#		include <sys/time.h>
#	endif
#	if HAVE_SYS_TYPES_H
#		include <sys/types.h>
#	endif
#	if HAVE_UNISTD_H
#		include <unistd.h>
#	endif
#endif

//
// CEventQueueTimer
//

class CEventQueueTimer { };


//
// CXWindowsEventQueueBuffer
//

CXWindowsEventQueueBuffer::CXWindowsEventQueueBuffer(
				Display* display, Window window) :
	m_display(display),
	m_window(window),
	m_waiting(false)
{
	assert(m_display != NULL);
	assert(m_window  != None);

	m_userEvent = XInternAtom(m_display, "SYNERGY_USER_EVENT", False);
}

CXWindowsEventQueueBuffer::~CXWindowsEventQueueBuffer()
{
	// do nothing
}

void
CXWindowsEventQueueBuffer::waitForEvent(double dtimeout)
{
	CThread::testCancel();

	{
		CLock lock(&m_mutex);
		// we're now waiting for events
		m_waiting = true;

		// push out pending events
		flush();
	}

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
#if HAVE_POLL
	poll(pfds, 1, timeout);
#else
	select(ConnectionNumber(m_display) + 1,
						SELECT_TYPE_ARG234 &rfds,
						SELECT_TYPE_ARG234 NULL,
						SELECT_TYPE_ARG234 NULL,
						SELECT_TYPE_ARG5   timeoutPtr);
#endif

	{
		// we're no longer waiting for events
		CLock lock(&m_mutex);
		m_waiting = false;
	}

	CThread::testCancel();
}

IEventQueueBuffer::Type
CXWindowsEventQueueBuffer::getEvent(CEvent& event, UInt32& dataID)
{
	CLock lock(&m_mutex);

	// push out pending events
	flush();

	// get next event
	XNextEvent(m_display, &m_event);

	// process event
	if (m_event.xany.type == ClientMessage &&
		m_event.xclient.message_type == m_userEvent) {
		dataID = static_cast<UInt32>(m_event.xclient.data.l[0]);
		return kUser;
	}
	else {
		event = CEvent(CEvent::kSystem,
							IEventQueue::getSystemTarget(), &m_event);
		return kSystem;
	}
}

bool
CXWindowsEventQueueBuffer::addEvent(UInt32 dataID)
{
	// prepare a message
	XEvent xevent;
	xevent.xclient.type         = ClientMessage;
	xevent.xclient.window       = m_window;
	xevent.xclient.message_type = m_userEvent;
	xevent.xclient.format       = 32;
	xevent.xclient.data.l[0]    = static_cast<long>(dataID);

	// save the message
	CLock lock(&m_mutex);
	m_postedEvents.push_back(xevent);

	// if we're currently waiting for an event then send saved events to
	// the X server now.  if we're not waiting then some other thread
	// might be using the display connection so we can't safely use it
	// too.
	if (m_waiting) {
		flush();
	}

	return true;
}

bool
CXWindowsEventQueueBuffer::isEmpty() const
{
	CLock lock(&m_mutex);
	return (XPending(m_display) == 0);
}

CEventQueueTimer*
CXWindowsEventQueueBuffer::newTimer(double, bool) const
{
	return new CEventQueueTimer;
}

void
CXWindowsEventQueueBuffer::deleteTimer(CEventQueueTimer* timer) const
{
	delete timer;
}

void
CXWindowsEventQueueBuffer::flush()
{
	// note -- m_mutex must be locked on entry

	// flush the posted event list to the X server
	for (size_t i = 0; i < m_postedEvents.size(); ++i) {
		XSendEvent(m_display, m_window, False, 0, &m_postedEvents[i]);
	}
	XFlush(m_display);
	m_postedEvents.clear();
}
