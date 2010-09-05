/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef IARCHMULTITHREAD_H
#define IARCHMULTITHREAD_H

#include "IInterface.h"

/*!      
\class CArchCondImpl
\brief Internal condition variable data.
An architecture dependent type holding the necessary data for a
condition variable.
*/
class CArchCondImpl;

/*!      
\var CArchCond
\brief Opaque condition variable type.
An opaque type representing a condition variable.
*/
typedef CArchCondImpl* CArchCond;

/*!      
\class CArchMutexImpl
\brief Internal mutex data.
An architecture dependent type holding the necessary data for a mutex.
*/
class CArchMutexImpl;

/*!      
\var CArchMutex
\brief Opaque mutex type.
An opaque type representing a mutex.
*/
typedef CArchMutexImpl* CArchMutex;

/*!      
\class CArchThreadImpl
\brief Internal thread data.
An architecture dependent type holding the necessary data for a thread.
*/
class CArchThreadImpl;

/*!      
\var CArchThread
\brief Opaque thread type.
An opaque type representing a thread.
*/
typedef CArchThreadImpl* CArchThread;

//! Interface for architecture dependent multithreading
/*!
This interface defines the multithreading operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchMultithread : public IInterface {
public:
	//! Result of waitForEvent()
	enum EWaitResult {
		kEvent,				//!< An event is pending
		kExit,				//!< Thread exited
		kTimeout			//!< Wait timed out
	};

	//! Type of thread entry point
	typedef void* (*ThreadFunc)(void*);
	//! Type of thread identifier
	typedef unsigned int ThreadID;

	//! @name manipulators
	//@{

	//
	// condition variable methods
	//

	//! Create a condition variable
	/*!
	The condition variable is an opaque data type.
	*/
	virtual CArchCond	newCondVar() = 0;

	//! Destroy a condition variable
	virtual void		closeCondVar(CArchCond) = 0;

	//! Signal a condition variable
	/*!
	Signalling a condition variable releases one waiting thread.
	*/
	virtual void		signalCondVar(CArchCond) = 0;

	//! Broadcast a condition variable
	/*!
	Broadcasting a condition variable releases all waiting threads.
	*/
	virtual void		broadcastCondVar(CArchCond) = 0;

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
	virtual bool		waitCondVar(CArchCond, CArchMutex, double timeout) = 0;

	//
	// mutex methods
	//

	//! Create a non-recursive mutex
	/*!
	Creates a non-recursive mutex.  A thread must not lock a
	non-recursive mutex when it already holds a lock on that mutex.
	If it does it will deadlock.  The mutex is an opaque data type.
	*/
	virtual CArchMutex	newMutex() = 0;

	//! Destroy a mutex
	virtual void		closeMutex(CArchMutex) = 0;

	//! Lock a mutex
	/*!
	(Cancellation point)
	*/
	virtual void		lockMutex(CArchMutex) = 0;

	//! Unlock a mutex
	virtual void		unlockMutex(CArchMutex) = 0;

	//
	// thread methods
	//

	//! Start a new thread
	/*!
	Creates and starts a new thread, using \c func as the entry point
	and passing it \c userData.  The thread is an opaque data type.
	*/
	virtual CArchThread	newThread(ThreadFunc func, void* userData) = 0;

	//! Get a reference to the calling thread
	/*!
	Returns a thread representing the current (i.e. calling) thread.
	*/
	virtual CArchThread	newCurrentThread() = 0;

	//! Copy a thread object
	/*!
	Returns a reference to to thread referred to by \c thread.
	*/
	virtual CArchThread	copyThread(CArchThread thread) = 0;

	//! Release a thread reference
	/*!
	Deletes the given thread object.  This does not destroy the thread
	the object referred to, even if there are no remaining references.
	Use cancelThread() and waitThread() to stop a thread and wait for
	it to exit.
	*/
	virtual void		closeThread(CArchThread) = 0;

	//! Force a thread to exit
	/*!
	Causes \c thread to exit when it next calls a cancellation point.
	A thread avoids cancellation as long as it nevers calls a
	cancellation point.  Once it begins the cancellation process it
	must always let cancellation go to completion but may take as
	long as necessary to clean up.
	*/
	virtual void		cancelThread(CArchThread thread) = 0;

	//! Change thread priority
	/*!
	Changes the priority of \c thread by \c n.  If \c n is positive
	the thread has a lower priority and if negative a higher priority.
	Some architectures may not support either or both directions.
	*/
	virtual void		setPriorityOfThread(CArchThread, int n) = 0;

	//! Cancellation point
	/*!
	This method does nothing but is a cancellation point.  Clients
	can make their own functions cancellation points by calling this
	method at appropriate times.
	*/
	virtual void		testCancelThread() = 0;

	//! Wait for a thread to exit
	/*!
	Waits for up to \c timeout seconds for \c thread to exit (normally
	or by cancellation).  Waits forever if \c timeout < 0.  Returns
	true if the thread exited, false otherwise.  Waiting on the current
	thread returns immediately with false.

	(Cancellation point)
	*/
	virtual bool		wait(CArchThread thread, double timeout) = 0;

	//! Wait for a user event
	/*!
	Waits for up to \c timeout seconds for a pending user event or
	\c thread to exit (normally or by cancellation).  Waits forever
	if \c timeout < 0.  Returns kEvent if an event occurred, kExit
	if \c thread exited, or kTimeout if the timeout expired.  If
	\c thread is NULL then it doesn't wait for any thread to exit
	and it will not return kExit.

	This method is not required by all platforms.

	(Cancellation point)
	*/
	virtual EWaitResult	waitForEvent(CArchThread thread, double timeout) = 0;

	//! Compare threads
	/*!
	Returns true iff two thread objects refer to the same thread.
	Note that comparing thread objects directly is meaningless.
	*/
	virtual bool		isSameThread(CArchThread, CArchThread) = 0;

	//! Test if thread exited
	/*!
	Returns true iff \c thread has exited.
	*/
	virtual bool		isExitedThread(CArchThread thread) = 0;

	//! Returns the exit code of a thread
	/*!
	Waits indefinitely for \c thread to exit (if it hasn't yet) then
	returns the thread's exit code.

	(Cancellation point)
	*/
	virtual void*		getResultOfThread(CArchThread thread) = 0;

	//! Returns an ID for a thread
	/*!
	Returns some ID number for \c thread.  This is for logging purposes.
	All thread objects referring to the same thread return the same ID.
	However, clients should us isSameThread() to compare thread objects
	instead of comparing IDs.
	*/
	virtual ThreadID	getIDOfThread(CArchThread thread) = 0;

	//@}
};

#endif
