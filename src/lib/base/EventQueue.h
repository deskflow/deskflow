/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/PriorityQueue.h"
#include "base/Stopwatch.h"
#include "common/stdmap.h"
#include "common/stdset.h"
#include "mt/CondVar.h"

#include <queue>

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
  virtual ~EventQueue();
  EventQueue &operator=(EventQueue const &) = delete;
  EventQueue &operator=(EventQueue &&) = delete;

  // IEventQueue overrides
  virtual void loop();
  virtual void adoptBuffer(IEventQueueBuffer *);
  virtual bool getEvent(Event &event, double timeout = -1.0);
  virtual bool dispatchEvent(const Event &event);
  virtual void addEvent(const Event &event);
  virtual EventQueueTimer *newTimer(double duration, void *target);
  virtual EventQueueTimer *newOneShotTimer(double duration, void *target);
  virtual void deleteTimer(EventQueueTimer *);
  virtual void adoptHandler(Event::Type type, void *target, IEventJob *handler);
  virtual void removeHandler(Event::Type type, void *target);
  virtual void removeHandlers(void *target);
  virtual Event::Type registerTypeOnce(Event::Type &type, const char *name);
  virtual bool isEmpty() const;
  virtual IEventJob *getHandler(Event::Type type, void *target) const;
  virtual const char *getTypeName(Event::Type type);
  virtual Event::Type getRegisteredType(const std::string &name) const;
  void *getSystemTarget();
  virtual void waitForReady() const;

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
    ~Timer();

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
  using TypeMap = std::map<Event::Type, const char *>;
  using NameMap = std::map<std::string, Event::Type>;
  using TypeHandlerTable = std::map<Event::Type, IEventJob *>;
  using HandlerTable = std::map<void *, TypeHandlerTable>;

  int m_systemTarget;
  ArchMutex m_mutex;

  // registered events
  Event::Type m_nextType;
  TypeMap m_typeMap;
  NameMap m_nameMap;

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

public:
  //
  // Event type providers.
  //
  ClientEvents &forClient();
  IStreamEvents &forIStream();
  IDataSocketEvents &forIDataSocket();
  IListenSocketEvents &forIListenSocket();
  ISocketEvents &forISocket();
  OSXScreenEvents &forOSXScreen();
  ClientListenerEvents &forClientListener();
  ClientProxyEvents &forClientProxy();
  ClientProxyUnknownEvents &forClientProxyUnknown();
  ServerEvents &forServer();
  ServerAppEvents &forServerApp();
  IKeyStateEvents &forIKeyState();
  IPrimaryScreenEvents &forIPrimaryScreen();
  IScreenEvents &forIScreen();
  ClipboardEvents &forClipboard();
  FileEvents &forFile();
  EiEvents &forEi();

private:
  ClientEvents *m_typesForClient;
  IStreamEvents *m_typesForIStream;
  IDataSocketEvents *m_typesForIDataSocket;
  IListenSocketEvents *m_typesForIListenSocket;
  ISocketEvents *m_typesForISocket;
  OSXScreenEvents *m_typesForOSXScreen;
  ClientListenerEvents *m_typesForClientListener;
  ClientProxyEvents *m_typesForClientProxy;
  ClientProxyUnknownEvents *m_typesForClientProxyUnknown;
  ServerEvents *m_typesForServer;
  ServerAppEvents *m_typesForServerApp;
  IKeyStateEvents *m_typesForIKeyState;
  IPrimaryScreenEvents *m_typesForIPrimaryScreen;
  IScreenEvents *m_typesForIScreen;
  ClipboardEvents *m_typesForClipboard;
  FileEvents *m_typesForFile;
  EiEvents *m_typesForEi;
  Mutex *m_readyMutex;
  CondVar<bool> *m_readyCondVar;
  std::queue<Event> m_pending;
};

#define EVENT_TYPE_ACCESSOR(type_)                                                                                     \
type_##Events&                                                                \
EventQueue::for##type_()                                                                                               \
  {                                                                                                                    \
    if (m_typesFor##type_ == NULL) {                                                                                   \
      m_typesFor##type_ = new type_##Events();                                                                         \
      m_typesFor##type_->setEvents(dynamic_cast<IEventQueue *>(this));                                                 \
    }                                                                                                                  \
    return *m_typesFor##type_;                                                                                         \
  }
