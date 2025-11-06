/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"
#include "base/PriorityQueue.h"
#include "base/Stopwatch.h"
#include "mt/CondVar.h"

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

//! Event queue
/*!
An event queue that implements the platform independent parts and
delegates the platform dependent parts to a subclass.
*/
class EventQueue : public IEventQueue
{
public:
  EventQueue();
  EventQueue(EventQueue const &) = delete;
  EventQueue(EventQueue &&) = delete;
  ~EventQueue() override;
  EventQueue &operator=(EventQueue const &) = delete;
  EventQueue &operator=(EventQueue &&) = delete;

  // IEventQueue overrides
  void loop() override;
  void adoptBuffer(IEventQueueBuffer *) override;
  bool getEvent(Event &event, double timeout = -1.0) override;
  bool dispatchEvent(const Event &event) override;
  void addEvent(const Event &event) override;
  EventQueueTimer *newTimer(double duration, void *target) override;
  EventQueueTimer *newOneShotTimer(double duration, void *target) override;
  void deleteTimer(EventQueueTimer *) override;
  void addHandler(EventTypes type, void *target, const EventHandler &handler) override;
  void removeHandler(EventTypes type, void *target) override;
  void removeHandlers(void *target) override;
  void *getSystemTarget() override;
  void waitForReady() const override;

private:
  const EventHandler *getHandler(EventTypes type, void *target) const;
  uint32_t saveEvent(const Event &event);
  Event removeEvent(uint32_t eventID);
  bool hasTimerExpired(Event &event);
  double getNextTimerTimeout() const;
  void addEventToBuffer(const Event &event);

  //!
  //! \brief processEvent Internal event proccessing
  //! \param event - event to process
  //! \param timeout - Timeout to stop
  //! \param timer - StopWatch to use
  //! \return true if handled
  //!
  bool processEvent(Event &event, double timeout, Stopwatch &timer);

private:
  class Timer
  {
  public:
    Timer(EventQueueTimer *, double timeout, double initialTime, void *target, bool oneShot);
    ~Timer() = default;

    void reset();

    Timer &operator-=(double);

    operator double() const;

    bool isOneShot() const;
    EventQueueTimer *getTimer() const;
    void *getTarget() const;
    void fillEvent(TimerEvent &) const;

  private:
    EventQueueTimer *m_timer;
    double m_timeout;
    void *m_target;
    bool m_oneShot;
    double m_time;
  };

  using Timers = std::set<EventQueueTimer *>;
  using TimerQueue = PriorityQueue<Timer>;
  using EventTable = std::map<uint32_t, Event>;
  using EventIDList = std::vector<uint32_t>;
  using TypeHandlerTable = std::map<EventTypes, EventHandler>;
  using HandlerTable = std::map<void *, TypeHandlerTable>;

  int m_systemTarget = 0;
  mutable std::mutex m_mutex;

  // buffer of events
  std::unique_ptr<IEventQueueBuffer> m_buffer;

  // saved events
  EventTable m_events;
  EventIDList m_oldEventIDs;

  // timers
  Stopwatch m_time;
  Timers m_timers;
  TimerQueue m_timerQueue;
  TimerEvent m_timerEvent;

  // event handlers
  HandlerTable m_handlers;

  Mutex *m_readyMutex = nullptr;
  CondVar<bool> *m_readyCondVar = nullptr;
  std::queue<Event> m_pending;
};
