/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/XWindowsEventQueueBuffer.h"

#include "mt/Lock.h"
#include "mt/Thread.h"
#include "base/Event.h"
#include "base/IEventQueue.h"

#include <fcntl.h>
#if HAVE_UNISTD_H
#    include <unistd.h>
#endif
#if HAVE_POLL
#    include <poll.h>
#else
#    if HAVE_SYS_SELECT_H
#        include <sys/select.h>
#    endif
#    if HAVE_SYS_TIME_H
#        include <sys/time.h>
#    endif
#    if HAVE_SYS_TYPES_H
#        include <sys/types.h>
#    endif
#endif

//
// EventQueueTimer
//

class EventQueueTimer { };


//
// XWindowsEventQueueBuffer
//

XWindowsEventQueueBuffer::XWindowsEventQueueBuffer(IXWindowsImpl* impl,
        Display* display, Window window, IEventQueue* events) :
    m_events(events),
    m_display(display),
    m_window(window),
    m_waiting(false)
{
    m_impl = impl;
    assert(m_display != NULL);
    assert(m_window  != None);

    m_userEvent = m_impl->XInternAtom(m_display, "BARRIER_USER_EVENT", False);
    // set up for pipe hack
    int result = pipe(m_pipefd);
    assert(result == 0);

    int pipeflags;
    pipeflags = fcntl(m_pipefd[0], F_GETFL);
    fcntl(m_pipefd[0], F_SETFL, pipeflags | O_NONBLOCK);
    pipeflags = fcntl(m_pipefd[1], F_GETFL);
    fcntl(m_pipefd[1], F_SETFL, pipeflags | O_NONBLOCK);
}

XWindowsEventQueueBuffer::~XWindowsEventQueueBuffer()
{
    // release pipe hack resources
    close(m_pipefd[0]);
    close(m_pipefd[1]);
}

void
XWindowsEventQueueBuffer::waitForEvent(double dtimeout)
{
    Thread::testCancel();

    // clear out the pipe in preparation for waiting.

    char buf[16];
    ssize_t read_response = read(m_pipefd[0], buf, 15);
    
    // with linux automake, warnings are treated as errors by default
    if (read_response < 0)
    {
        // todo: handle read response
    }

    {
        Lock lock(&m_mutex);
        // we're now waiting for events
        m_waiting = true;

        // push out pending events
        flush();
    }
    // calling flush may have queued up a new event.
    if (!XWindowsEventQueueBuffer::isEmpty()) {
        Thread::testCancel();
        return;
    }

    // use poll() to wait for a message from the X server or for timeout.
    // this is a good deal more efficient than polling and sleeping.
#if HAVE_POLL
    struct pollfd pfds[2];
    pfds[0].fd     = ConnectionNumber(m_display);
    pfds[0].events = POLLIN;
    pfds[1].fd     = m_pipefd[0];
    pfds[1].events = POLLIN;
    int timeout    = (dtimeout < 0.0) ? -1 :
                        static_cast<int>(1000.0 * dtimeout);
    int remaining  =  timeout;
    int retval     =  0;
#else
    struct timeval timeout;
    struct timeval* timeoutPtr;
    if (dtimeout < 0.0) {
        timeoutPtr = NULL;
    }
    else {
        timeout.tv_sec  = static_cast<int>(dtimeout);
        timeout.tv_usec = static_cast<int>(1.0e+6 *
                                (dtimeout - timeout.tv_sec));
        timeoutPtr      = &timeout;
    }

    // initialize file descriptor sets
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(ConnectionNumber(m_display), &rfds);
    FD_SET(m_pipefd[0], &rfds);
     int nfds;
     if (ConnectionNumber(m_display) > m_pipefd[0]) {
         nfds = ConnectionNumber(m_display) + 1;
     }
     else {
         nfds = m_pipefd[0] + 1;
     }
#endif
    // It's possible that the X server has queued events locally
    // in xlib's event buffer and not pushed on to the fd. Hence we
    // can't simply monitor the fd as we may never be woken up.
    // ie addEvent calls flush, XFlush may not send via the fd hence
    // there is an event waiting to be sent but we must exit the poll
    // before it can.
    // Instead we poll for a brief period of time (so if events
    // queued locally in the xlib buffer can be processed)
    // and continue doing this until timeout is reached.
    // The human eye can notice 60hz (ansi) which is 16ms, however
    // we want to give the cpu a chance s owe up this to 25ms
#define TIMEOUT_DELAY 25

    while (((dtimeout < 0.0) || (remaining > 0)) && QLength(m_display)==0 && retval==0){
#if HAVE_POLL
    retval = poll(pfds, 2, TIMEOUT_DELAY); //16ms = 60hz, but we make it > to play nicely with the cpu
     if (pfds[1].revents & POLLIN) {
         ssize_t read_response = read(m_pipefd[0], buf, 15);
        
        // with linux automake, warnings are treated as errors by default
        if (read_response < 0)
        {
            // todo: handle read response
        }

     }
#else
    retval = select(nfds,
                        SELECT_TYPE_ARG234 &rfds,
                        SELECT_TYPE_ARG234 NULL,
                        SELECT_TYPE_ARG234 NULL,
                        SELECT_TYPE_ARG5   TIMEOUT_DELAY);
    if (FD_SET(m_pipefd[0], &rfds)) {
        read(m_pipefd[0], buf, 15);
    }
#endif
        remaining-=TIMEOUT_DELAY;
    }

    {
        // we're no longer waiting for events
        Lock lock(&m_mutex);
        m_waiting = false;
    }

    Thread::testCancel();
}

