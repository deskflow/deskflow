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

#include "CMSWindowsEventQueueBuffer.h"
#include "CThread.h"
#include "IEventQueue.h"
#include "CArchMiscWindows.h"

//
// CEventQueueTimer
//

class CEventQueueTimer { };


//
// CMSWindowsEventQueueBuffer
//

CMSWindowsEventQueueBuffer::CMSWindowsEventQueueBuffer()
{
	// remember thread.  we'll be posting messages to it.
	m_thread     = GetCurrentThreadId();

	// create a message type for custom events
	m_userEvent  = RegisterWindowMessage("SYNERGY_USER_EVENT");

	// get message type for daemon quit
	m_daemonQuit = CArchMiscWindows::getDaemonQuitMessage();

	// make sure this thread has a message queue
	MSG dummy;
	PeekMessage(&dummy, NULL, WM_USER, WM_USER, PM_NOREMOVE);
}

CMSWindowsEventQueueBuffer::~CMSWindowsEventQueueBuffer()
{
	// do nothing
}

void
CMSWindowsEventQueueBuffer::waitForEvent(double timeout)
{
	// check if messages are available first.  if we don't do this then
	// MsgWaitForMultipleObjects() will block even if the queue isn't
	// empty if the messages in the queue were there before the last
	// call to GetMessage()/PeekMessage().
	if (HIWORD(GetQueueStatus(QS_ALLINPUT)) != 0) {
		return;
	}

	// convert timeout
	DWORD t;
	if (timeout < 0.0) {
		t = INFINITE;
	}
	else {
		t = (DWORD)(1000.0 * timeout);
	}

	// wait for a message.  we cannot be interrupted by thread
	// cancellation but that's okay because we're run in the main
	// thread and we never cancel that thread.
	HANDLE dummy[1];
	MsgWaitForMultipleObjects(0, dummy, FALSE, t, QS_ALLINPUT);
}

IEventQueueBuffer::Type
CMSWindowsEventQueueBuffer::getEvent(CEvent& event, UInt32& dataID)
{
	// peek at messages first.  waiting for QS_ALLINPUT will return
	// if a message has been sent to our window but GetMessage will
	// dispatch that message behind our backs and block.  PeekMessage
	// will also dispatch behind our backs but won't block.
	if (!PeekMessage(&m_event, NULL, 0, 0, PM_NOREMOVE) &&
		!PeekMessage(&m_event, (HWND)-1, 0, 0, PM_NOREMOVE)) {
		return kNone;
	}

	// BOOL.  yeah, right.
	BOOL result = GetMessage(&m_event, NULL, 0, 0);
	if (result == -1) {
		return kNone;
	}
	else if (result == 0) {
		event = CEvent(CEvent::kQuit);
		return kSystem;
	}
	else if (m_daemonQuit != 0 && m_event.message == m_daemonQuit) {
		event = CEvent(CEvent::kQuit);
		return kSystem;
	}
	else if (m_event.message == m_userEvent) {
		dataID = static_cast<UInt32>(m_event.wParam);
		return kUser;
	}
	else {
		event = CEvent(CEvent::kSystem,
							IEventQueue::getSystemTarget(), &m_event);
		return kSystem;
	}
}

bool
CMSWindowsEventQueueBuffer::addEvent(UInt32 dataID)
{
	return (PostThreadMessage(m_thread, m_userEvent,
							static_cast<WPARAM>(dataID), 0) != 0);
}

bool
CMSWindowsEventQueueBuffer::isEmpty() const
{
	return (HIWORD(GetQueueStatus(QS_ALLINPUT)) == 0);
}

CEventQueueTimer*
CMSWindowsEventQueueBuffer::newTimer(double, bool) const
{
	return new CEventQueueTimer;
}

void
CMSWindowsEventQueueBuffer::deleteTimer(CEventQueueTimer* timer) const
{
	delete timer;
}
