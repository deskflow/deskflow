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

#include "CTimerThread.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CLog.h"

//
// CTimerThread
//

CTimerThread::CTimerThread(double timeout) : m_timeout(timeout)
{
	if (m_timeout >= 0.0) {
		m_callingThread = new CThread(CThread::getCurrentThread());
		m_timingThread  = new CThread(new TMethodJob<CTimerThread>(
								this, &CTimerThread::timer));
	}
	else {
		m_callingThread = NULL;
		m_timingThread  = NULL;
	}
}

CTimerThread::~CTimerThread()
{
	if (m_timingThread != NULL) {
		LOG((CLOG_DEBUG1 "cancelling timeout"));
		m_timingThread->cancel();
		m_timingThread->wait();
		LOG((CLOG_DEBUG1 "cancelled timeout"));
		delete m_timingThread;
		delete m_callingThread;
	}
}

void
CTimerThread::timer(void*)
{
	LOG((CLOG_DEBUG1 "timeout in %f seconds", m_timeout));
	CThread::sleep(m_timeout);
	LOG((CLOG_DEBUG1 "timeout"));
	m_callingThread->cancel();
}
