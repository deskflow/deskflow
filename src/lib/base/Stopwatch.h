/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#pragma once

#include "common/common.h"

//! A timer class
/*!
This class measures time intervals.  All time interval measurement
should use this class.
*/
class Stopwatch {
public:
    /*!
    The default constructor does an implicit reset() or setTrigger().
    If triggered == false then the clock starts ticking.
    */
    Stopwatch (bool triggered = false);
    ~Stopwatch ();

    //! @name manipulators
    //@{

    //! Reset the timer to zero
    /*!
    Set the start time to the current time, returning the time since
    the last reset.  This does not remove the trigger if it's set nor
    does it start a stopped clock.  If the clock is stopped then
    subsequent reset()'s will return 0.
    */
    double reset ();

    //! Stop the timer
    /*!
    Stop the stopwatch.  The time interval while stopped is not
    counted by the stopwatch.  stop() does not remove the trigger.
    Has no effect if already stopped.
    */
    void stop ();

    //! Start the timer
    /*!
    Start the stopwatch.  start() removes the trigger, even if the
    stopwatch was already started.
    */
    void start ();

    //! Stop the timer and set the trigger
    /*!
    setTrigger() stops the clock like stop() except there's an
    implicit start() the next time (non-const) getTime() is called.
    This is useful when you want the clock to start the first time
    you check it.
    */
    void setTrigger ();

    //! Get elapsed time
    /*!
    Returns the time since the last reset() (or calls reset() and
    returns zero if the trigger is set).
    */
    double getTime ();
    //! Same as getTime()
    operator double ();
    //@}
    //! @name accessors
    //@{

    //! Check if timer is stopped
    /*!
    Returns true if the stopwatch is stopped.
    */
    bool isStopped () const;

    // return the time since the last reset().
    //! Get elapsed time
    /*!
    Returns the time since the last reset().  This cannot trigger the
    stopwatch to start and will not clear the trigger.
    */
    double getTime () const;
    //! Same as getTime() const
    operator double () const;
    //@}

private:
    double getClock () const;

private:
    double m_mark;
    bool m_triggered;
    bool m_stopped;
};
