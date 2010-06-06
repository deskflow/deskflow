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

#ifndef CCONDVAR_H
#define CCONDVAR_H

#include "CMutex.h"
#include "BasicTypes.h"

class CStopwatch;

//! Generic condition variable
/*!
This class provides functionality common to all condition variables
but doesn't provide the actual variable storage.  A condition variable
is a multiprocessing primitive that can be waited on.  Every condition
variable has an associated mutex.
*/
class CCondVarBase {
public:
	/*!
	\c mutex must not be NULL.  All condition variables have an
	associated mutex.  The mutex needn't be unique to one condition
	variable.
	*/
	CCondVarBase(CMutex* mutex);
	~CCondVarBase();

	//! @name manipulators
	//@{

	//! Lock the condition variable's mutex
	/*!
	Lock the condition variable's mutex.  The condition variable should
	be locked before reading or writing it.  It must be locked for a
	call to wait().  Locks are not recursive;  locking a locked mutex
	will deadlock the thread.
	*/
	void				lock() const;

	//! Unlock the condition variable's mutex
	void				unlock() const;

	//! Signal the condition variable
	/*!
	Wake up one waiting thread, if there are any.  Which thread gets
	woken is undefined.
	*/
	void				signal();

	//! Signal the condition variable
	/*!
	Wake up all waiting threads, if any.
	*/
	void				broadcast();

	//@}
	//! @name accessors
	//@{

	//! Wait on the condition variable
	/*!
	Wait on the condition variable.  If \c timeout < 0 then wait until
	signalled, otherwise up to \c timeout seconds or until signalled,
	whichever comes first.	Returns true if the object was signalled
	during the wait, false otherwise.
	
	The proper way to wait for a condition is:
	\code
	cv.lock();
	while (cv-expr) {
		cv.wait();
    }
	cv.unlock();
	\endcode
	where \c cv-expr involves the value of \c cv and is false when the
	condition is satisfied.

	(cancellation point)
	*/
	bool				wait(double timeout = -1.0) const;

	//! Wait on the condition variable
	/*!
	Same as \c wait(double) but use \c timer to compare against \timeout.
	Since clients normally wait on condition variables in a loop, clients
	can use this to avoid recalculating \c timeout on each iteration.
	Passing a stopwatch with a negative \c timeout is pointless (it will
	never time out) but permitted.

	(cancellation point)
	*/
	bool				wait(CStopwatch& timer, double timeout) const;

	//! Get the mutex
	/*!
	Get the mutex passed to the c'tor.
	*/
	CMutex*				getMutex() const;

	//@}

private:
	// not implemented
	CCondVarBase(const CCondVarBase&);
	CCondVarBase&		operator=(const CCondVarBase&);

private:
	CMutex*				m_mutex;
	CArchCond			m_cond;
};

//! Condition variable
/*!
A condition variable with storage for type \c T.
*/
template <class T>
class CCondVar : public CCondVarBase {
public:
	//! Initialize using \c value
	CCondVar(CMutex* mutex, const T& value);
	//! Initialize using another condition variable's value
	CCondVar(const CCondVar&);
	~CCondVar();

	//! @name manipulators
	//@{

	//! Assigns the value of \c cv to this
	/*!
	Set the variable's value.  The condition variable should be locked
	before calling this method.
	*/
	CCondVar&			operator=(const CCondVar& cv);

	//! Assigns \c value to this
	/*!
	Set the variable's value.  The condition variable should be locked
	before calling this method.
	*/
	CCondVar&			operator=(const T& v);

	//@}
	//! @name accessors
	//@{

	//! Get the variable's value
	/*!
	Get the variable's value.  The condition variable should be locked
	before calling this method.
	*/
						operator const volatile T&() const;

	//@}

private:
	volatile T			m_data;
};

template <class T>
inline
CCondVar<T>::CCondVar(
	CMutex* mutex,
	const T& data) :
	CCondVarBase(mutex),
	m_data(data)
{
	// do nothing
}

template <class T>
inline
CCondVar<T>::CCondVar(
	const CCondVar& cv) :
	CCondVarBase(cv.getMutex()),
	m_data(cv.m_data)
{
	// do nothing
}

template <class T>
inline
CCondVar<T>::~CCondVar()
{
	// do nothing
}

template <class T>
inline
CCondVar<T>&
CCondVar<T>::operator=(const CCondVar<T>& cv)
{
	m_data = cv.m_data;
	return *this;
}

template <class T>
inline
CCondVar<T>&
CCondVar<T>::operator=(const T& data)
{
	m_data = data;
	return *this;
}

template <class T>
inline
CCondVar<T>::operator const volatile T&() const
{
	return m_data;
}

#endif
