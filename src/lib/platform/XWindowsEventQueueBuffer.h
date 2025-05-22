/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueueBuffer.h"
#include "mt/Mutex.h"

#include <vector>

#if X_DISPLAY_MISSING
#error X11 is required to build deskflow
#else
#include <X11/Xlib.h>
#endif

class IEventQueue;

//! Event queue buffer for X11
class XWindowsEventQueueBuffer : public IEventQueueBuffer
{
public:
  XWindowsEventQueueBuffer(Display *, Window, IEventQueue *events);
  XWindowsEventQueueBuffer(XWindowsEventQueueBuffer const &) = delete;
  XWindowsEventQueueBuffer(XWindowsEventQueueBuffer &&) = delete;
  ~XWindowsEventQueueBuffer() override;

  XWindowsEventQueueBuffer &operator=(XWindowsEventQueueBuffer const &) = delete;
  XWindowsEventQueueBuffer &operator=(XWindowsEventQueueBuffer &&) = delete;

  // IEventQueueBuffer overrides
  void init() override
  {
  }
  void waitForEvent(double timeout) override;
  Type getEvent(Event &event, uint32_t &dataID) override;
  bool addEvent(uint32_t dataID) override;
  bool isEmpty() const override;
  EventQueueTimer *newTimer(double duration, bool oneShot) const override;
  void deleteTimer(EventQueueTimer *) const override;

private:
  void flush();

  int getPendingCountLocked();

private:
  using EventList = std::vector<XEvent>;

  Mutex m_mutex;
  Display *m_display;
  Window m_window;
  Atom m_userEvent;
  XEvent m_event;
  EventList m_postedEvents;
  bool m_waiting = false;
  int m_pipefd[2];
  IEventQueue *m_events;
};
