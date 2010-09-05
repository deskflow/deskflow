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

#ifndef CTIMERTHREAD_H
#define CTIMERTHREAD_H

#include "common.h"

class CThread;

//! A timer thread
/*!
An object of this class cancels the thread that called the c'tor unless
the object is destroyed before a given timeout.
*/
class CTimerThread {
public:
	//! Cancel calling thread after \c timeout seconds
	/*!
	Cancels the calling thread after \c timeout seconds unless destroyed
	before then.  If \c timeout is less than zero then it never times
	out and this is a no-op.  If \c timedOutFlag is not NULL then it's
	set to false in the c'tor and to true if the timeout exipires before
	it's cancelled.
	*/
	CTimerThread(double timeout, bool* timedOutFlag = NULL);
	//! Cancel the timer thread
	~CTimerThread();

private:
	void				timer(void*);

	// not implemented
	CTimerThread(const CTimerThread&);
	CTimerThread& operator=(const CTimerThread&);

private:
	double				m_timeout;
	bool*				m_timedOut;
	CThread*			m_callingThread;
	CThread*			m_timingThread;
};

#endif

