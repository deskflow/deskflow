/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsEventQueueBuffer.h"

#include "arch/win32/ArchMiscWindows.h"
#include "base/IEventQueue.h"
#include "mt/Thread.h"

//
// EventQueueTimer
//

class EventQueueTimer
{
};

//
// MSWindowsEventQueueBuffer
//

MSWindowsEventQueueBuffer::MSWindowsEventQueueBuffer(IEventQueue *events) : m_events(events)
{
  // remember thread.  we'll be posting messages to it.
  m_thread = GetCurrentThreadId();

  // create a message type for custom events
  m_userEvent = RegisterWindowMessage(L"DESKFLOW_USER_EVENT");

  // get message type for daemon quit
  m_daemonQuit = ArchMiscWindows::getDaemonQuitMessage();

  // make sure this thread has a message queue
  MSG dummy;
  PeekMessage(&dummy, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
}

void MSWindowsEventQueueBuffer::waitForEvent(double timeout)
{
  // check if messages are available first.  if we don't do this then
  // MsgWaitForMultipleObjects() will block even if the queue isn't
  // empty if the messages in the queue were there before the last
  // call to GetMessage()/PeekMessage().
  if (HIWORD(GetQueueStatus(QS_ALLPOSTMESSAGE)) != 0) {
    return;
  }

  // convert timeout
  DWORD t;
  if (timeout < 0.0) {
    t = INFINITE;
  } else {
    t = (DWORD)(1000.0 * timeout);
  }

  // wait for a message.  we cannot be interrupted by thread
  // cancellation but that's okay because we're run in the main
  // thread and we never cancel that thread.
  HANDLE dummy[1];
  MsgWaitForMultipleObjects(0, dummy, FALSE, t, QS_ALLPOSTMESSAGE);
}

IEventQueueBuffer::Type MSWindowsEventQueueBuffer::getEvent(Event &event, uint32_t &dataID)
{
  using enum IEventQueueBuffer::Type;
  // NOTE: QS_ALLINPUT was replaced with QS_ALLPOSTMESSAGE.
  //
  // peek at messages first.  waiting for QS_ALLINPUT will return
  // if a message has been sent to our window but GetMessage will
  // dispatch that message behind our backs and block.  PeekMessage
  // will also dispatch behind our backs but won't block.
  if (!PeekMessage(&m_event, nullptr, 0, 0, PM_NOREMOVE) && !PeekMessage(&m_event, (HWND)-1, 0, 0, PM_NOREMOVE)) {
    return Unknown;
  }

  // BOOL.  yeah, right.
  BOOL result = GetMessage(&m_event, nullptr, 0, 0);
  if (result == -1) {
    return Unknown;
  } else if (result == 0) {
    event = Event(EventTypes::Quit);
    return System;
  } else if (m_daemonQuit != 0 && m_event.message == m_daemonQuit) {
    event = Event(EventTypes::Quit);
    return System;
  } else if (m_event.message == m_userEvent) {
    dataID = static_cast<uint32_t>(m_event.wParam);
    return User;
  } else {
    event = Event(EventTypes::System, m_events->getSystemTarget(), &m_event);
    return System;
  }
}

bool MSWindowsEventQueueBuffer::addEvent(uint32_t dataID)
{
  return (PostThreadMessage(m_thread, m_userEvent, static_cast<WPARAM>(dataID), 0) != 0);
}

bool MSWindowsEventQueueBuffer::isEmpty() const
{
  return (HIWORD(GetQueueStatus(QS_ALLPOSTMESSAGE)) == 0);
}

EventQueueTimer *MSWindowsEventQueueBuffer::newTimer(double, bool) const
{
  return new EventQueueTimer;
}

void MSWindowsEventQueueBuffer::deleteTimer(EventQueueTimer *timer) const
{
  delete timer;
}
