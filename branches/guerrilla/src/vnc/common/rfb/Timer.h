/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __RFB_TIMER_H__
#define __RFB_TIMER_H__

#include <list>
#ifdef WIN32
//#include <winsock2.h>
#else
#include <sys/time.h>
#endif

namespace rfb {

  /* Timer

     Cross-platform timeout handling.  The caller creates instances of Timer and passes a
     Callback implementation to each.  The Callback will then be called with a pointer to
     the Timer instance that timed-out when the timeout occurs.

     The static methods of Timer are used by the main loop of the application both to
     dispatch elapsed Timer callbacks and to determine how long to wait in select() for
     the next timeout to occur.

  */

  struct Timer {

    struct Callback {
      // handleTimeout
      //   Passed a pointer to the Timer that has timed out.  If the handler returns true
      //   then the Timer is reset and left running, causing another timeout after the
      //   appropriate interval.
      //   If the handler returns false then the Timer is cancelled.
      virtual bool handleTimeout(Timer* t) = 0;
    };

    // checkTimeouts()
    //   Dispatches any elapsed Timers, and returns the number of milliseconds until the
    //   next Timer will timeout.
    static int checkTimeouts();

    // getNextTimeout()
    //   Returns the number of milliseconds until the next timeout, without dispatching
    //   any elapsed Timers.
    static int getNextTimeout();

    // Create a Timer with the specified callback handler
    Timer(Callback* cb_) {cb = cb_;}
    ~Timer() {stop();}

    // startTimer
    //   Starts the timer, causing a timeout after the specified number of milliseconds.
    //   If the timer is already active then it will be implicitly cancelled and re-started.
    void start(int timeoutMs_);

    // stopTimer
    //   Cancels the timer.
    void stop();

    // isStarted
    //   Determines whether the timer is started.
    bool isStarted();

    // getTimeoutMs
    //   Determines the previously used timeout value, if any.
    //   Usually used with isStarted() to get the _current_ timeout.
    int getTimeoutMs();

    // isBefore
    //   Determine whether the Timer will timeout before the specified time.
    bool isBefore(timeval other);

  protected:
    timeval dueTime;
    int timeoutMs;
    Callback* cb;

    static void insertTimer(Timer* t);
    // The list of currently active Timers, ordered by time left until timeout.
    static std::list<Timer*> pending;
  };

};

#endif
