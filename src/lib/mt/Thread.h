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

#include "arch/IArchMultithread.h"

class IJob;

//! Thread handle
/*!
Creating a Thread creates a new context of execution (i.e. thread) that
runs simulatenously with the calling thread.  A Thread is only a handle
to a thread;  deleting a Thread does not cancel or destroy the thread it
refers to and multiple Thread objects can refer to the same thread.

Threads can terminate themselves but cannot be forced to terminate by
other threads.  However, other threads can signal a thread to terminate
itself by cancelling it.  And a thread can wait (block) on another thread
to terminate.

Most functions that can block for an arbitrary time are cancellation
points.  A cancellation point is a function that can be interrupted by
a request to cancel the thread.  Cancellation points are noted in the
documentation.
*/
// note -- do not derive from this class
class Thread {
public:
    //! Run \c adoptedJob in a new thread
    /*!
    Create and start a new thread executing the \c adoptedJob.  The
    new thread takes ownership of \c adoptedJob and will delete it.
    */
    Thread (IJob* adoptedJob);

    //! Duplicate a thread handle
    /*!
    Make a new thread object that refers to an existing thread.
    This does \b not start a new thread.
    */
    Thread (const Thread&);

    //! Release a thread handle
    /*!
    Release a thread handle.  This does not terminate the thread.  A thread
    will keep running until the job completes or calls exit() or allows
    itself to be cancelled.
    */
    ~Thread ();

    //! @name manipulators
    //@{

    //! Assign thread handle
    /*!
    Assign a thread handle.  This has no effect on the threads, it simply
    makes this thread object refer to another thread.  It does \b not
    start a new thread.
    */
    Thread& operator= (const Thread&);

    //! Terminate the calling thread
    /*!
    Terminate the calling thread.  This function does not return but
    the stack is unwound and automatic objects are destroyed, as if
    exit() threw an exception (which is, in fact, what it does).  The
    argument is saved as the result returned by getResult().  If you
    have \c catch(...) blocks then you should add the following before
    each to avoid catching the exit:
    \code
    catch(ThreadExit&) { throw; }
    \endcode
    or add the \c RETHROW_XTHREAD macro to the \c catch(...) block.
    */
    static void exit (void*);

    //! Cancel thread
    /*!
    Cancel the thread.  cancel() never waits for the thread to
    terminate;  it just posts the cancel and returns.  A thread will
    terminate when it enters a cancellation point with cancellation
    enabled.  If cancellation is disabled then the cancel is
    remembered but not acted on until the first call to a
    cancellation point after cancellation is enabled.

    A cancellation point is a function that can act on cancellation.
    A cancellation point does not return if there's a cancel pending.
    Instead, it unwinds the stack and destroys automatic objects, as
    if cancel() threw an exception (which is, in fact, what it does).
    Threads must take care to unlock and clean up any resources they
    may have, especially mutexes.  They can \c catch(XThreadCancel) to
    do that then rethrow the exception or they can let it happen
    automatically by doing clean up in the d'tors of automatic
    objects (like Lock).  Clients are strongly encouraged to do the latter.
    During cancellation, further cancel() calls are ignored (i.e.
    a thread cannot be interrupted by a cancel during cancellation).

    Clients that \c catch(XThreadCancel) must always rethrow the
    exception.  Clients that \c catch(...) must either rethrow the
    exception or include a \c catch(XThreadCancel) handler that
    rethrows.  The \c RETHROW_XTHREAD macro may be useful for that.
    */
    void cancel ();

    //! Change thread priority
    /*!
    Change the priority of the thread.  Normal priority is 0, 1 is
    the next lower, etc.  -1 is the next higher, etc. but boosting
    the priority may not be permitted and will be silenty ignored.
    */
    void setPriority (int n);

    //! Force pollSocket() to return
    /*!
    Forces a currently blocked pollSocket() in the thread to return
    immediately.
    */
    void unblockPollSocket ();

    //@}
    //! @name accessors
    //@{

    //! Get current thread's handle
    /*!
    Return a Thread object representing the calling thread.
    */
    static Thread getCurrentThread ();

    //! Test for cancellation
    /*!
    testCancel() does nothing but is a cancellation point.  Call
    this to make a function itself a cancellation point.  If the
    thread was cancelled and cancellation is enabled this will
    cause the thread to unwind the stack and terminate.

    (cancellation point)
    */
    static void testCancel ();

    //! Wait for thread to terminate
    /*!
    Waits for the thread to terminate (by exit() or cancel() or
    by returning from the thread job) for up to \c timeout seconds,
    returning true if the thread terminated and false otherwise.
    This returns immediately with false if called by a thread on
    itself and immediately with true if the thread has already
    terminated.  This will wait forever if \c timeout < 0.0.

    (cancellation point)
    */
    bool wait (double timeout = -1.0) const;

    //! Get the exit result
    /*!
    Returns the exit result.  This does an implicit wait().  It returns
    NULL immediately if called by a thread on itself or on a thread that
    was cancelled.

    (cancellation point)
    */
    void* getResult () const;

    //! Get the thread id
    /*!
    Returns an integer id for this thread.  This id must not be used to
    check if two Thread objects refer to the same thread.  Use
    operator==() for that.
    */
    IArchMultithread::ThreadID getID () const;

    //! Compare thread handles
    /*!
    Returns true if two Thread objects refer to the same thread.
    */
    bool operator== (const Thread&) const;

    //! Compare thread handles
    /*!
    Returns true if two Thread objects do not refer to the same thread.
    */
    bool operator!= (const Thread&) const;

    //@}

private:
    Thread (ArchThread);

    static void* threadFunc (void*);

private:
    ArchThread m_thread;
};
