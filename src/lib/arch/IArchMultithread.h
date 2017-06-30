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

#include "common/IInterface.h"

/*!
\class ArchCondImpl
\brief Internal condition variable data.
An architecture dependent type holding the necessary data for a
condition variable.
*/
class ArchCondImpl;

/*!
\var ArchCond
\brief Opaque condition variable type.
An opaque type representing a condition variable.
*/
typedef ArchCondImpl* ArchCond;

/*!
\class ArchMutexImpl
\brief Internal mutex data.
An architecture dependent type holding the necessary data for a mutex.
*/
class ArchMutexImpl;

/*!
\var ArchMutex
\brief Opaque mutex type.
An opaque type representing a mutex.
*/
typedef ArchMutexImpl* ArchMutex;

/*!
\class ArchThreadImpl
\brief Internal thread data.
An architecture dependent type holding the necessary data for a thread.
*/
class ArchThreadImpl;

/*!
\var ArchThread
\brief Opaque thread type.
An opaque type representing a thread.
*/
typedef ArchThreadImpl* ArchThread;

//! Interface for architecture dependent multithreading
/*!
This interface defines the multithreading operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchMultithread : public IInterface {
public:
    //! Type of thread entry point
    typedef void* (*ThreadFunc) (void*);
    //! Type of thread identifier
    typedef unsigned int ThreadID;
    //! Types of signals
    /*!
    Not all platforms support all signals.  Unsupported signals are
    ignored.
    */
    enum ESignal {
        kINTERRUPT, //!< Interrupt (e.g. Ctrl+C)
        kTERMINATE, //!< Terminate (e.g. Ctrl+Break)
        kHANGUP,    //!< Hangup (SIGHUP)
        kUSER,      //!< User (SIGUSR2)
        kNUM_SIGNALS
    };
    //! Type of signal handler function
    typedef void (*SignalFunc) (ESignal, void* userData);

    //! @name manipulators
    //@{

    //
    // condition variable methods
    //

    //! Create a condition variable
    /*!
    The condition variable is an opaque data type.
    */
    virtual ArchCond newCondVar () = 0;

    //! Destroy a condition variable
    virtual void closeCondVar (ArchCond) = 0;

    //! Signal a condition variable
    /*!
    Signalling a condition variable releases one waiting thread.
    */
    virtual void signalCondVar (ArchCond) = 0;

    //! Broadcast a condition variable
    /*!
    Broadcasting a condition variable releases all waiting threads.
    */
    virtual void broadcastCondVar (ArchCond) = 0;

    //! Wait on a condition variable
    /*!
    Wait on a conditation variable for up to \c timeout seconds.
    If \c timeout is < 0 then there is no timeout.  The mutex must
    be locked when this method is called.  The mutex is unlocked
    during the wait and locked again before returning.  Returns
    true if the condition variable was signalled and false on
    timeout.

    (Cancellation point)
    */
    virtual bool waitCondVar (ArchCond, ArchMutex, double timeout) = 0;

    //
    // mutex methods
    //

    //! Create a recursive mutex
    /*!
    Creates a recursive mutex.  A thread may lock a recursive mutex
    when it already holds a lock on that mutex.  The mutex is an
    opaque data type.
    */
    virtual ArchMutex newMutex () = 0;

    //! Destroy a mutex
    virtual void closeMutex (ArchMutex) = 0;

    //! Lock a mutex
    virtual void lockMutex (ArchMutex) = 0;

    //! Unlock a mutex
    virtual void unlockMutex (ArchMutex) = 0;

    //
    // thread methods
    //

    //! Start a new thread
    /*!
    Creates and starts a new thread, using \c func as the entry point
    and passing it \c userData.  The thread is an opaque data type.
    */
    virtual ArchThread newThread (ThreadFunc func, void* userData) = 0;

    //! Get a reference to the calling thread
    /*!
    Returns a thread representing the current (i.e. calling) thread.
    */
    virtual ArchThread newCurrentThread () = 0;

    //! Copy a thread object
    /*!
    Returns a reference to to thread referred to by \c thread.
    */
    virtual ArchThread copyThread (ArchThread thread) = 0;

    //! Release a thread reference
    /*!
    Deletes the given thread object.  This does not destroy the thread
    the object referred to, even if there are no remaining references.
    Use cancelThread() and waitThread() to stop a thread and wait for
    it to exit.
    */
    virtual void closeThread (ArchThread) = 0;

    //! Force a thread to exit
    /*!
    Causes \c thread to exit when it next calls a cancellation point.
    A thread avoids cancellation as long as it nevers calls a
    cancellation point.  Once it begins the cancellation process it
    must always let cancellation go to completion but may take as
    long as necessary to clean up.
    */
    virtual void cancelThread (ArchThread thread) = 0;

    //! Change thread priority
    /*!
    Changes the priority of \c thread by \c n.  If \c n is positive
    the thread has a lower priority and if negative a higher priority.
    Some architectures may not support either or both directions.
    */
    virtual void setPriorityOfThread (ArchThread, int n) = 0;

    //! Cancellation point
    /*!
    This method does nothing but is a cancellation point.  Clients
    can make their own functions cancellation points by calling this
    method at appropriate times.

    (Cancellation point)
    */
    virtual void testCancelThread () = 0;

    //! Wait for a thread to exit
    /*!
    Waits for up to \c timeout seconds for \c thread to exit (normally
    or by cancellation).  Waits forever if \c timeout < 0.  Returns
    true if the thread exited, false otherwise.  Waiting on the current
    thread returns immediately with false.

    (Cancellation point)
    */
    virtual bool wait (ArchThread thread, double timeout) = 0;

    //! Compare threads
    /*!
    Returns true iff two thread objects refer to the same thread.
    Note that comparing thread objects directly is meaningless.
    */
    virtual bool isSameThread (ArchThread, ArchThread) = 0;

    //! Test if thread exited
    /*!
    Returns true iff \c thread has exited.
    */
    virtual bool isExitedThread (ArchThread thread) = 0;

    //! Returns the exit code of a thread
    /*!
    Waits indefinitely for \c thread to exit (if it hasn't yet) then
    returns the thread's exit code.

    (Cancellation point)
    */
    virtual void* getResultOfThread (ArchThread thread) = 0;

    //! Returns an ID for a thread
    /*!
    Returns some ID number for \c thread.  This is for logging purposes.
    All thread objects referring to the same thread return the same ID.
    However, clients should us isSameThread() to compare thread objects
    instead of comparing IDs.
    */
    virtual ThreadID getIDOfThread (ArchThread thread) = 0;

    //! Set the interrupt handler
    /*!
    Sets the function to call on receipt of an external interrupt.
    By default and when \p func is NULL, the main thread is cancelled.
    */
    virtual void
    setSignalHandler (ESignal, SignalFunc func, void* userData) = 0;

    //! Invoke the signal handler
    /*!
    Invokes the signal handler for \p signal, if any.  If no handler
    cancels the main thread for \c kINTERRUPT and \c kTERMINATE and
    ignores the call otherwise.
    */
    virtual void raiseSignal (ESignal signal) = 0;

    //@}
};
