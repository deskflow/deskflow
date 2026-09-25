/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"

#include <mutex>

// The native callback never retains an OSXScreen. Cancel under the same lock
// before destroying a screen or its queue; late callbacks then have no target.
// Events carry no borrowed data: they only wake the owner to take a pending
// request. An old wakeup can service a new request, but cannot invent one.
class OSXMenuBarEventBridge
{
public:
  uint64_t arm(IEventQueue *events, void *target)
  {
    std::scoped_lock lock(m_mutex);
    m_events = events;
    m_target = target;
    m_pending = 0;
    return m_generation = ++m_serial;
  }

  void cancel(void *target)
  {
    std::scoped_lock lock(m_mutex);
    if (m_target == target)
      clear();
  }

  void notify() noexcept
  {
    try {
      std::scoped_lock lock(m_mutex);
      if (!m_events || m_pending)
        return;
      m_pending = m_generation;
      try {
        m_events->addEvent(Event(EventTypes::OsxScreenMenuBarShown, m_target));
      } catch (...) {
        m_pending = 0;
      }
    } catch (...) {
      // Exceptions must never cross a native notification callback.
    }
  }

  bool take(void *target, uint64_t generation)
  {
    std::scoped_lock lock(m_mutex);
    if (!generation || target != m_target || generation != m_pending)
      return false;
    clear(); // At most one reapply per departure, including duplicate notices.
    return true;
  }

private:
  void clear()
  {
    m_events = nullptr;
    m_target = nullptr;
    m_generation = m_pending = 0;
  }

  std::mutex m_mutex;
  IEventQueue *m_events = nullptr;
  void *m_target = nullptr;
  uint64_t m_serial = 0, m_generation = 0, m_pending = 0;
};
