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
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace deskflow {

EiEventQueueBuffer::EiEventQueueBuffer(ei *ei, IEventQueue *events) : m_ei{ei_ref(ei)}, m_events{events}
{
  // We need a pipe to signal ourselves when addEvent() is called
  int pipefd[2];
  int result = pipe2(pipefd, O_NONBLOCK);
  assert(result == 0);

  m_pipeRead = pipefd[0];
  m_pipeWrite = pipefd[1];
}

EiEventQueueBuffer::~EiEventQueueBuffer()
{
  ei_unref(m_ei);
  close(m_pipeRead);
  close(m_pipeWrite);
}

void EiEventQueueBuffer::waitForEvent(double msTimeout)
{
  Thread::testCancel();

  static const auto s_eiFd = 0;
  static const auto s_pipeFd = 1;
  static const auto s_pollFdCount = 2;

  struct pollfd pfds[s_pollFdCount];
  pfds[s_eiFd].fd = ei_get_fd(m_ei);
  pfds[s_eiFd].events = POLLIN;
  pfds[s_pipeFd].fd = m_pipeRead;
  pfds[s_pipeFd].events = POLLIN;

  int timeout = (msTimeout < 0.0) ? -1 : static_cast<int>(1000.0 * msTimeout);

  if (int retval = poll(pfds, s_pollFdCount, timeout); retval > 0) {
    if (pfds[s_eiFd].revents & POLLIN) {
      std::scoped_lock lock{m_mutex};

      // libei doesn't allow ei_event_ref() because events are
      // supposed to be short-lived only. So instead, we create an nullptr-data
      // kSystemEvent whenever there's data on the fd, shove that event
      // into our event queue and once we process the event (see
      // getEvent()), the EiScreen will call ei_dispatch() and process
      // all actual pending ei events. In theory this means that a
      // flood of ei events could starve the events added with
      // addEvents() but let's hope it doesn't come to that.
      m_queue.push({true, 0U});
    }
    // the pipefd data doesn't matter, it only exists to wake up the thread
    // and potentially testCancel
    if (pfds[s_pipeFd].revents & POLLIN) {
      char buf[64];
      ssize_t total = 0;
      ssize_t result;
      while ((result = read(m_pipeRead, buf, sizeof(buf)) > 0)) {
        total += result;
      }
      LOG_DEBUG2("event queue read result: %d (total drained: %zd)", result, total);
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
  std::scoped_lock lock{m_mutex};
  auto pair = m_queue.front();
  m_queue.pop();

  // if this an injected special event, just return the data and exit
  if (pair.first == false) {
    dataID = pair.second;
    return IEventQueueBuffer::Type::User;
  }

  event = Event(EventTypes::System, m_events->getSystemTarget());

  return IEventQueueBuffer::Type::System;
}

bool EiEventQueueBuffer::addEvent(uint32_t dataID)
{
  std::scoped_lock lock{m_mutex};
  m_queue.push({false, dataID});

  // tickle the pipe so our read thread wakes up
  auto result = write(m_pipeWrite, "!", 1);
  LOG_DEBUG2("event queue write result: %d", result);

  return true;
}

bool EiEventQueueBuffer::isEmpty() const
{
  std::scoped_lock lock{m_mutex};

  return m_queue.empty();
}

} // namespace deskflow
