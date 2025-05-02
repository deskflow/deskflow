/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/PriorityQueue.h"
#include "base/Stopwatch.h"
#include "mt/CondVar.h"

#include <map>
#include <queue>
#include <set>

class Mutex;

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
  void adoptHandler(EventTypes type, void *target, IEventJob *handler) override;
  void removeHandler(EventTypes type, void *target) override;
  void removeHandlers(void *target) override;
  bool isEmpty() const override;
  IEventJob *getHandler(EventTypes type, void *target) const override;
  void *getSystemTarget() override;
  void waitForReady() const override;

private:
  uint32_t saveEvent(const Event &event);
  Event removeEvent(uint32_t eventID);
  bool hasTimerExpired(Event &event);
  double getNextTimerTimeout() const;
  void addEventToBuffer(const Event &event);

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

    bool operator<(const Timer &) const;

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
  using TypeHandlerTable = std::map<EventTypes, IEventJob *>;
  using HandlerTable = std::map<void *, TypeHandlerTable>;

  int m_systemTarget;
  ArchMutex m_mutex;

  // buffer of events
  IEventQueueBuffer *m_buffer;

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

private:
  Mutex *m_readyMutex;
  CondVar<bool> *m_readyCondVar;
  std::queue<Event> m_pending;
};