IEventQueueBuffer::Type
XWindowsEventQueueBuffer::getEvent(Event& event, UInt32& dataID)
{
    Lock lock(&m_mutex);

    // push out pending events
    flush();

    // get next event
    m_impl->XNextEvent(m_display, &m_event);

    // process event
    if (m_event.xany.type == ClientMessage &&
        m_event.xclient.message_type == m_userEvent) {
        dataID = static_cast<UInt32>(m_event.xclient.data.l[0]);
        return kUser;
    }
    else {
        event = Event(Event::kSystem,
                            m_events->getSystemTarget(), &m_event);
        return kSystem;
    }
}

bool
XWindowsEventQueueBuffer::addEvent(UInt32 dataID)
{
    // prepare a message
    XEvent xevent;
    xevent.xclient.type         = ClientMessage;
    xevent.xclient.window       = m_window;
    xevent.xclient.message_type = m_userEvent;
    xevent.xclient.format       = 32;
    xevent.xclient.data.l[0]    = static_cast<long>(dataID);

    // save the message
    Lock lock(&m_mutex);
    m_postedEvents.push_back(xevent);

    // if we're currently waiting for an event then send saved events to
    // the X server now.  if we're not waiting then some other thread
    // might be using the display connection so we can't safely use it
    // too.
    if (m_waiting) {
        flush();
        // Send a character through the round-trip pipe to wake a thread
        // that is waiting for a ConnectionNumber() socket to be readable.
        // The flush call can read incoming data from the socket and put
        // it in Xlib's input buffer.  That sneaks it past the other thread.
        ssize_t write_response = write(m_pipefd[1], "!", 1);

        // with linux automake, warnings are treated as errors by default
        if (write_response < 0)
        {
            // todo: handle read response
        }
    }

    return true;
}

bool
XWindowsEventQueueBuffer::isEmpty() const
{
    Lock lock(&m_mutex);
    return (m_impl->XPending(m_display) == 0 );
}

EventQueueTimer*
XWindowsEventQueueBuffer::newTimer(double, bool) const
{
    return new EventQueueTimer;
}

void
XWindowsEventQueueBuffer::deleteTimer(EventQueueTimer* timer) const
{
    delete timer;
}

void
XWindowsEventQueueBuffer::flush()
{
    // note -- m_mutex must be locked on entry

    // flush the posted event list to the X server
    for (size_t i = 0; i < m_postedEvents.size(); ++i) {
        m_impl->XSendEvent(m_display, m_window, False, 0, &m_postedEvents[i]);
    }
    m_impl->XFlush(m_display);
    m_postedEvents.clear();
}
