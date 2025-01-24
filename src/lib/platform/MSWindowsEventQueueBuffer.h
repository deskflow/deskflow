/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueueBuffer.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class IEventQueue;

//! Event queue buffer for Win32
class MSWindowsEventQueueBuffer : public IEventQueueBuffer
{
public:
  MSWindowsEventQueueBuffer(IEventQueue *events);
  virtual ~MSWindowsEventQueueBuffer();

  // IEventQueueBuffer overrides
  virtual void init()
  {
  }
  virtual void waitForEvent(double timeout);
  virtual Type getEvent(Event &event, uint32_t &dataID);
  virtual bool addEvent(uint32_t dataID);
  virtual bool isEmpty() const;
  virtual EventQueueTimer *newTimer(double duration, bool oneShot) const;
  virtual void deleteTimer(EventQueueTimer *) const;

private:
  DWORD m_thread;
  UINT m_userEvent;
  MSG m_event;
  UINT m_daemonQuit;
  IEventQueue *m_events;
};
