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

#include "CThread.h"
#include "CLock.h"
#include "CThreadRep.h"
#include "XThread.h"
#include "CLog.h"
#include "CStopwatch.h"

//
// CThread
//

CThread::CThread(IJob* job, void* userData)
{
	m_rep = new CThreadRep(job, userData);
}

CThread::CThread(const CThread& thread) :
	m_rep(thread.m_rep)
{
	m_rep->ref();
}

CThread::CThread(CThreadRep* rep) :
	m_rep(rep)
{
	// do nothing.  rep should have already been Ref()'d.
}

CThread::~CThread()
{
	m_rep->unref();
}

CThread&
CThread::operator=(const CThread& thread)
{
	if (thread.m_rep != m_rep) {
		m_rep->unref();
		m_rep = thread.m_rep;
		m_rep->ref();
	}
	return *this;
}

void
CThread::init()
{
	CThreadRep::initThreads();
}

void
CThread::sleep(double timeout)
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	if (timeout >= 0.0) {
		currentRep->testCancel();
		currentRep->sleep(timeout);
	}
	currentRep->testCancel();
}

void
CThread::exit(void* result)
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	log((CLOG_DEBUG1 "throw exit on thread %p", currentRep.operator->()));
	throw XThreadExit(result);
}

bool
CThread::enableCancel(bool enable)
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	return currentRep->enableCancel(enable);
}

void
CThread::cancel()
{
	m_rep->cancel();
}

void
CThread::setPriority(int n)
{
	m_rep->setPriority(n);
}

CThread
CThread::getCurrentThread()
{
	return CThread(CThreadRep::getCurrentThreadRep());
}

bool
CThread::wait(double timeout) const
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	return currentRep->wait(m_rep, timeout);
}

#if WINDOWS_LIKE
bool
CThread::waitForEvent(double timeout)
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	return currentRep->waitForEvent(timeout);
}
#endif

void
CThread::testCancel()
{
	CThreadPtr currentRep(CThreadRep::getCurrentThreadRep());
	currentRep->testCancel();
}

void*
CThread::getResult() const
{
	if (wait())
		return m_rep->getResult();
	else
		return NULL;
}

void*
CThread::getUserData()
{
	return m_rep->getUserData();
}

bool
CThread::operator==(const CThread& thread) const
{
	return (m_rep == thread.m_rep);
}

bool
CThread::operator!=(const CThread& thread) const
{
	return (m_rep != thread.m_rep);
}


//
// CThreadMaskCancel
//

CThreadMaskCancel::CThreadMaskCancel() :
	m_old(CThread::enableCancel(false))
{
	// do nothing
}

CThreadMaskCancel::~CThreadMaskCancel()
{
	CThread::enableCancel(m_old);
}
