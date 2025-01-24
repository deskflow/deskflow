/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiEventQueueBuffer.h"

#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "mt/Thread.h"

#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

class EventQueueTimer
{
};

namespace deskflow {

EiEventQueueBuffer::EiEventQueueBuffer(EiScreen *screen, ei *ei, IEventQueue *events) : ei_(ei_ref(ei)), events_(events)
{
  // We need a pipe to signal ourselves when addEvent() is called
  int pipefd[2];
  int result = pipe2(pipefd, O_NONBLOCK);
  assert(result == 0);

  pipe_r_ = pipefd[0];
  pipe_w_ = pipefd[1];
}

EiEventQueueBuffer::~EiEventQueueBuffer()
{
  ei_unref(ei_);
  close(pipe_r_);
  close(pipe_w_);
}

void EiEventQueueBuffer::waitForEvent(double timeout_in_ms)
{
  Thread::testCancel();

  enum
  {
    EIFD,
    PIPEFD,
    POLLFD_COUNT, // Last element
  };

  struct pollfd pfds[POLLFD_COUNT];
  pfds[EIFD].fd = ei_get_fd(ei_);
  pfds[EIFD].events = POLLIN;
  pfds[PIPEFD].fd = pipe_r_;
  pfds[PIPEFD].events = POLLIN;

  int timeout = (timeout_in_ms < 0.0) ? -1 : static_cast<int>(1000.0 * timeout_in_ms);

  int retval = poll(pfds, POLLFD_COUNT, timeout);
  if (retval > 0) {
    if (pfds[EIFD].revents & POLLIN) {
      std::lock_guard<std::mutex> lock(mutex_);

      // libei doesn't allow ei_event_ref() because events are
      // supposed to be short-lived only. So instead, we create an NULL-data
      // kSystemEvent whenever there's data on the fd, shove that event
      // into our event queue and once we process the event (see
      // getEvent()), the EiScreen will call ei_dispatch() and process
      // all actual pending ei events. In theory this means that a
      // flood of ei events could starve the events added with
      // addEvents() but let's hope it doesn't come to that.
      queue_.push({true, 0U});
    }
    // the pipefd data doesn't matter, it only exists to wake up the thread
    // and potentially testCancel
    if (pfds[PIPEFD].revents & POLLIN) {
      char buf[64];
      auto result = read(pipe_r_, buf, sizeof(buf)); // discard
      LOG_DEBUG2("event queue read result: %d", result);
    }
  }
  Thread::testCancel();
}

IEventQueueBuffer::Type EiEventQueueBuffer::getEvent(Event &event, uint32_t &dataID)
{
  // the addEvent/getEvent pair is a bit awkward for libei.
  //
  // it assumes that there's a nice queue of events sitting there that we can
  // just append to and get everything back out in the same order. We *could*
  // emulate that by taking the libei events immediately out of the event
  // queue after dispatch (see above) and putting it into the event queue,
  // intermixed with whatever addEvents() did.
  //
  // But this makes locking more awkward and libei isn't really designed to
  // keep calling ei_dispatch() while we hold a bunch of event refs. So instead
  // we just have a "something happened" event on the ei fd and the rest is
  // handled by the EiScreen.
  //
  std::lock_guard<std::mutex> lock(mutex_);
  auto pair = queue_.front();
  queue_.pop();

  // if this an injected special event, just return the data and exit
  if (pair.first == false) {
    dataID = pair.second;
    return kUser;
  }

  event = Event(Event::kSystem, events_->getSystemTarget());

  return kSystem;
}

bool EiEventQueueBuffer::addEvent(uint32_t dataID)
{
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.push({false, dataID});

  // tickle the pipe so our read thread wakes up
  auto result = write(pipe_w_, "!", 1);
  LOG_DEBUG2("event queue write result: %d", result);

  return true;
}

bool EiEventQueueBuffer::isEmpty() const
{
  std::lock_guard<std::mutex> lock(mutex_);

  return queue_.empty();
}

EventQueueTimer *EiEventQueueBuffer::newTimer(double duration, bool oneShot) const
{
  return new EventQueueTimer;
}

void EiEventQueueBuffer::deleteTimer(EventQueueTimer *timer) const
{
  delete timer;
}

} // namespace deskflow
